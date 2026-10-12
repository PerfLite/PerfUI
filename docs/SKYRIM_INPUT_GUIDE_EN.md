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

---

## 8. Dual Click Handling (DirectInput + WndProc Dual Fallback) and Lazy `BSInputDeviceManager` Registration

### 1. Critical pitfall: early `BSInputDeviceManager` initialization
Inside the early graphics interception point `BSGraphics::InitD3D` (frame 0) the
`RE::BSInputDeviceManager::GetSingleton()` instance is **not created yet** and returns `nullptr`.
If you try to call `AddEventSink` at that stage:
- The call is silently skipped.
- The `m_installed` flag still becomes `true`.
- When `Install()` runs again at `kDataLoaded`, the hook thinks it is already installed and returns early.
- As a result `ProcessEvent` is **never called**, so DirectInput mouse events never arrive at all!

**Iron rule:** register lazily via `EnsureInputSink()`:
```cpp
void InputHook::EnsureInputSink() {
    if (m_sinkRegistered.load()) return;
    auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputManager) {
        inputManager->AddEventSink(this);
        m_sinkRegistered.store(true);
        SKSE::log::info("InputHook: Registered BSInputDeviceManager event sink");
    }
}
```
Call `EnsureInputSink()`:
- In `InputHook::Install()` (at `kDataLoaded`).
- In `InputHook::SetCaptureInput(true)` (whenever a modal menu opens).
- In `Hooked_WndProc` on the first message while input capture is active.

### 2. Dual mouse click handling (DirectInput + Windows Messages)
In windowed and borderless modes Windows delivers plain `WM_LBUTTONDOWN`, `WM_LBUTTONUP`,
`WM_LBUTTONDBLCLK` messages to `WndProc`. In exclusive fullscreen those messages may never reach
the window, and clicks arrive exclusively through DirectInput (`BSInputDeviceManager::ProcessEvent`).

Relying on only one source, or simply swallowing messages in `WndProc` (`return 0`), loses clicks
in one of the two display modes.

**The right architecture is a shared button state array `m_mouseButtonDown[3]`:**
- **In `Hooked_WndProc`:**
  ```cpp
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK: {
      if (!hook.m_mouseButtonDown[0]) {
          hook.m_mouseButtonDown[0] = true;
          hook.PushInput({ QueuedInput::Type::MouseDown, 0, x, y });
      }
      return 0;
  }
  case WM_LBUTTONUP: {
      if (hook.m_mouseButtonDown[0]) {
          hook.m_mouseButtonDown[0] = false;
          hook.PushInput({ QueuedInput::Type::MouseUp, 0, x, y });
      }
      return 0;
  }
  ```
- **In `ProcessEvent` (DirectInput):**
  ```cpp
  bool isPressed = btnEvent->IsPressed();
  if (isPressed && !m_mouseButtonDown[keyCode]) {
      m_mouseButtonDown[keyCode] = true;
      PushInput({ QueuedInput::Type::MouseDown, keyCode, x, y });
  } else if (!isPressed && m_mouseButtonDown[keyCode]) {
      m_mouseButtonDown[keyCode] = false;
      PushInput({ QueuedInput::Type::MouseUp, keyCode, x, y });
  }
  ```
- On `SetCaptureInput(false)` every element of the array is reset to `false`.

**Result:**
1. Whichever handler catches the click first pushes the event into the queue immediately.
2. The second handler sees the flag already set and safely ignores the duplicate.
3. Mouse clicks are guaranteed to work with zero delay in any Skyrim SE display mode.

> **Important:** the dual fallback applies to **mouse buttons only**. The DirectInput keyboard
> branch (`scanCode == 1` for ESC, `28` for Enter) was removed from `ProcessEvent`, see section 9.

---

## 9. Single `ControlMap` Owner and the WndProc-Only Keyboard Path

### 1. `ToggleControls` is a toggle, not a setter
`RE::ControlMap::ToggleControls(UEFlag::kAll, !capture)` used to be called from two places:
`D3D11Hook::SetUIVisible()` and `InputHook::SetCaptureInput()` — and the former calls the latter.
Two identical calls in a row flip the flags twice, i.e. put them back where they started: the menu
is open while the game keeps receiving WASD, camera movement and weapon swings.

**Rule:** the `ToggleControls` call lives **only** in `D3D11Hook::SetUIVisible()`, marshalled through
`SKSE::GetTaskInterface()->AddTask` because `ControlMap` may only be touched from the game thread.
`InputHook::SetCaptureInput()` is responsible for the `ShowCursor` counter and the `m_captureInput`
flag, nothing else.

### 2. Keyboard arrives through `WndProc` only
While the UI is open, `Hooked_WndProc` swallows `WM_KEYDOWN / WM_SYSKEYDOWN / WM_KEYUP / WM_CHAR`
(`return 0`) and pushes the codes into the `InputHook` queue, which `Hooked_Present` drains into
`UIContext::onKeyDown()`. `BSInputDeviceManager::ProcessEvent` returns `kStop` in the same period,
so DirectInput keys never reach the game and no duplicates are produced.

### 3. Bubbling up the tree
`UIContext::onKeyDown()` offers the key to registered hotkeys first, then to an open `ComboBox`,
then to the active modal. From there the key travels **up the parent chain** from the focused
element; when nothing has focus it is offered to the visible children of the root. This lets a
container react to a key none of its children consumed — and is the price paid for removing key
handling from the hook.

### 4. Consequences worth knowing
- **ESC no longer closes the menu at hook level.** Closing is now the widget's job: its `onKeyDown()`
  must return `true`. Currently ESC is handled by `ComboBox` (closing the dropdown), `ModalDialog`,
  `TextInput` and `SettingsView` (cancelling a rebind), but **not** by `JournalWindow` /
  `MainMenuWindow`. Menus close via their exit button, `VK_F10` (main menu toggle), or
  `SetUIVisible(false)` from mod code.
- **Exclusive fullscreen:** keyboard messages may fail to reach the window there, exactly like clicks
  (see section 8). Since DirectInput keys are no longer intercepted, keyboard navigation in that mode
  needs its own fallback following the same pattern as the mouse one.
- Always verify against `SKSE\Plugins\PerfUI.log`: the line
  `InputHook: Registered BSInputDeviceManager event sink` must appear before the first click.
