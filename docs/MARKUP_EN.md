# PerfUI Markup Specification & Reference (English)

[English](MARKUP_EN.md) | [Русский](MARKUP.md)

**PerfUI Markup** (`PerfUI_Markup`) is a declarative, runtime XML user interface subsystem designed for high performance, authoring clarity, and **instant hot-reloading without recompilation or game restarts**.

Modders and developers can define rich UI hierarchies in clean `.xml` files, bind C++ callbacks by name, style widgets with design tokens, and iterate on interfaces live in-game or within the standalone desktop sandbox.

---

## ⚡ Quick Start

### 1. Declare UI in XML (`settings.xml`)
```xml
<UI>
  <Panel name="MainSettings" direction="column" width="420" padding="20" gap="12">
    <Text text="Audio & Gameplay Settings" font="title" color="$borderFocus"/>

    <Slider name="MasterVolume" label="Master Volume" min="0" max="100" value="75"/>
    <Checkbox name="ShowProfiler" label="Show Retained Profiler" checked="true"/>

    <Panel direction="row" gap="10" justify="end">
      <Button name="BtnCancel" text="Cancel" onClick="onCancel"/>
      <Button name="BtnSave" text="Save Changes" onClick="onSave" class="primary"/>
    </Panel>
  </Panel>
</UI>
```

### 2. Load and Bind Callbacks in C++
```cpp
#include <PerfUI/PerfUI.h>
#include <PerfUI/Markup/MarkupLoader.h>

void SetupMyMenu(PerfUI::UIContext& context) {
    PerfUI::MarkupLoader loader(context);

    // Bind event callbacks by name
    loader.bindCallback("onCancel", [&]() {
        context.showToast("Settings", "Operation cancelled", PerfUI::ToastType::Info);
    });

    loader.bindCallback("onSave", [&]() {
        context.showToast("Settings", "Changes successfully saved!", PerfUI::ToastType::Success);
    });

    // Enable file monitoring hot-reload
    loader.enableHotReload(true);

    // Load file into context root (or a custom parent element)
    PerfUI::UIElement* menu = loader.loadFile("Data/Interface/PerfUI/settings.xml");

    // Query elements dynamically by name
    auto* volumeSlider = dynamic_cast<PerfUI::Slider*>(loader.findByName("MasterVolume"));
}
```

### 3. Update Every Frame (Hot-Reload Polling Loop)
```cpp
void OnFrameUpdate(float dt) {
    loader.poll(); // Checks file modification timestamp every ~500ms
    context.update(dt);
}
```
If your mod stores element pointers, use `loader.onBeforeReload(...)` to save state and `loader.onAfterReload(...)` to look up new elements and reconnect callbacks. Old pointers are invalid after a reload.

---

## 📐 Common Layout Attributes (Supported by All Tags)

All widget tags support the flexbox and box-model layout properties declared in `LayoutProps`:

| Attribute | Accepted Values | Description & Equivalent C++ Method |
| :--- | :--- | :--- |
| `name` | string | Unique name for `findByName` lookup and state preservation |
| `direction` | `row`, `column` (or `horizontal`, `vertical`) | Flex layout direction axis (`direction(...)`) |
| `align` | `start`, `center`, `end`, `stretch` | Cross-axis alignment (`alignment(...)`) |
| `justify` | `start`, `center`, `end`, `space-between`, `space-around` | Main-axis distribution (`justify(...)`) |
| `width` | `120`, `50%`, `auto`, `flex`, `flex:2` | Width dimension constraint (`width(...)`) |
| `height` | `40`, `100%`, `auto`, `flex`, `flex:1.5` | Height dimension constraint (`height(...)`) |
| `flex` | `1`, `2.5` (number) | Flex grow/shrink weight (`flex(...)`) |
| `padding` | `8`, `8 12`, `1 2 3 4` | Internal insets: uniform, horizontal/vertical, or left/top/right/bottom |
| `margin` | `4`, `4 8`, `1 2 3 4` | External insets: uniform, horizontal/vertical, or left/top/right/bottom |
| `gap` | `8`, `$spacingSM` | Spacing between sibling children (`gap(...)`) |
| `min-width` | number | Minimum width boundary |
| `max-width` | number | Maximum width boundary |
| `min-height` | number | Minimum height boundary |
| `max-height` | number | Maximum height boundary |
| `visible` | `true`, `false`, `1`, `0` | Element visibility (`setVisible(...)`) |
| `enabled` | `true`, `false`, `1`, `0` | Element interactive enabled state (`setEnabled(...)`) |
| `focusable` | `true`, `false`, `1`, `0` | Gamepad / keyboard navigation focus (`setFocusable(...)`) |
| `tooltip` | string | Hover tooltip message (`tooltip(...)`) |
| `class` | `primary danger` (space-separated) | CSS-like class tags for styling and querying (`classes(...)`) |
| `bind` | `player.stats` | One-way Text binding updated through `MarkupLoader::setBinding` (`<Text>` only) |
| `position` | `absolute` | Removes the element from flex layout |
| `left`, `right`, `top`, `bottom` | pixels | Absolute offsets from the parent edges |
| `center-x` | `true`, `false` | Centers an absolute element horizontally |

