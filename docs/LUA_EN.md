# Lua 5.4 Guide for PerfUI (PerfUI_Lua Module)

## 1. Overview and Purpose

The **PerfUI_Lua** module enables mod developers to implement **user interface logic** using the Lua 5.4 scripting language:
- Event handlers for buttons, checkboxes, sliders, dropdowns, and tabs.
- Interactive calculations and data formatting.
- One-shot (`ui.timer`) and periodic (`ui.interval`) timers.
- In-game toast notifications (`ui.toast`).
- Design token queries (`ui.theme.color`).

> [!IMPORTANT]
> **Separation of Concerns:**
> Visual layout and styling remain declarative within XML markup files (`PerfUI_Markup`).
> Lua scripts handle lightweight UI state and logic.
> Heavy-duty tasks (3D rendering, complex math, iterating through hundreds of game actors) remain in native C++.

> [!WARNING]
> **Script Security:**
> Scripts represent executable code. Only run scripts from trusted mods.
> The module isolates scripts within a secure sandbox (`io`, `os`, `debug`, raw memory access, networking, and arbitrary file system calls are blocked). However, malicious scripts could attempt to exhaust CPU time or memory quotas.

---

## 2. Architecture and Isolation

- **Per-Mod Script Host:** Each mod receives its own dedicated `ScriptHost` with an isolated `lua_State`. Global variables from one mod are strictly invisible to other mods.
- **Clean C API Without Header Leaks:** Internal Lua types (`lua_State*`, `lua.h`) are never leaked into the SDK (`PerfUIApi.h`) or public headers of `PerfUI_Core` or `PerfUI_Markup`.
- **Thread Safety:** Scripts execute **strictly on the UI thread**. Invocations from background game threads must be dispatched via `PerfUI::Client::RunOnUIThread`. Debug builds assert UI thread ownership.

---

## 3. Sandboxing and Execution Limits

### 3.1 Allowed and Blocked Libraries

| Status | Libraries and Built-in Functions |
| :--- | :--- |
| **Allowed** | `base` (`print`, `assert`, `pcall`, `type`, `tostring`, `tonumber`, `select`, `ipairs`, `pairs`, `next`, `error`), `table`, `string`, `math`, `utf8`, `coroutine`, `collectgarbage("count")` |
| **Blocked** | `io` (all I/O functions), `os` (process and system calls), `package` (default module loader), `debug` (stack introspection), `dofile`, `loadfile`, arbitrary `load`, `collectgarbage("collect")` |
| **Safe require** | Custom sandboxed `require(name)`: only allows loading `.lua` modules located within the mod script's own folder. Relative traversal (`..`) or absolute paths (`C:/`, `/etc`) trigger a security error. |

### 3.2 Execution Quotas (`ScriptLimits`)

To safeguard the game against hitching, freezes, and memory leaks, strict per-call limits are enforced:
- **Instruction Limit:** Defaults to 1,000,000 virtual instructions per call. Exceeding this quota aborts the call with an error.
- **Time Limit:** Defaults to 50.0 ms per call (monitored via `lua_sethook`).
- **Memory Quota:** Defaults to 16 MB per `ScriptHost` instance (managed by a custom `lua_Alloc`). Exceeding the quota raises an OOM error without crashing the game.
- **Circuit Breaker:** After 5 consecutive unhandled script errors, the host enters a `disabled` state, halting script execution while keeping the interface interactive.

---

## 4. API Reference v1

### 4.1 Global `ui` Table

```lua
-- Find an element by name in the active UI hierarchy
local elem = ui.find("SaveButton") -- returns a UIElement handle or nil

-- Display an in-game toast notification
ui.toast("Settings saved successfully", "success") -- types: "info", "success", "warning", "error"

-- One-shot timer (delay in seconds, callback function)
local timerId = ui.timer(2.0, function()
    log.info("2 seconds elapsed!")
end)

-- Recurring timer (interval in seconds, callback function)
local intervalId = ui.interval(0.5, function()
    log.info("Tick every 500 ms")
end)

-- Cancel an active timer or interval by ID
local cancelled = ui.cancel(timerId) -- true if successfully cancelled

-- Query theme color tokens
local color = ui.theme.color("borderFocus")
-- Returns a table: { r = 212, g = 175, b = 55, a = 255, hex = "#D4AF37FF" }
```

### 4.2 Element Handles (`UIElement Handle`)

Handles **never store raw C++ pointers**. Each handle encapsulates an `ElementId` and a weak observer `WeakElementRef`. If an element is destroyed (e.g., during live hot-reload or container rebuild), invoking methods on stale handles **will not crash the process**: a warning is logged, and the call returns `nil` or `false` safely.

#### Common Element Methods:
- `handle:id()` -> integer element ID.
- `handle:name()` -> string element name.
- `handle:isValid()` -> `true` if the underlying C++ element is still alive.
- `handle:isVisible()` -> boolean visibility flag.
- `handle:setVisible(bool)` -> set visibility.
- `handle:isEnabled()` -> boolean interactivity flag.
- `handle:setEnabled(bool)` -> enable or disable element.
- `handle:isFocused()` -> `true` if element currently holds input focus.
- `handle:opacity()` -> float value between `0.0` and `1.0`.
- `handle:setOpacity(number)` -> set element opacity.

