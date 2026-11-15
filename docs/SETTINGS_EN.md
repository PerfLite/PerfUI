# PerfUI Mod Settings Layer (Modern MCM Alternative)

[English](SETTINGS_EN.md) | [Русский](SETTINGS.md)

PerfUI provides a modular, ultra-high-performance mod configuration layer for Skyrim SE/AE/VR, designed as a modern, instant, Flash/Scaleform-independent alternative to SkyUI MCM and MCM Helper.

The PerfUI Settings Layer integrates:
1. **Universal Thread-Safe `SettingsStore`** featuring schema validation, type clamping, background worker persistence, and atomic JSON persistence.
2. **Declarative Page Schema (`perfui.settings.json`)** with mandatory `"perfuiSchema": 1` marker, multi-page navigation, localization, and conditional logic (`visibleIf`, `enabledIf`).
3. **Unified `ModSettingsWindow`** featuring a fast mod category list, instant search, and full PSS styling.
4. **ABI-Stable C-API `IPerfUI_v4` and `IPerfUI_v5`** and Lua bindings for SKSE mod developers.
5. **Persistent Hotkey Manager** with real-time reactive rebinding and keybind conflict detection.

---

## 1. Storage Architecture (`SettingsStore`)

`SettingsStore` is a thread-safe singleton managing configuration schemas, memory caching, range clamping, and atomic disk writes.

### 1.1 Namespacing
Settings are strictly partitioned by unique `modId`:
```
SettingsStore
 ├── "SkyrimUpscaler"
 │    ├── "enabled"          -> bool
 │    ├── "renderScale"      -> float [50.0 .. 100.0]
 │    └── "hotkey"           -> keycode (uint32)
 └── "TrueDirectionalMovement"
      ├── "targetLockKey"    -> keycode (uint32)
      └── "cameraDistance"   -> float [100.0 .. 600.0]
```

### 1.2 Supported Data Types
* `bool` — check switches (`Toggle`).
* `int32_t` — discrete integer options.
* `float` — continuous floating-point sliders (`Slider`, `Stepper`).
* `std::string` — arbitrary text strings (`Input`, `Menu/Enum`).
* `Color` — RGBA colors (packed `uint32_t`).
* `KeyCode` (`uint32_t`) — keyboard, mouse, and gamepad virtual keycodes.

### 1.3 Range Clamping and Validation (`SettingDef`)
Each setting is described by a `SettingDef`:
```cpp
SettingDef def;
def.key = "renderScale";
def.type = SettingType::Float;
def.defaultValue = 100.0f;
def.minValue = 50.0f;
def.maxValue = 100.0f;
def.step = 5.0f;
def.formatString = "%.0f%%";
```
When mutators (`set*`) are invoked:
* Types are safely converted (e.g., numeric string to integer or hex color).
* Floating-point and integer numbers are clamped to `[minValue, maxValue]`.
* Stepped parameters snap to the nearest step increment.

### 1.4 Persistence, Background Worker & Crash Safety
* Each mod's configuration is saved to its own file:  
  `Data/SKSE/Plugins/PerfUI/Settings/<modId>.json`.
* **Atomic Writes:** Data is written to a temporary file `<modId>.json.tmp.PID_TID`, flushed, and atomically renamed via `std::filesystem::rename`. An unexpected game shutdown will never corrupt the active settings file.
* **Asynchronous Debounced Disk IO:** Rapid slider changes are batched in memory. When the 400 ms debounce timer elapses, JSON serialization and disk I/O are dispatched to a dedicated background worker thread (`std::jthread`) with write coalescing. The render thread remains completely unaffected (~0.012 ms dispatch vs 5–10 ms synchronous I/O).
* **Flush on Shutdown:** `flushAll()` ensures that all pending writes complete safely before game exit.
* **Corrupt File Recovery:** If a file contains invalid JSON, it is automatically backed up to `<modId>.json.bak`, an error is logged, and clean default values are restored without crashing the game.

---