---

## 🎨 Design Tokens & Theme Values

Numeric measurements and colors support Nordic Skyrim theme tokens from `Theme.h`:

### Spacing & Metrics
- `$spacingXS` = `4.0`
- `$spacingSM` = `8.0`
- `$spacingMD` = `16.0`
- `$spacingLG` = `24.0`
- `$spacingXL` = `32.0`
- `$radiusSmall` = `4.0`
- `$radiusMedium` = `8.0`
- `$radiusLarge` = `12.0`

### Typography
- `$fontSmall` / `small` = `12.0`
- `$fontBody` / `body` = `14.0`
- `$fontMedium` / `medium` = `16.0`
- `$fontTitle` / `title` = `20.0`
- `$fontHeader` / `header` = `26.0`

### Colors
- Hex Formats: `#RRGGBB` (e.g. `#D4AF37`), `#RRGGBBAA` (e.g. `#161B22EA`), `#RGB`, `#RGBA`
- Theme Tokens:
  - `$borderFocus`, `$nordicGold`, `$accent` (`#D4AF37` / Nordic Gold)
  - `$surfaceBase` (`#0D1117EB`)
  - `$surfaceElevated` (`#161B22DC`)
  - `$surfaceActive` (`#21262DF5`)
  - `$borderSubtle`, `$borderStrong`
  - `$textPrimary`, `$textSecondary`, `$textMuted`
  - `$white`, `$black`, `$transparent`

---

## 🧩 Built-In Widget Catalog (v1)

### `<Panel>`
Retained container with background color, borders, corner rounding, and drop shadows:
```xml
<Panel name="Card" bg="$surfaceElevated" border="$borderSubtle" border-width="1" radius="8" shadow="true" padding="16">
  <!-- Child widgets -->
</Panel>
```
`bg-image="frame.png"` loads an image beside the XML file. `bg-uv="0 0 1 1"` selects a source region. Use `bg-slice="150 140 150 140"` for source border pixels and `bg-corner="32 30 32 30"` for rendered corner sizes in a nine-slice frame.

### `<Text>`
Formatted vector typography with Cyrillic glyph support and text wrapping:
```xml
<Text text="Dragonborn Journal" font="$fontTitle" color="$nordicGold" bold="true" italic="false" wrap="true"/>
```
For live text, use `<Text bind="player.stats" text="Loading..."/>` and call `loader.setBinding("player.stats", value)`. Values survive XML hot reload.

### `<Button>`
Interactive clickable and focusable button with hover transitions:
```xml
<Button name="BtnAccept" text="Accept Quest" onClick="onAccept" font="14" radius="6" class="primary"/>
```

### `<ScrollView>`
Scrollable viewport with automatic scrollbar and mousewheel navigation:
```xml
<ScrollView name="QuestList" scrollSpeed="36" showScrollbar="true" offset="0">
  <!-- Content elements of arbitrary dimensions -->
</ScrollView>
```

### `<Checkbox>`
Boolean toggle switch with smooth animated state transitions:
```xml
<Checkbox name="SoundToggle" label="Enable Spatial Audio" checked="true" fontSize="14" checkColor="$nordicGold"/>
```

### `<ProgressBar>`
Visual percentage gauge with dynamic easing curves:
```xml
<ProgressBar name="HealthBar" value="0.75" min="0" max="1" height="16" fillColor="#E54D2E" trackColor="#1F1515"/>
```

### `<Slider>`
Continuous or stepped numeric range slider with drag handle:
```xml
<Slider name="FOV" label="Field of View" min="70" max="110" value="90" step="1"/>
```

### `<Image>`
DirectX 11 texture display widget supporting disk files, UV coordinates, and offscreen 3D Render Target previews.
```xml
<!-- File-backed texture -->
<Image src="Data/Textures/MyMod/icon.png" width="64" height="64"/>

<!-- External D3D11 Render Target with 3D item preview and vertical flip -->
<Image texture="1001" uv="0 0 1 1" flip-y="true" width="256" height="256"/>
```

