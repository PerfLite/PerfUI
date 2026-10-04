# Skyrim UI Input & Cursor Integration Guide (PerfUI)

[English](SKYRIM_INPUT_GUIDE_EN.md) | [Русский](SKYRIM_INPUT_GUIDE.md)

This document describes the critical rules, architectural caveats, and solutions for managing input events, hardware mouse integration, and cursor visibility in custom DirectX 11 user interface mods for *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition* (SKSE64 / CommonLibSSE).

---

## 1. Input Architecture in Skyrim SE: DirectInput vs Windows Messages

### The Problem
In standard Win32 desktop applications, mouse interactions arrive through the window procedure (`WndProc`) via `WM_LBUTTONDOWN`, `WM_LBUTTONUP`, and `WM_MOUSEMOVE` messages.

In Skyrim SE, the game engine accesses mouse hardware directly through **DirectInput8** (`RE::BSWin32MouseDevice`). Consequently:
- `WM_LBUTTONDOWN` and `WM_LBUTTONUP` messages **are NOT posted** to the game window upon clicks.
- Coordinates in `WM_MOUSEMOVE` messages may be missing or frozen because the device operates in raw/exclusive mode.

### The Solution
1. **Mouse Clicks:** Intercept via `RE::BSTEventSink<RE::InputEvent*>`:
   ```cpp
   if (event->GetDevice() == RE::INPUT_DEVICE::kMouse &&
       event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton)
   {
       auto* btn = event->AsButtonEvent();
       uint32_t code = btn->GetIDCode(); // 0 = Left, 1 = Right, 2 = Middle, 8 = WheelUp, 9 = WheelDown
       if (btn->IsDown()) { /* Click down */ }
       else if (btn->IsUp()) { /* Click released */ }
   }
   ```
2. **Mouse Coordinates:** Query every render frame via Win32 API:
   ```cpp
   POINT pt;
   if (::GetCursorPos(&pt) && ::ScreenToClient(hWnd, &pt)) {
       Point mousePos{ static_cast<float>(pt.x), static_cast<float>(pt.y) };
       uiContext->onMouseMove(mousePos);
   }
   ```

---

## 2. Cursor Dilemma: Hardware Arrow vs Flash Cursor

### Main Menu Cursor Rendering Architecture
In the vanilla Skyrim main menu, the dragon cursor is rendered inside the Flash/Scaleform movie `mainmenu.swf`. When an author replaces or hides the vanilla interface (`uiMovie->SetVisible(false)`), the Flash dragon cursor disappears along with the vanilla buttons.

Therefore, the only correct cursor for a custom DirectX 11 menu is the **hardware Windows cursor** (`IDC_ARROW`).

---

### Critical Bug: Cursor Disappears on First Click (ShowCursor Display Counter Drop)

> [!CAUTION]
> **Symptom:** The menu opens, and the cursor is visible and moves normally. However, as soon as the player clicks **for the first time**, the cursor permanently vanishes, even though buttons continue to highlight on hover and register clicks!
>
> **Root Cause:**
> 1. In the Win32 API, `SetCursor(hCursor)` displays the cursor **only if the internal visibility counter of `ShowCursor` is greater than or equal to zero (`>= 0`)**. If the counter drops to `-1`, the cursor is hidden at the hardware level, regardless of how often `SetCursor` is invoked.
> 2. Whenever the player clicks a mouse button, DirectInput and Skyrim's internal input handler (`BSWin32MouseDevice`) intercept the click and internally invoke `ShowCursor(FALSE)`, decrementing the counter from `0` to `-1`.
> 3. Calling `while (::ShowCursor(TRUE) < 0)` only once when opening the menu is insufficient: the first click drops the counter back to `-1`, rendering the cursor permanently invisible.

### The Ironclad Fix:
Restore the `ShowCursor` counter back to `0` **on every render frame and in every click handler**:
1. **On every DirectX 11 Present frame (`D3D11Hook::Hooked_Present`)**:
   ```cpp
   // Even if Skyrim decremented the counter on click, it is restored within 16 ms
   while (::ShowCursor(TRUE) < 0);
   ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
   ```
2. **In the DirectInput click handler (`InputHook::ProcessEvent`)**:
   ```cpp
   if (keyCode <= 2) {
       while (::ShowCursor(TRUE) < 0);
       ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
       // ... process click ...
   }
   ```
3. **In Window Messages (`WndProc` / `WM_SETCURSOR` / `WM_LBUTTONDOWN`)**:
   ```cpp
   if (msg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
       while (::ShowCursor(TRUE) < 0);
       ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
       return TRUE;
   }
   ```
4. **Why `while (... < 0)` is critical:**  
   It guarantees that the counter never exceeds `0`. The loop halts at exactly `0` (the visibility threshold) and will never blow up into large positive numbers that cannot be hidden upon closing the menu.
5. **Disable ImGui Software Cursor:**
   ```cpp
   ImGui::GetIO().MouseDrawCursor = false;
   ```
This permanently ensures zero cursor latency, eliminates ghost cursor duplicates, and guarantees 100% stable cursor visibility across all clicks and movements.

---

## 3. ImGui Mouse Capture Conflicts (`SetCapture` / `ReleaseCapture`)