## 2. Declarative Page Schema (`perfui.settings.json`)

Authors do not need C++ or Lua code to build menu pages: simply place `perfui.settings.json` inside your mod directory (`Data/SKSE/Plugins/<ModName>/perfui.settings.json` or `SKSE/Plugins/...`). PerfUI's `WidgetAutoLoader` automatically parses and adds it to the settings menu.

> [!IMPORTANT]
> **Mandatory Schema Marker:** The root JSON object **must** contain `"perfuiSchema": 1`.  
> Files named `config.json` or JSON files lacking `"perfuiSchema": 1` are intentionally skipped by the auto-loader to eliminate collisions with other plugins' internal configuration files.

### 2.1 Example `perfui.settings.json`
```json
{
  "perfuiSchema": 1,
  "modName": "MyDynamicHUD",
  "displayName": "$MOD_TITLE",
  "pages": [
    {
      "pageDisplayName": "$PAGE_GENERAL",
      "content": [
        {
          "type": "header",
          "text": "Display Settings"
        },
        {
          "type": "toggle",
          "id": "enableDynamicHud",
          "text": "Enable Status Bars",
          "help": "Shows or hides the status bar overlay.",
          "valueOptions": {
            "default": true
          }
        },
        {
          "type": "slider",
          "id": "hudScale",
          "text": "HUD Scale",
          "help": "Adjust HUD scaling factor.",
          "visibleIf": "enableDynamicHud",
          "valueOptions": {
            "min": 50.0,
            "max": 150.0,
            "step": 5.0,
            "default": 100.0,
            "formatString": "%.0f%%"
          }
        },
        {
          "type": "stepper",
          "id": "tickRate",
          "text": "Tick Rate",
          "visibleIf": "enableDynamicHud",
          "valueOptions": {
            "min": 15.0,
            "max": 60.0,
            "step": 15.0,
            "default": 30.0,
            "formatString": "%.0f Hz"
          }
        },
        {
          "type": "color",
          "id": "accentColor",
          "text": "Theme Accent Color",
          "valueOptions": {
            "default": "0x3399FFCC"
          }
        },
        {
          "type": "menu",
          "id": "anchorPreset",
          "text": "Screen Position",
          "valueOptions": {
            "default": "top_left",
            "options": [
              { "id": "top_left", "text": "Top Left" },
              { "id": "top_right", "text": "Top Right" },
              { "id": "bottom_center", "text": "Bottom Center" }
            ]
          }
        },
        {
          "type": "keymap",
          "id": "toggleKey",
          "text": "Toggle Hotkey",
          "help": "Key to toggle HUD visibility during combat.",
          "valueOptions": {
            "default": 65
          }
        },
        {
          "type": "input",
          "id": "profileName",
          "text": "Active Profile",
          "valueOptions": {
            "default": "Standard"
          }
        }
      ]
    }
  ]
}
```

### 2.2 Content Element Types
| Type | Description | Rendered Widget |
|---|---|---|
| `header` | Section title | `Text` with `settings-header` class |
| `text` | Descriptive or help text | `Text` with `settings-help` class |
| `empty` | Vertical layout spacer | `Panel` (12px height) |
| `toggle` | Boolean switch | `Checkbox` |
| `slider` | Continuous numeric bar | `Slider` with value label |
| `stepper` | Stepped numeric buttons | Increment/Decrement stepper |
| `menu` / `enum` | Dropdown option list | `ComboBox` |
| `color` | RGBA color picker | Color preview + hex editor |
| `keymap` | Keyboard/Mouse bind | `KeybindButton` with conflict badge |
| `input` | Single-line string editor | `TextInput` |

### 2.3 Conditional Visibility & Enablement
* `visibleIf: "keyName"` — visible only when setting `keyName` evaluates to true. Prefixing with `!` (`visibleIf: "!keyName"`) inverts the condition.
* `enabledIf: "keyName"` — enabled only when `keyName` is true; otherwise grayed out with the `.settings-disabled` class.
* `groupControl` / `groupCondition` — compatible grouping syntax.