### `<WorldAnchor>`
3D anchor widget projecting UI hierarchies into screen coordinates based on Skyrim target/actor positions. Supports distance scaling, alpha fading, screen edge clamping, and behind-camera culling.
```xml
<WorldAnchor targetId="42" align="bottom-center" offset="0 -60"
             distanceScaling="true" minDist="100" maxDist="3500" minScale="0.4" maxScale="1.0"
             distanceAlpha="true" minAlpha="0.0" maxAlpha="1.0"
             edgeClamping="true" screenMargin="40">
  <!-- Nested elements follow the 3D target in the game world -->
  <ProgressBar name="BossHp" value="0.8" width="120" height="12"/>
  <Text name="BossName" text="$BOSS_NAME" color="$nordicGold"/>
</WorldAnchor>
```

### `<TabBar>`
Horizontal tab navigation strip with active tab indicators:
```xml
<TabBar name="MenuTabs" tabs="Active Quests, Completed Quests, Player Stats" selectedTab="0"/>
```

### `<ComboBox>`
Dropdown selection menu with viewport auto-clamping:
```xml
<ComboBox name="ResolutionSelect" options="1920x1080, 2560x1440, 3840x2160" selectedIndex="0"/>
```

### `<TextInput>`
Editable text field with cursor blinking, selection, and keyboard focus:
```xml
<TextInput name="SearchBox" placeholder="Filter inventory..." text="" fontSize="13"/>
```

---

## 🔥 Hot-Reload Mechanism

When `loader.enableHotReload(true)` is enabled:
1. `MarkupLoader::poll()` monitors the XML file's `std::filesystem::last_write_time` on the main thread (~500ms intervals, non-blocking).
2. When the file is saved, the new XML is parsed into an isolated candidate tree.
3. If parsing fails, the current in-game UI is **kept intact without crashing**, and an error Toast displays the exact line number of the syntax error.
4. If parsing succeeds, state is automatically migrated to the new tree:
   - **Keyboard & Gamepad Focus:** The focused element's name is remembered and restored on the newly loaded widget.
   - **Scroll Position:** `ScrollView` scroll offsets are preserved matching element names.
   - **Tab Navigation:** `TabBar` active tab indices are restored matching element names.
5. The obsolete hierarchy is safely destroyed and replaced atomically.

---

## 📜 Lua Script Binding (`<UI script="...">`)

Markup supports zero-C++ event logic when paired with the `PerfUI_Lua` module. By specifying the `script` attribute on `<UI>`, events like `onClick`, `onToggle`, and `onValueChanged` are resolved directly into Lua functions via `ScriptCallbackResolver`:

```xml
<UI script="main.lua">
  <Panel direction="column" gap="10">
    <Button name="BtnAccept" text="Accept" onClick="onAccept"/>
    <Checkbox name="MuteAudio" label="Mute Audio" onToggle="onMuteToggled"/>
  </Panel>
</UI>
```

In `main.lua`:
```lua
function onAccept()
    ui.toast("Accepted!", "success")
end

function onMuteToggled(checked)
    log.info("Mute state: " .. tostring(checked))
end
```

See [`docs/LUA_EN.md`](LUA_EN.md) for full scripting details, quotas, and element handles.

---

## 🌐 Multilingual Localization (`$KEY`)

Textual attributes in XML (`text`, `label`, `placeholder`, `tooltip`, `title`, `message`) automatically resolve translations. When an attribute value begins with `$`, the parser looks up the key in `LocalizationService`:

```xml
<Panel gap="12">
  <Text text="$MENU_HEADER"/>
  <Button text="$BTN_SAVE" tooltip="$TIP_SAVE"/>
  <TextInput placeholder="$SEARCH_PLACEHOLDER"/>
</Panel>
```

Supports JSON dictionaries (`lang/en.json`), standard INI key-value (`KEY=Value`), and native Skyrim SKSE translation files (`$KEY\tValue`). Switching languages live immediately updates all displayed text.

---

## 🛠️ Registering Custom Modder Widgets in C++

Third-party modders can register custom XML tags in one call via `WidgetRegistry`:

```cpp
#include <PerfUI/Markup/WidgetRegistry.h>

void RegisterCustomWidgets() {
    PerfUI::WidgetRegistry::instance().registerWidget("CompassHud", [](PerfUI::UIElement* parent, const PerfUI::AttrMap& attrs) {
        auto hud = std::make_unique<MyCustomCompassWidget>();
        // Parse custom attributes from attrs
        if (parent) return parent->addChild(std::move(hud));
        return hud.release();
    });
}
```

Now `<CompassHud showEnemies="true"/>` can be used directly in any XML layout!
