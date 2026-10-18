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

### 7. `ItemSlot`
Inventory slot widget designed for items, armor, weapons, and spells:
- Full `ItemRarity` support (`Common`, `Fine`, `Superior`, `Epic`, `Legendary`) with corresponding border & background palettes.
- Stack count badge with shadow.
- Equipped diamond badge indicator.
- Click, hover, and interaction callbacks.
```cpp
auto* slot = grid->add<PerfUI::ItemSlot>(52.0f);
slot->setItem("textures/items/ebony_sword.png", 1, PerfUI::ItemRarity::Epic);
slot->setEquipped(true);
slot->onClick([] {
    EquipItem();
});
```

### 8. `KeybindSelector`
Interactive key binding selector for mod configuration menus (MCM):
- Listening state with pulsing prompt ("Press any key...").
- Cancellation on `Esc` key or clicking outside.
- Virtual Key code to human-readable string conversion (`KeyCodeToString`).
- Callback `onKeyChanged(uint32_t newKey)`.
```cpp
auto* binder = panel->add<PerfUI::KeybindSelector>(VK_F4);
binder->onKeyChanged([](uint32_t key) {
    SaveHotkey(key);
});
```

### 9. `Accordion` & `AccordionSection`
Collapsible container with smooth animated unfolding/collapsing transitions:
- Animated height interpolation with hardware Scissor rect clipping.
- Arrow indicator chevron (`▼` / `►`).
- Optional `exclusive(true)` accordion mode where opening one section folds others.
```cpp
auto* accordion = panel->add<PerfUI::Accordion>();
accordion->exclusive(true);

auto* sec1 = accordion->addSection("General", true);
sec1->content()->add<PerfUI::Switch>(true);

auto* sec2 = accordion->addSection("Advanced");
sec2->content()->add<PerfUI::Button>("Reset Defaults");
```

### 10. `SearchableList`
Fast real-time filtering list with an integrated search box:
- Built-in `TextInput` header and `ScrollView`.
- Case-insensitive substring filtering.
- Item selection callback with hover and selection highlighting.
```cpp
auto* list = panel->add<PerfUI::SearchableList>();
list->setItems({ "Ebony Sword", "Daedric Battleaxe", "Iron Dagger", "Glass Bow" });
list->onItemSelected([](size_t idx, const std::string& item) {
    SelectItem(item);
});
```

### 11. `TextArea`
Multiline scrollable text block:
- Automatic line wrapping and mouse wheel scrolling.
- Hardware Scissor-rect clipping.
- Integrated scrollbar rendering.
```cpp
auto* textLog = panel->add<PerfUI::TextArea>("Initial log line...\nNext line...");
textLog->append("\nOperation succeeded.");
```

### 12. `SegmentedControl`
Horizontal pill segmented control with smoothly animated sliding thumb indicator:
- Physical sliding animation (`AnimatedFloat`).
- Segment dividers between inactive items.
- Values and labels support with `onSelectionChanged(index, text)` callback.
```cpp
auto* filter = panel->add<PerfUI::SegmentedControl>();
filter->addSegment("All", "all")
      .addSegment("Weapons", "wpn")
      .addSegment("Armor", "arm")
      .addSegment("Potions", "pot");
filter->onSelectionChanged([](size_t index, const std::string& label) {
    FilterInventory(label);
});
```

### 13. `CategoryList`
Vertical category selector designed for MCM (Mod Configuration Menus) and navigation menus:
- Active left accent indicator bar (`borderFocus`).
- Support for badges (e.g. "NEW", "3") and icons.
- Callback `onCategoryChanged(id)`.
```cpp
auto* catList = panel->add<PerfUI::CategoryList>();
catList->addCategory("general", "General Settings")
       .addCategory("graphics", "Graphics", "", "NEW")
       .addCategory("audio", "Audio");
catList->onCategoryChanged([](const std::string& catId) {
    LoadSettingsPage(catId);
});
```

### 14. `QuestTracker`
In-game Skyrim quest objective HUD tracker and journal component:
- Quest title with Skyrim-style diamond quest marker.
- Objectives list with progress counters `(current/max)`, checkmarks, and `(Optional)` flags.
- Interactive clicks with `onObjectiveClicked(idx)`.
```cpp
auto* tracker = hud->add<PerfUI::QuestTracker>();
tracker->setQuestTitle("The Way of the Voice")
       .setQuestCategory("Main Quest")
       .addObjective("Speak to the Greybeards")
       .addObjective("Demonstrate Unrelenting Force", false, 0, 3)
       .addObjective("Listen to Arngeir", false, 0, 0, true);
```

### 15. `Notification`
Rich notification banner with progress timer and interactive action:
- Notification levels: `NotificationLevel::Info`, `Success`, `Warning`, `Error`.
- Accent color coding and remaining time progress bar.
- Close button `[X]` and customizable action button.
```cpp
auto* notif = panel->add<PerfUI::Notification>();
notif->setTitle("Level Up!")
     .setMessage("Destruction skill reached level 20.")
     .setLevel(PerfUI::NotificationLevel::Success)
     .setAction("View Perks", [] { OpenPerkTree(); })
     .setDuration(5.0f);
```

### 16. `Sidebar`
Collapsible side navigation drawer:
- Animated width transitions between compact (icons-only) and expanded states.
- Header with toggle button (`<<` / `>>`), navigation items with badge counters.
- Full integration with active theme styling.
```cpp
auto* sidebar = panel->add<PerfUI::Sidebar>();
sidebar->setTitle("Skyrim UI")
       .addItem("dashboard", "Dashboard")
       .addItem("inventory", "Inventory", "", "12")
       .addItem("spells", "Spells");
sidebar->onItemSelected([](const std::string& id) {
    SwitchView(id);
});
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
