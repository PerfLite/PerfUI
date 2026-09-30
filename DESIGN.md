# PerfUI Design System & API Specifications

## 1. Design Philosophy

PerfUI is designed to look and feel like a modern, premium AAA game interface while maintaining the atmospheric, Nordic fantasy spirit of Skyrim. It completely diverges from the utilitarian "developer tool" appearance of standard Dear ImGui.

### Key Visual Pillars
1. **Depth & Layering**: Subtle translucent backgrounds, soft drop shadows, and clean borders create a tactile hierarchy.
2. **Atmospheric Typography**: High legibility with distinct hierarchy (Titles, Headers, Body text, Subtitles, Meta tags).
3. **Fluid Micro-Interactions**: Smooth state transitions on hover, focus, press, and toggle.
4. **Gamepad-First Accessibility**: High contrast focus indicators, intuitive 2D directional navigation, and persistent button legends.

---

## 2. Design Tokens (Default Skyrim Theme)

### 2.1 Color Palette
```text
Surface:
  BackgroundBase:     #0D1117 (Alpha: 0.92) - Deep charcoal slate
  BackgroundElevated: #161B22 (Alpha: 0.85) - Card / panel fill
  BackgroundActive:   #21262D (Alpha: 0.95) - Pressed / active items

Borders & Dividers:
  BorderSubtle:       #30363D (Alpha: 0.60) - Delimiters
  BorderStrong:       #484F58 (Alpha: 0.80) - Card boundaries
  BorderFocus:        #D4AF37 (Alpha: 1.00) - Nordic Gold (active selection)

Typography:
  TextPrimary:        #F0F6FC - Crisp pearl white for titles and primary labels
  TextSecondary:      #8B949E - Muted silver for descriptions and inactive quests
  TextAccent:         #E5C07B - Warm Nordic amber / quest marker gold
  TextSuccess:        #7EE787 - Completed quest green
  TextDisabled:       #484F58 - Inactive / locked elements

Accents & Glows:
  FocusGlow:          rgba(212, 175, 55, 0.35) - Soft radiant glow on focused item
  SelectionFill:      rgba(212, 175, 55, 0.15) - Translucent gold highlight
```

### 2.2 Metrics & Spacing
- **Base Grid**: 4px standard grid.
- **Spacing Units**:
  - `SpaceXS` = 4px
  - `SpaceSM` = 8px
  - `SpaceMD` = 16px
  - `SpaceLG` = 24px
  - `SpaceXL` = 32px
- **Border Radii**:
  - `RadiusSM` = 4px (Buttons, badges)
  - `RadiusMD` = 8px (Cards, popups)
  - `RadiusLG` = 12px (Windows, large dialogs)

---

## 3. Public API Ergonomics

PerfUI prioritizes a clean, expressive C++20 fluent API. Developers build trees in a retained structure with type-safe method chaining.

### 3.1 Creating a Window and Components
```cpp
#include <PerfUI/PerfUI.h>

void SetupUI(PerfUI::UIContext& ctx)
{
    auto window = ctx.createWindow("JournalWindow", {
        .title = "Journal",
        .width = PerfUI::Size::Percent(80.0f),
        .height = PerfUI::Size::Percent(75.0f),
        .anchor = PerfUI::Anchor::Center
    });

    // Root horizontal layout: Sidebar (Left) + Details (Right)
    auto content = window->add<PerfUI::Panel>();
    content->layout()
        .direction(PerfUI::LayoutDirection::Horizontal)
        .gap(16.0f)
        .padding(16.0f);

    // Left Sidebar: Quests List
    auto sidebar = content->add<PerfUI::Panel>();
    sidebar->layout()
        .width(PerfUI::Size::Fixed(320.0f))
        .direction(PerfUI::LayoutDirection::Vertical)
        .gap(8.0f);

    sidebar->add<PerfUI::Text>("ACTIVE QUESTS")
        .style().fontSize(18).color(PerfUI::Color::NordicGold);

    auto questList = sidebar->add<PerfUI::ScrollView>();
    questList->layout().flex(1.0f); // Fill remaining vertical space

    for (const auto& quest : quests) {
        auto item = questList->add<PerfUI::Button>(quest.name);
        item->onClick([quest]() {
            SelectQuest(quest);
        });
    }

    // Right Content: Quest Details
    auto details = content->add<PerfUI::Panel>();
    details->layout()
        .flex(1.0f)
        .direction(PerfUI::LayoutDirection::Vertical)
        .gap(12.0f);

    details->add<PerfUI::Text>("The Way of the Voice")
        .style().fontSize(24).bold(true);
}
```

### 3.2 Custom Modder Widgets
Third-party mods can create encapsulated, reusable widgets simply by inheriting from `PerfUI::Widget`:

```cpp
class QuestCard : public PerfUI::Widget {
public:
    QuestCard(const std::string& title, bool isCompleted) {
        layout()
            .direction(PerfUI::LayoutDirection::Horizontal)
            .padding(12.0f)
            .gap(8.0f);

        m_icon = add<PerfUI::Image>(isCompleted ? "checked_icon" : "active_icon");
        m_label = add<PerfUI::Text>(title);
    }

    void setCompleted(bool completed) {
        m_label->style().color(completed ? PerfUI::Color::MutedGray : PerfUI::Color::White);
    }

private:
    PerfUI::Image* m_icon{ nullptr };
    PerfUI::Text* m_label{ nullptr };
};
```

---

## 4. Gamepad & Controller Interaction Model

Skyrim menus must be effortless to operate without touching the mouse:

### 4.1 Input Mapping
| Gamepad (XInput) | Keyboard | Action |
| :--- | :--- | :--- |
| **D-Pad / Left Stick** | **Arrow Keys** | Spatial 2D directional navigation |
| **A Button** | **Enter / Space** | Submit / Activate / Primary Click |
| **B Button** | **Escape** | Cancel / Back / Close Window |
| **X Button** | **R** | Secondary Action (e.g. Set Active Quest) |
| **Y Button** | **F** | Tertiary Action (e.g. Show on Map) |
| **LB / RB** | **Q / E** | Tab Switcher (Previous / Next Tab) |
| **Right Stick** | **Page Up / Down** | Direct Scrolling in active ScrollView |

### 4.2 Spatial Focus Navigation Algorithm
1. The focused element holds an axis-aligned bounding box (AABB) in screen space.
2. When a directional input is triggered (e.g. `NavDirection::Right`):
   - PerfUI collects all enabled, focusable elements in the active window.
   - For each candidate, it calculates a directional distance metric:
     $$\text{Score} = \text{PrimaryDistance} \times 1.0 + \text{OrthogonalDistance} \times 2.0$$
   - The element with the minimum score in the target half-plane receives focus.
3. When focused, the element triggers a smooth animated focus ring and scrolls into view if inside a `ScrollView`.