### Cause of Dropped Clicks
The standard Dear ImGui Win32 implementation (`ImGui_ImplWin32_WndProcHandler`):
- Invokes `::SetCapture(hWnd)` on left-click down.
- Invokes `::ReleaseCapture()` on release.

In Skyrim, this clashes directly with `BSInputDeviceManager`:
- Spurious `WM_CAPTURECHANGED` messages are emitted.
- `WM_LBUTTONUP` messages are dropped.
- UI buttons get stuck in the pressed state, and `onClick` events fail to fire.

### The Solution
**Do not route mouse events through `ImGui_ImplWin32_WndProcHandler`.**  
In Skyrim plugins, ImGui must serve **strictly as a rendering backend** for vertex streaming, while all input routing must proceed through PerfUI's decoupled input hook.

---

## 4. Thread Safety: The Thread-Safe Event Queue

### Cause of Intermittent Crashes (Race Conditions)
- `BSInputDeviceManager::ProcessEvent` is called from the **main game thread / input thread**.
- `IDXGISwapChain::Present` is called from the **DirectX 11 render thread**.

Directly mutating UI widget hierarchies (`onMouseDown`, `onMouseUp`, swapping tab panels) inside `ProcessEvent` results in two threads concurrently reading and writing the `UIElement` tree. This causes memory access violations and crashes to desktop (CTD).

### The Solution: Thread-Safe Queue
1. `ProcessEvent` only pushes input events into a thread-safe queue:
   ```cpp
   PushInput({ QueuedInput::Type::MouseDown, keyCode, mouseX, mouseY });
   ```
2. At the start of `Hooked_Present` on the D3D11 render thread, the queue is drained:
   ```cpp
   auto events = InputHook::GetSingleton().DrainInputQueue();
   for (const auto& ev : events) {
       uiContext->onMouseDown(...);
   }
   uiContext->update(deltaTime);
   uiContext->render(backend);
   ```
All UI tree mutations occur strictly on a single thread.

---

## 5. Save Game Loading via `BGSSaveLoadManager`

### Nuances of `BGSSaveLoadManager`
- The function `LoadMostRecentSaveGame()` searches inside the internal array `saveManager->saveGameList`.
- The array `saveGameList` is **empty** until the player opens the vanilla Scaleform save/load menu.
- Calling `LoadMostRecentSaveGame()` when this list is unpopulated silently returns `false` without loading anything.
- **The Correct Approach:** Locate the newest `.ess` file on disk and invoke `saveManager->Load(fileName.c_str(), false)`.
- **Never invoke two load requests concurrently:** Calling `Load()` while simultaneously executing a console command `load` crashes Skyrim during the loading screen.

---

## 6. Critical WndProc Hook Crash: ANSI vs Unicode (`0xFFFFxxxx` Pseudo-Handles)

### Symptom
Crash with `Unhandled exception EXCEPTION_ACCESS_VIOLATION at 0x0000FFFF03D3` inside third-party mods (e.g. `SKSEMenuFramework.dll+00E6DA0`).

### Root Cause
1. Skyrim SE's window is created as an ANSI window (`CreateWindowExA`).
2. If a plugin invokes `SetWindowLongPtrW` (Unicode) on an ANSI window, `USER32.dll` does not return a genuine function pointer; instead, it returns a Unicode-to-ANSI translation **pseudo-handle** of the form `0xFFFFxxxx` (e.g. `0xFFFF03D3`).
3. Other mods hook the window procedure and invoke the returned address directly as `func(...)`. The CPU instruction `jmp rax` crashes on `0x0000FFFFxxxx`.

### Ironclad Rules:
1. **Always use ANSI versions of Windows APIs:**
   ```cpp
   // CORRECT:
   m_originalWndProc = reinterpret_cast<WNDPROC>(
       ::SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hooked_WndProc))
   );
   
   // Inside Hooked_WndProc:
   return CallWindowProcA(hook.m_originalWndProc, hWnd, msg, wParam, lParam);
   ```
2. **Do not install WndProc hooks too early:**
   - The D3D11 graphics hook can be installed early (in `BSGraphics::InitD3D` for frame 0).
   - The `WndProc` and `BSInputDeviceManager` hooks should be installed **strictly upon `kDataLoaded`**, when the window and external mods are fully initialized.

---

## 7. Zero-Frame Dragon Flash Elimination

### The Problem
When clicking "Continue" in a custom Main Menu, the vanilla 3D dragon flashed on screen for 1 second while the engine loaded save data from disk.

### Root Cause
Clicks are processed mid-frame in `Hooked_Present`. If the menu hides itself (`m_isVisible = false`) after the loading screen check has already passed, nothing is drawn in the current frame. The engine presents its backbuffer showing the 3D dragon and immediately blocks the main thread in `saveManager->Load(...)`.

### The Solution
In `Hooked_Present`, immediately after draining the input event queue, check `IsLoadingMenuOpen()`:
```cpp
if (MainMenuManager::GetSingleton().IsLoadingMenuOpen()) {
    renderLoadingScreen(); // Paint solid black background with spinner
} else {
    uiContext->update(deltaTime);
    uiContext->render(backend);
}
```
The frame prior to disk blocking is guaranteed to display a solid black loading screen.