### 2.4 Localization
Strings prefixed with `$` (e.g. `"$MOD_TITLE"`) are dynamically resolved against `Localization::Get()`. If missing, the `$` prefix is stripped and rendered verbatim.

---

## 3. PSS Styling Classes

Every UI element generated by the settings subsystem is tagged with semantic classes:

```css
/* Settings Page container */
.settings-page {
    padding: 16px;
    gap: 8px;
}

/* Individual setting row (label on left, control on right) */
.settings-row {
    height: 36px;
    flex-direction: row;
    align-items: center;
    justify-content: space-between;
    padding: 4px 8px;
    border-radius: 4px;
    transition: background-color 0.15s ease;
}

.settings-row:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

/* Section Header */
.settings-header {
    font-size: 16px;
    color: #e5c158;
    margin-top: 12px;
    margin-bottom: 4px;
}

/* Setting label */
.settings-label {
    font-size: 13px;
    color: #dcdcdc;
}

/* Control wrapper */
.settings-control {
    flex-direction: row;
    align-items: center;
    gap: 8px;
}

/* Help description */
.settings-help {
    font-size: 11px;
    color: #8a9ba8;
}

/* Row modified from default value */
.settings-row.settings-modified {
    border-left: 2px solid #e5c158;
}

/* Disabled row */
.settings-row.settings-disabled {
    opacity: 0.4;
}

/* Keybinding conflict alert */
.settings-conflict {
    color: #ff4d4f;
    border: 1px solid #ff4d4f;
}
```

---

## 4. Hotkeys & Conflict Resolution

### 4.1 Persistence & User Priority
When a plugin registers a hotkey:
```cpp
context.registerHotkey("MyMod:openMenu", 0x43 /* F9 */, []() { ... });
```
1. The hotkey is stored in `SettingsStore` under `MyMod` with default `0x43`.
2. Any user override previously saved takes immediate precedence over mod defaults.
3. When rebound in the UI, `UIContext` updates its active trigger key dynamically without requiring a game reload.

### 4.2 Conflict Detection
`ModSettingsManager` detects key collisions:
```cpp
auto conflicts = ModSettingsManager::GetSingleton().findKeybindConflicts("MyMod", "openMenu", keyCode);
for (const auto& c : conflicts) {
    std::cout << "Conflict detected with Mod: " << c.modId << ", Key: " << c.key << std::endl;
}

// Unbind conflicting keys to 0 (Unbound)
ModSettingsManager::GetSingleton().unbindConflictingKeybinds("MyMod", "openMenu", keyCode);
```

> [!NOTE]
> Currently, single virtual key codes (including standalone Shift, Ctrl, and Alt) are supported. Multi-key chord combinations will be added in a future input revision.

---

## 5. ABI-Stable C-API (`IPerfUI_v4` & `IPerfUI_v5`)

PerfUI provides a binary-stable C interface avoiding C++ standard library types (`std::string`, `std::vector`, `std::function`) across the DLL boundary.

### 5.1 ABI Versions
* **`IPerfUI_v4`** (376 bytes): Complete suite of settings management functions (`RegisterModSettingsJson`, `GetSetting*`, `SetSetting*`, `SubscribeSettingChanged`, `OpenSettings`).
* **`IPerfUI_v5`** (384 bytes): Backward-compatible extension adding `GetSettingStringCopy` for safe, caller-buffered string retrieval.
* Struct sizes and offsets are verified via `static_assert(sizeof(...) == ...)`.

### 5.2 String Lifetime Safety: `GetSettingString` vs `GetSettingStringCopy`
* **`GetSettingString(modId, key, fallback)`:**  
  Returns `const char*` pointing to a `thread_local` buffer. The string is valid in the calling thread until the next string setting retrieval call. Ideal for zero-allocation frame queries.