#### Widget-Specific Methods:
- `handle:text()` -> current text (`Text`, `Button`, `TextInput`, `Checkbox`, `Slider`).
- `handle:setText(str)` -> update text content.
- `handle:value()` -> current numeric value (`Slider`, `ProgressBar`, `Checkbox`).
- `handle:setValue(num)` -> set numeric value.
- `handle:isChecked()` -> `true`/`false` for `Checkbox`.
- `handle:setChecked(bool)` -> toggle checkbox checkmark state.
- `handle:selectedIndex()` -> selected index (`ComboBox`, `TabBar`).
- `handle:selectIndex(index)` -> select item by index.
- `handle:targetId()` / `handle:setTargetId(id)` -> TargetID / FormID for `WorldAnchor`.
- `handle:setScreenOffset(ox, oy)` -> pixel offset for `WorldAnchor`.
- `handle:isClamped()` -> `true` if `WorldAnchor` is clamped against screen boundary.
- `handle:clampedAngle()` -> clamping arrow orientation in radians for `WorldAnchor`.
- `handle:currentScale()` -> distance-based scale factor for `WorldAnchor`.
- `handle:setTexture(texId)` -> update texture / D3D11 SRV ID for `Image`.
- `handle:setUV(u0, v0, u1, v1)` -> UV bounds mapping for `Image`.
- `handle:setFlipY(bool)` -> vertical inversion (Direct3D Render Target) for `Image`.

### 4.3 Localization (`ui.tr`, `ui.translate`)

PerfUI provides a built-in localization service supporting both JSON (`{ "KEY": "Text" }`) and standard Skyrim translation files (`$KEY\tText` or `KEY=Text`):

```lua
-- Retrieve localized string with fallback
local saveText = ui.tr("BTN_SAVE", "Save")

-- Switch active language code
ui.setLanguage("ru")
log.info("Active language: " .. ui.getLanguage())

-- Load mod translation dictionary dynamically
ui.loadDictionary("ru", "Data/Interface/MyMod/lang_ru.json")
```

### 4.4 Logging `log`

Scripts output messages into the PerfUI logging pipeline (in-game written to `SKSE.log` prefixed with `[Lua:<ModName>]` alongside file and line references):
```lua
log.info("Informational message")
log.warn("Warning regarding non-standard state")
log.error("Critical logic error")
```

---

## 5. XML Markup Integration (`PerfUI_Markup`)

In the root element of your XML layout, specify the `script` attribute:
```xml
<UI script="main.lua">
  <Panel name="Root" padding="16" gap="8">
    <Button name="BtnSave" text="Save" onClick="onSaveClicked"/>
    <Checkbox name="SoundToggle" text="Sound" onToggle="onSoundToggled"/>
    <Slider name="VolSlider" min="0" max="100" value="50" onValueChanged="onVolumeChanged"/>
  </Panel>
</UI>
```

In `main.lua`:
```lua
function onSaveClicked()
    ui.toast("Settings saved!", "success")
end

function onSoundToggled(enabled)
    log.info("Sound toggled: " .. tostring(enabled))
end

function onVolumeChanged(newVal)
    log.info("New volume: " .. tostring(newVal))
end
```

### Callback Precedence
If both a native C++ callback (via `loader.bindCallback(...)`) and a Lua function share the same name (e.g., `onSaveClicked`), **C++ always takes precedence**. The Lua callback is only invoked if no native C++ handler is registered.

### Live Hot-Reload
When an XML markup or Lua script file is modified on disk:
1. Script state is reset (`host.reset()`).
2. All active timers and intervals are automatically cancelled.
3. Stale element handles become invalid (`handle:isValid() == false`).
4. Updated script code is loaded, and the UI element tree is reconstructed seamlessly.

---

## 6. Extending from C++ Mods (`registerModule`)

Lua scripts have no direct access to Skyrim engine memory. C++ mods can safely expose game data and helper functions via registered modules:

```cpp
auto* host = PerfUI::Client::CreateScriptHost("MyMod");

host->registerModule("game", [](PerfUI::ModuleBuilder& m) {
    m.function("getPlayerHealth", []() -> double {
        return CachedPlayerHealth();
    });
    m.function("playSound", [](std::string soundId) {
        SKSE::log::info("Play sound: {}", soundId);
    });
});
```

In Lua:
```lua
local hp = game.getPlayerHealth()
if hp < 0.2 then
    game.playSound("UIHeartbeat")
end
```
Any C++ exceptions thrown inside module functions are safely caught and converted into standard Lua errors without crashing the game.

---

## 7. Troubleshooting and Common Errors

1. **`attempt to use destroyed element 'MyButton' in method 'setText'`**
   - *Cause:* The UI element hierarchy was reconstructed (e.g., markup reload or container clear), but the script referenced a cached handle.
   - *Solution:* Re-query the element handle using `ui.find("MyButton")`.

2. **`instruction limit exceeded (max 1000000 per call)`**
   - *Cause:* Infinite loop `while true do` or overly heavy recursive algorithm in Lua.
   - *Solution:* Split work across frames using `ui.interval` or move heavy computations to C++.

3. **`time limit exceeded (max 50.0 ms per call)`**
   - *Cause:* Script stalled the UI thread longer than the acceptable frame time budget.
   - *Solution:* Optimize logic or offload computations to background C++ threads.

4. **`not enough memory`**
   - *Cause:* Script allocated more than 16 MB of heap memory (e.g., concatenating massive strings).
   - *Solution:* Avoid storing large binary or text payloads in Lua state.
