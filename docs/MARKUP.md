# PerfUI Markup Specification & Reference

[English](MARKUP_EN.md) | [Русский](MARKUP.md)

PerfUI Markup is a declarative, runtime XML user interface format designed for high performance, ease of authoring, and instant **hot-reloading without restarting or recompiling**.

Modders and developers can define rich UI hierarchies in clean `.xml` files, bind C++ callbacks by name, style widgets with design tokens, and iterate on interfaces live in-game or in the desktop sandbox.

---

## ⚡ Quick Start

### 1. Define UI in XML (`settings.xml`)
```xml
<UI>
  <Panel name="MainSettings" direction="column" width="420" padding="20" gap="12">
    <Text text="Audio & Gameplay Settings" font="title" color="$borderFocus"/>

    <Slider name="MasterVolume" label="Master Volume" min="0" max="100" value="75" bind="settings.audio.volume"/>
    <Checkbox name="ShowProfiler" label="Show Retained Profiler" checked="true"/>

    <Panel direction="row" gap="10" justify="end">
      <Button name="BtnCancel" text="Cancel" onClick="onCancel"/>
      <Button name="BtnSave" text="Save Changes" onClick="onSave" class="primary"/>
    </Panel>
  </Panel>
</UI>
```

### 2. Load and Bind in C++
```cpp
#include <PerfUI/PerfUI.h>
#include <PerfUI/Markup/MarkupLoader.h>

void SetupMyMenu(PerfUI::UIContext& context) {
    PerfUI::MarkupLoader loader(context);

    // Bind event callbacks by name
    loader.bindCallback("onCancel", [&]() {
        context.showToast("Settings", "Cancelled", PerfUI::ToastType::Info);
    });

    loader.bindCallback("onSave", [&]() {
        context.showToast("Settings", "Changes saved!", PerfUI::ToastType::Success);
    });

    // Enable live hot-reload
    loader.enableHotReload(true);

    // Load file into context root (or custom parent panel)
    PerfUI::UIElement* menu = loader.loadFile("Data/Interface/PerfUI/settings.xml");

    // Retrieve widgets for dynamic manipulation
    auto* volumeSlider = dynamic_cast<PerfUI::Slider*>(loader.findByName("MasterVolume"));
}
```

### 3. Update Every Frame (Hot-Reload Loop)
```cpp
void OnFrameUpdate(float dt) {
    loader.poll(); // Checks file modification timestamp every ~500ms
    context.update(dt);
}
```

---

## 📐 Common Layout Attributes (All Tags)

All widget tags support the flexbox and box-model layout properties declared in `LayoutProps`:

| Attribute | Accepted Values | Description & Equivalent C++ Method |
| :--- | :--- | :--- |
| `name` | string | Identifier for `findByName` lookup and state preservation |
| `direction` | `row`, `column` (or `horizontal`, `vertical`) | Flex layout axis direction (`direction(...)`) |
| `align` | `start`, `center`, `end`, `stretch` | Cross-axis item alignment (`alignment(...)`) |
| `justify` | `start`, `center`, `end`, `space-between`, `space-around` | Main-axis distribution (`justify(...)`) |
| `width` | `120`, `50%`, `auto`, `flex`, `flex:2` | Width dimension constraint (`width(...)`) |
| `height` | `40`, `100%`, `auto`, `flex`, `flex:1.5` | Height dimension constraint (`height(...)`) |
| `flex` | `1`, `2.5` (number) | Flex grow weight (`flex(...)`) |
| `padding` | `8`, `8 12`, `1 2 3 4` | Internal insets: uniform, horizontal/vertical, or left/top/right/bottom |
| `margin` | `4`, `4 8`, `1 2 3 4` | External insets: uniform, horizontal/vertical, or left/top/right/bottom |
| `gap` | `8`, `$spacingSM` | Spacing between sibling children (`gap(...)`) |
| `min-width` | number | Minimum width boundary |
| `max-width` | number | Maximum width boundary |
| `min-height` | number | Minimum height boundary |
| `max-height` | number | Maximum height boundary |
| `visible` | `true`, `false`, `1`, `0` | Element visibility (`setVisible(...)`) |
| `enabled` | `true`, `false`, `1`, `0` | Element enabled state (`setEnabled(...)`) |
| `focusable` | `true`, `false`, `1`, `0` | Controller / keyboard navigation focus (`setFocusable(...)`) |
| `tooltip` | string | Floating hover tooltip text (`tooltip(...)`) |
| `class` | `primary danger` (space-separated) | CSS-like class tags for stylesheet targeting |
| `bind` | `settings.volume` | Data-binding key path (v1 stub) |

---

## 🎨 Theme Tokens & Values