* **`GetSettingStringCopy(modId, key, buffer, bufferSize, fallback)`:**  
  Copies the string into a caller-supplied buffer with null-termination and returns bytes written. If `buffer == nullptr`, returns the required buffer capacity. Safe for multi-threading and persistent storage.
* Client C++ wrapper `PerfUI::Client::GetSettingString` returns an allocated `std::string`:
  ```cpp
  std::string profile = PerfUI::Client::GetSettingString("MyMod", "profileName", "Default");
  ```

### 5.3 SKSE Lifecycle, API Readiness (`IsApiReady`) & Rejection Caching
* Resolution is thread-safe, atomic, and **retries upon initial failure**: if a plugin queries PerfUI before `PerfUI.dll` loads, subsequent calls successfully retry and resolve the interface once loaded.
* **DLL Status Distinction:** When `PerfUI.dll` is loaded into the process but predates the requested version (e.g. older DLL lacking v4/v5), the refusal is cached permanently (`s_unsupportedV*`), preventing redundant queries and log spam.
* Signature `PerfUI::Client::IsApiReady(minVersion = InterfaceVersion_1)`:
  - Without arguments checks whether any base PerfUI API (v1+) is available.
  - To verify settings support, explicitly pass the version: `PerfUI::Client::IsApiReady(4)` (Settings API v4) or `PerfUI::Client::IsApiReady(5)` (String Copy v5).
* **Recommended Registration Hook:** Register mod settings inside `SKSE::MessagingInterface::kDataLoaded` or `kPostLoad`.

### 5.4 Subscription Threading & UI Thread Safety
* **External Mod Subscriptions (`SubscribeSettingChanged`):** Invoked **synchronously on the caller thread** executing `SetSetting*` per the C-API specification.
* **Internal Widget Subscriptions (`SettingsSection`):** Thread-safe. When mutated from background game threads, widget tree updates are enqueued into `m_pendingUpdates` and safely dispatched on the UI thread during the next `update(dt)`, eliminating races with rendering.
* **Safe Re-entrancy:** Inside a callback, you may:
  1. Mutate another setting via `SetSetting*` (deadlock-free; recursion for the same key is suppressed).
  2. Unsubscribe itself via `UnsubscribeSettingChanged(subId)`.
  3. Unsubscribe other listeners (cancelled listeners are immediately skipped in the active dispatch loop).

### 5.5 Third-Party Plugin Integration
```cpp
#include "PerfUI/PerfUIApi.h"

// 1. Verify API readiness
if (!PerfUI::Client::IsApiReady(4)) {
    // PerfUI not loaded yet
}

// 2. Register via JSON schema
PerfUI::Client::RegisterModSettingsJson(
    "TrueHud",
    "True HUD",
    "Data/SKSE/Plugins/TrueHUD/perfui.settings.json"
);

// 3. Query settings
bool isEnabled = PerfUI::Client::GetSettingBool("TrueHud", "enableBars", true);
float scale = PerfUI::Client::GetSettingFloat("TrueHud", "scale", 100.0f);
std::string profile = PerfUI::Client::GetSettingString("TrueHud", "profile", "Default");

// 4. Safe C-buffer copy (API v5)
char nameBuf[128];
PerfUI::Client::GetSettingStringCopy("TrueHud", "profile", nameBuf, sizeof(nameBuf), "Default");

// 5. Reactive subscription
uint64_t subId = PerfUI::Client::SubscribeSettingChanged("TrueHud", "scale", [](void* userData, const char* modId, const char* key) {
    float newScale = PerfUI::Client::GetSettingFloat(modId, key, 100.0f);
    ApplyScaleToGameHud(newScale);
}, nullptr);

// 6. Open Settings Window to your mod's page
PerfUI::Client::OpenSettings("TrueHud");
```

### 5.6 Lifecycle & Safe Shutdown (`shutdown`)
* `SettingsStore::shutdown()`: Idempotent graceful termination method.
* Flushes all pending and dirty changes to disk (`flushAll()`) and terminates worker thread with a 2-second timeout.
* During DLL detach (`DLL_PROCESS_DETACH`), `s_processDetaching` prevents blocking `join()` under the OS loader lock.
* After shutdown, the store falls back to **synchronous write mode**, ensuring late `set` calls persist to disk without data loss.

