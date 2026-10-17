# 🎨 PerfUI Styling & Component System

PerfUI provides a flexible, component-based design system that enables Skyrim SE/AE mod authors and standalone application developers to fully control their UI aesthetics without being constrained by any built-in theme.

---

## 🏛️ Core Architectural Principles

1. **Decoupled Style & Logic:** Component business logic, Flexbox layout, and visual presentation remain entirely separated.
2. **Any Visual Aesthetic:** Easily craft authentic Vanilla Skyrim, Dark Fantasy, Clean Minimalist, Futuristic/Sci-Fi, or custom UI designs.
3. **Deterministic Cascade Order:**
   1. **Local Override** — properties set directly on the element (`element->style().box.normal = ...`, `btn->normalColor(...)`).
   2. **Widget State Style** — state-specific rule (`Normal`, `Hovered`, `Pressed`, `Focused`, `Disabled`, `Selected`).
   3. **Component Theme Style** — component-type defaults from the active theme (`Theme::Current().components.<widget>`).
   4. **Theme Tokens** — global palette, metric, and typography design tokens (`Theme::Current().colors`, `metrics`, `typography`).
4. **Replaceable Themes:** Themes are purely demonstrative and can be modified or completely swapped with a single function call.

---

## 🎨 Theme Presets

`Theme` includes built-in presets accessible via `Theme::ApplyPreset("Name")` or `Theme::SetCurrent(Theme::...)`:

| Preset | Palette | Corner Radiuses | Aesthetic |
| :--- | :--- | :--- | :--- |
| **Nordic Fantasy** (default) | Nordic Gold (`#D4AF37`), Dark Slate (`#0D1117`) | `4px / 8px / 12px` | Modern fantasy UI |
| **Dark Fantasy** | Obsidian (`#0A080C`), Crimson Blood (`#B41E28`) | `3px / 6px / 10px` | Gothic, vampires, dark cults |
| **Minimalist** | Neutral Graphite (`#121214`), Indigo Accent (`#5A78DC`) | `2px / 4px / 6px` | Modern crisp flat UI |
| **Vanilla Skyrim** | Parchment, Dark Iron (`#121110`), Skyrim Gold (`#D8B964`) | `0px / 2px / 4px` | Authentic classic Skyrim |
| **Futuristic / SciFi** | Deep Cyber Black (`#060A12`), Neon Cyan (`#00E5FF`) | `0px / 3px / 6px` | High-tech & sci-fi interfaces |

---

## 🧱 New Components

### 1. `Divider`
Horizontal or vertical line separator.
```cpp
auto* div = panel->add<PerfUI::Divider>(PerfUI::LayoutDirection::Horizontal);
div->thickness(1.5f);
div->color(PerfUI::Color(80, 80, 90, 180));
```

### 2. `Spacer`
Fixed or flexible space-occupying element in Flexbox layouts.
```cpp
// Fixed 16px space
panel->add<PerfUI::Spacer>(16.0f);

// Expanding flex spacer
auto* flexSpacer = panel->add<PerfUI::Spacer>();
flexSpacer->flex(1.0f);
```

### 3. `Switch`
Interactive toggle switch with animated thumb and color transitions.
```cpp
auto* toggle = panel->add<PerfUI::Switch>(true);
toggle->onToggle([](bool checked) {
    SetOptionEnabled(checked);
});
```

### 4. `RadioButton` & `RadioGroup`
Mutually exclusive option selection container.
```cpp
auto* group = panel->add<PerfUI::RadioGroup>();
group->addOption(1, "Low Quality");
group->addOption(2, "Medium Quality");
group->addOption(3, "Ultra");
group->select(2);

group->onSelectionChanged([](int val) {
    ApplySetting(val);
});
```

### 5. `Label`
Semantic typography widget with preset styles:
- `LabelVariant::Heading`
- `LabelVariant::Body`
- `LabelVariant::Caption`
- `LabelVariant::Accent`

### 6. `Panel::clipChildren`
Hardware scissor-rect clipping for container children:
```cpp
panel->clipChildren(true);
```

---

## 📝 XML Markup Support

```xml
<Panel name="Settings" clip-children="true" bg="$surfaceElevated" corner-radius="12" padding="20">
    <Label text="Mod Settings" variant="heading" />
    <Divider thickness="1" color="$borderSubtle" />

    <Switch name="HardcoreMode" checked="true" />
    <Spacer size="12" />

    <RadioGroup name="Difficulty" selected="2">
        <RadioButton value="1" label="Easy" />
        <RadioButton value="2" label="Normal" />
        <RadioButton value="3" label="Hard" />
    </RadioGroup>
</Panel>
```