Numbers and colors support Nordic Skyrim theme tokens from `Theme.h`:

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
- Hex: `#RRGGBB` (e.g. `#D4AF37`), `#RRGGBBAA` (e.g. `#161B22EA`), `#RGB`, `#RGBA`
- Theme Tokens:
  - `$borderFocus`, `$nordicGold`, `$accent` (`#D4AF37` / Gold)
  - `$surfaceBase` (`#0D1117EB`)
  - `$surfaceElevated` (`#161B22DC`)
  - `$surfaceActive` (`#21262DF5`)
  - `$borderSubtle`, `$borderStrong`
  - `$textPrimary`, `$textSecondary`, `$textMuted`
  - `$white`, `$black`, `$transparent`

---

## 🧩 Built-In Widgets (v1)

### `<Panel>`
Flexible container for nesting child elements with optional background, border, and drop shadow.
```xml
<Panel name="Card" bg="$surfaceElevated" border="$borderSubtle" border-width="1" radius="8" shadow="true" padding="16">
  <!-- Children -->
</Panel>
```

### `<Text>`
Formatted UTF-8 typography with Cyrillic support and style attributes.
```xml
<Text text="Dragonborn Journal" font="$fontTitle" color="$nordicGold" bold="true" italic="false" wrap="true"/>
```

### `<Button>`
Interactive clickable and focusable button.
```xml
<Button name="BtnAccept" text="Accept Quest" onClick="onAccept" font="14" radius="6" class="primary"/>
```

### `<ScrollView>`
Scrollable viewport with automatic scrollbar and mousewheel support.
```xml
<ScrollView name="QuestList" scrollSpeed="36" showScrollbar="true" offset="0">
  <!-- Content elements of any height -->
</ScrollView>
```

### `<Checkbox>`
Toggle control with smooth checkmark animation.
```xml
<Checkbox name="SoundToggle" label="Enable Spatial Audio" checked="true" fontSize="14" checkColor="$nordicGold"/>
```

### `<ProgressBar>`
Fluid visual progress bar with easing animation and percentage readout.
```xml
<ProgressBar name="HealthBar" value="0.75" min="0" max="1" height="16" fillColor="#E54D2E" trackColor="#1F1515"/>
```

### `<Slider>`
Numeric range slider with drag handle and value tracking.
```xml
<Slider name="FOV" label="Field of View" min="70" max="110" value="90" step="1"/>
```

### `<Image>`
DirectX 11 texture display widget with color tinting.
```xml
<Image texture="1001" tint="$white" width="64" height="64"/>
```

### `<TabBar>`
Tab strip navigation bar with automatic active tab indicator.
```xml
<TabBar name="MenuTabs" tabs="Active Quests, Completed Quests, Player Stats" selectedTab="0"/>
```

### `<ComboBox>`
Dropdown selection menu with animated overlay.
```xml
<ComboBox name="ResolutionSelect" options="1920x1080, 2560x1440, 3840x2160" selectedIndex="0"/>
```

### `<TextInput>`
Editable text field with cursor blinking, selection, and keyboard focus.
```xml
<TextInput name="SearchBox" placeholder="Filter inventory..." text="" fontSize="13"/>
```

---

## 🔥 Hot-Reload Details

When `loader.enableHotReload(true)` is activated:
1. `MarkupLoader::poll()` monitors the XML file's `std::filesystem::last_write_time` on the main thread (~500ms throttle).
2. Upon file modification, the new XML is parsed into a detached candidate tree.
3. If parsing fails, the current in-game tree is **preserved intact**, and a toast notification (`showToast`) reports the syntax error and line number.
4. If parsing succeeds, state is automatically carried over:
   - **Focus:** The focused widget's name is saved and restored on the reloaded tree.
   - **Scroll Position:** `ScrollView` offsets are preserved by element name.
   - **Tab Navigation:** `TabBar` active tabs are preserved by element name.
5. The old subtree is destroyed and the new subtree is atomically attached without disrupting game state.

---

## 🛠️ Registering Custom Modder Widgets

Third-party modders can register custom widgets in one line via `WidgetRegistry`:

```cpp
#include <PerfUI/Markup/WidgetRegistry.h>

void RegisterCustomWidgets() {
    PerfUI::WidgetRegistry::instance().registerWidget("CompassHud", [](PerfUI::UIElement* parent, const PerfUI::AttrMap& attrs) {
        auto hud = std::make_unique<MyCustomCompassWidget>();
        // Process custom attributes from attrs
        if (parent) return parent->addChild(std::move(hud));
        return hud.release();
    });
}
```

Now `<CompassHud showEnemies="true"/>` can be used directly in any XML layout!