---

## 6. Lua Scripting API (`perfui.settings`) & Mod Isolation

```lua
-- Read setting with default fallback (reading other mods' settings is permitted)
local isHudEnabled = perfui.settings.get("MyMod", "enableDynamicHud", true)
local hudScale = perfui.settings.get("MyMod", "hudScale", 100.0)

-- Short form (modId defaults to the script host's own name)
local hudScaleShort = perfui.settings.get("hudScale", 100.0)

-- Subscribe to adjustments
perfui.settings.onChanged("MyMod", "hudScale", function(modId, key, newValue)
    print(string.format("Scale adjusted to: %.1f%%", newValue))
end)

-- Programmatic updates (STRICT MOD ISOLATION)
perfui.settings.set("MyMod", "hudScale", 120.0)
-- perfui.settings.set("OtherMod", "volume", 50.0) -- THROWS LUA ERROR: script 'MyMod' cannot modify settings of mod 'OtherMod'
```

### 6.1 Script Isolation Invariants
* **Read (`get`):** Allowed for any mod's settings (enabling cross-mod integration and compatibility checks).
* **Write (`set`):** Strictly restricted to the script's own mod (`modId == host->name()`). Writing to another mod raises a Lua error `settings access denied`.
* **Subscription (`onChanged`):** Allowed for own or foreign mods; automatically cleaned up on `ScriptHost` destruction.

---

## 7. Comparison with SkyUI MCM & MCM Helper

| Feature | SkyUI MCM | MCM Helper | PerfUI Settings Layer |
|---|---|---|---|
| **Runtime Engine** | Flash / Scaleform 42 | C++ + Scaleform UI | Native C++20 + DirectX 11 |
| **Script Overhead** | Heavy Papyrus Quest scripts | C++ backend, Scaleform UI | Zero (0 Papyrus scripts) |
| **Menu Open Latency** | 1.5 – 5.0 seconds (lag spikes) | ~1 second | **< 16 ms (instantaneous)** |
| **FPS in Menu** | 20 – 40 FPS (ActionScript cap) | 30 – 50 FPS | **60 – 144+ FPS (native refresh)** |
| **Styling Customization**| Impossible (hardcoded in SWF)| Impossible (hardcoded in SWF)| **Full PSS CSS-like styling** |
| **Conflict Handling** | None | Limited | Built-in with unbind resolution |
| **Schema Format** | Papyrus `OnConfigInit` code | `config.json` | Declarative JSON / C++ / Lua |
| **Dependencies** | SkyUI.esp, SKSE | SkyUI.esp, MCM Helper.dll | PerfUI.dll only |

---

## 8. SimpleJson Parser & Security Hardening

The embedded header-only parser `src/settings/SimpleJson.h` provides robust parsing and generation for manifests and settings:
* **Stack Overflow Prevention:** Recursion depth is strictly capped at 64 levels. Exceeding the limit fails gracefully with a syntax error.
* **UTF-16 Surrogate Pair Decoding:** Sequences such as `\uD83D\uDE00` are decoded to 4-byte UTF-8 emojis (e.g. 😀). Malformed or orphaned surrogates are rejected.
* **Strict RFC 8259 Numeric Parsing:** Disallows leading zeros (`012`), bare `NaN`, or `Infinity`. Validates exponential formats.
* **UTF-8 BOM Skipping:** Transparently handles files containing the `\xEF\xBB\xBF` byte sequence.
* **Duplicate Object Keys:** Follows standard *"last key wins"* semantics.
* **Diagnostic Reporting:** Error messages provide line numbers, column numbers, and byte positions.
* **Fuzz Tested:** Hardened and verified against 2,000 randomized and mutated payloads without crashes or memory corruption.
