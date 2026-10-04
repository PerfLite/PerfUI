# PerfUI Modder Guide (Developer Reference Manual)

[English](MODDER_GUIDE_EN.md) | [Русский](MODDER_GUIDE.md)

Welcome to **PerfUI** — the high-performance, independent, retained-mode C++20 user interface framework tailored specifically for *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition* (SKSE64 / CommonLibSSE) and DirectX 11 applications.

PerfUI eliminates the burden of writing fragile DirectX 11 hooks, wrestling with raw Dear ImGui immediate-mode boilerplate, calculating pixel offsets manually, or reverse engineering Flash/Scaleform `.swf` files.

---

## 1. Architectural Philosophy: "Plug, Link & Play"

### For Players (Runtime):
- A single file **`PerfUI.dll`** located in `Data/SKSE/Plugins/` (installed once via Mod Organizer 2 or Vortex).

### For Mod Developers (Compile-Time / SDK):
1. **Public Header Files:** `sdk/include/` (contains the public framework headers, including [`PerfUI/PerfUIApi.h`](../include/PerfUI/PerfUIApi.h)).
2. **Import Library:** `sdk/lib/PerfUI.lib` — the static import library for dynamic linking against `PerfUI.dll`. Every mod links to `PerfUI.lib` and shares a **single unified framework instance in game memory**. Mods do **not** link against `PerfUI_Core.lib`.
3. **Compiler Toolchain:** **MSVC v143** (Visual Studio 2022, `_MSC_VER >= 1930`).
4. **C++ Runtime:** Dynamic multithreaded runtime **`/MD`** (Release). Debug runtime (`/MDd`) uses an incompatible `_ITERATOR_DEBUG_LEVEL` (2 vs 0) and will be safely rejected by the runtime ABI validator to prevent game crashes.
5. **Language Standard:** **C++20** (`/std:c++20`).
6. **Zero External Dependencies:** You do not need to install DirectX 11 SDK, Dear ImGui, or Flash tools.

---

## 2. Quick Start

To render a custom UI (HUD widget, overlay, health bar, or menu window) in Skyrim, include the headers, request the context, and mount your elements:

```cpp
#include "PerfUI/PerfUIApi.h"
#include "PerfUI/Panel.h"
#include "PerfUI/Text.h"
#include "PerfUI/Button.h"

void InitializeMyMod() {
    // 1. Acquire the root PerfUI context
    auto* ctx = PerfUI::Client::GetContext();
    if (!ctx || !ctx->root()) {
        return; // PerfUI.dll is not loaded or not installed
    }

    // 2. Create and attach a panel directly to the root screen container
    auto* myPanel = ctx->root()->add<PerfUI::Panel>("MyModPanel");

    // 3. Style the panel with Nordic glass aesthetics
    myPanel->backgroundColor(PerfUI::Color(16, 22, 32, 220));
    myPanel->borderColor(PerfUI::Color::NordicGold());
    myPanel->cornerRadius(8.0f);
    myPanel->shadow(true, PerfUI::Color(0, 0, 0, 180), 16.0f);

    // 4. Configure automatic Flexbox layout
    myPanel->layout().direction(PerfUI::LayoutDirection::Vertical)
                     .padding(12.0f)
                     .gap(8.0f);

    // 5. Add text and interactive buttons
    auto* title = myPanel->add<PerfUI::Text>("My First PerfUI Mod!");
    title->color(PerfUI::Color::TextPrimary()).bold(true);

    auto* btn = myPanel->add<PerfUI::Button>("Click Me");
    btn->onClick([]() {
        // Trigger native Skyrim audio and a styled toast notification
        PerfUI::Client::PlaySound("UIMenuOK");
        PerfUI::Client::ShowToast("Success!", "Button clicked!", PerfUI::ToastType::Success);
    });
}
```

---

## 3. Core Widget Catalog

PerfUI includes an extensive suite of production-ready widgets:

| Widget | Purpose | Usage Example |
| :--- | :--- | :--- |
| **`Panel`** | Styled container with corner radius, borders, and drop shadows | `auto* p = add<Panel>();` |
| **`Text`** | Crisp vector typography (font size, weight, auto-wrapping) | `auto* t = add<Text>("Status");` |
| **`Button`** | Interactive button with hover transitions and audio feedback | `auto* b = add<Button>("Confirm");` |
| **`ProgressBar`**| Animated gauge with easing (HP, Magicka, Stamina, XP) | `auto* bar = add<ProgressBar>(0.75f);` |
| **`Slider`** | Draggable numeric slider with min/max range and steps | `auto* s = add<Slider>(50.0f, 0.0f, 100.0f);` |
| **`Checkbox`** | Interactive toggle with animated check dot | `auto* c = add<Checkbox>("Enabled");` |
| **`ComboBox`** | Dropdown selection list with viewport auto-clamping | `auto* cb = add<ComboBox>(options);` |
| **`TextInput`** | Single-line text input field with cursor navigation | `auto* in = add<TextInput>("Search...");` |
| **`ScrollView`** | Scrollable container with hardware scrollbar and wheel support | `auto* sv = add<ScrollView>();` |
| **`Image`** | DirectX 11 shader resource view (`ID3D11ShaderResourceView*`) display | `auto* img = add<Image>(srv, w, h);` |
| **`TabBar`** | Horizontal tab navigation bar with selection indicators | `auto* tb = add<TabBar>();` |
| **`ModalDialog`**| Centered modal popup dialog with confirm/cancel buttons | `ctx->showModal(modal);` |
| **`ContextMenu`**| Floating right-click context menu | `ctx->showContextMenu(pos, items);` |

---

## 4. Layout Box Model (Flexbox)

PerfUI features an automatic CSS Flexbox-inspired layout engine. You never need to hardcode absolute pixel coordinates:

- **Stack Direction:**
  - `layout().direction(LayoutDirection::Horizontal)` — children align horizontally in a row.
  - `layout().direction(LayoutDirection::Vertical)` — children stack vertically in a column.
- **Spacing & Padding:**
  - `layout().padding(12.0f, 8.0f)` — internal padding (horizontal, vertical).
  - `layout().margin(0.0f, 10.0f)` — external margins.
  - `layout().gap(6.0f)` — inter-item spacing between consecutive children.
- **Alignment:**
  - `layout().alignment(Alignment::Center)` — cross-axis alignment (`Start`, `Center`, `End`, `Stretch`).
  - `layout().justify(JustifyContent::SpaceBetween)` — main-axis alignment (`Start`, `Center`, `End`, `SpaceBetween`, `SpaceAround`).
- **Dimensions:**
  - `layout().width(180.0f)` — fixed width in pixels.
  - `layout().width(DimensionConstraint::Flex(1.0f))` — flexible weight filling available remaining space.
  - `layout().width(DimensionConstraint::Percent(50.0f))` — percentage of parent width.

---

## 5. Animations & Lifecycle (`update`)

For smooth transitions and easing curves, use the built-in `AnimatedFloat` helper:

```cpp
class MyAnimatedCard : public PerfUI::Panel {
public:
    MyAnimatedCard() {
        m_scale.snapTo(0.0f);
        m_scale.setTarget(1.0f); // Smooth pop-in animation
    }

    void update(float deltaTime) override {
        Panel::update(deltaTime);
        m_scale.update(deltaTime); // Physics-based spring easing curve
    }

private:
    PerfUI::AnimatedFloat m_scale{ 0.0f, 14.0f }; // (initialValue, stiffness)
};
```

---

## 6. Audio & Toast API

Mods can play native Skyrim UI sounds via their EditorIDs and dispatch sleek toast notifications with a single call:

```cpp
// Native Skyrim sound descriptors
PerfUI::Client::PlaySound("UIMenuBlade");      // Tab switch sound
PerfUI::Client::PlaySound("UIMenuOK");         // Confirm sound
PerfUI::Client::PlaySound("UIMenuCancel");     // Cancel / close sound
PerfUI::Client::PlaySound("UIQuestUpdate");    // Quest objective sound

// Toast notifications
PerfUI::Client::ShowToast("Notice", "Inventory synchronized.", PerfUI::ToastType::Info, 3.0f);
PerfUI::Client::ShowToast("Danger", "Health dropped below 20%!", PerfUI::ToastType::Warning, 2.5f);
```

---

## 7. Complete Custom HUD Example (`CustomHealthBar`)

The following complete example demonstrates how to build a dynamic HUD element and link it into PerfUI:

```cpp
#include "PerfUI/Panel.h"
#include "PerfUI/ProgressBar.h"
#include "PerfUI/Text.h"
#include "PerfUI/PerfUIApi.h"
#include <algorithm>
#include <cstdio>

class CustomHealthBar : public PerfUI::Panel {
public:
    CustomHealthBar() : Panel("CustomHealthBar") {
        backgroundColor(PerfUI::Color(14, 18, 26, 215));
        borderColor(PerfUI::Color::BorderSubtle());
        borderWidth(1.2f);
        cornerRadius(8.0f);
        shadow(true, PerfUI::Color(0, 0, 0, 180), 16.0f, { 0.0f, 4.0f });

        layout().direction(PerfUI::LayoutDirection::Horizontal)
                .alignment(PerfUI::Alignment::Center)
                .padding(12.0f, 6.0f)
                .gap(10.0f);

        m_label = add<PerfUI::Text>("HP");
        m_label->color(PerfUI::Color(255, 80, 80, 255)).fontSize(12.0f).bold(true);

        m_bar = add<PerfUI::ProgressBar>(1.0f);
        m_bar->layout().width(180.0f).height(12.0f);
        m_bar->fillColor(PerfUI::Color(210, 40, 40, 255));
        m_bar->trackColor(PerfUI::Color(30, 16, 16, 220));

        m_valText = add<PerfUI::Text>("100 / 100");
        m_valText->color(PerfUI::Color::TextSecondary()).fontSize(11.0f);
    }

    void setHealth(float current, float max) {
        float ratio = std::clamp(current / max, 0.0f, 1.0f);
        m_bar->progress(ratio);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", current, max);
        m_valText->text(buf);
    }

private:
    PerfUI::Text* m_label{ nullptr };
    PerfUI::ProgressBar* m_bar{ nullptr };
    PerfUI::Text* m_valText{ nullptr };
};
```

---

## 8. Threading Model & `RunOnUIThread`

PerfUI executes layout calculations, animations, and graphics rendering strictly on the **DirectX 11 Present Render Thread** (the UI thread).

- **Golden Rule:** Never mutate UI elements directly from arbitrary background threads (such as Papyrus event sinks, async worker threads, SKSE message listeners, or network callbacks).
- To safely dispatch changes to the UI from any thread, use `RunOnUIThread`:
  ```cpp
  PerfUI::Client::RunOnUIThread([=]() {
      // Guaranteed to execute on the UI thread at the beginning of UIContext::update
      if (g_customHud) {
          g_customHud->setHealth(newHp, maxHp);
      }
  });
  ```
- **Debug Protection:** In Debug builds, PerfUI checks thread identity via `assertUIThread()`. If external code attempts to modify widgets without `RunOnUIThread`, the debugger will halt immediately, identifying the exact offending call site.

---

## 9. Troubleshooting & FAQ

### `error LNK2019 / LNK2001: unresolved external symbol UIElement::... Panel::...`
- **Cause:** The project build settings do not link against the dynamic import library `PerfUI.lib`.
- **Solution:** Add `PerfUI.lib` from the `sdk/lib/` folder to `target_link_libraries`:
  ```cmake
  find_library(PERFUI_IMPORT_LIB NAMES PerfUI PATHS "$ENV{PERFUI_SDK}/lib")
  target_link_libraries(MyMod PRIVATE ${PERFUI_IMPORT_LIB})
  ```
  *(Do not link `PerfUI_Core.lib` — each mod must dynamically link to the shared `PerfUI.dll`!)*

### `GetApi()` or `GetContext()` returns `nullptr` (or log reports `_ITERATOR_DEBUG_LEVEL mismatch`)
- **Cause:** Your mod was compiled in Debug mode with `/MDd` (`_ITERATOR_DEBUG_LEVEL == 2`), while `PerfUI.dll` runs in Release (`_ITERATOR_DEBUG_LEVEL == 0`). In MSVC, standard library types like `std::string` and `std::function` have incompatible memory layouts between Debug and Release. To protect the game from crashes, PerfUI rejects mismatched ABI calls.
- **Solution:** Compile your SKSE plugin in Release mode using the `/MD` runtime switch.

### Log reports: `MSVC toolchain version mismatch`
- **Cause:** The compiler toolset version is older than Visual Studio 2022 v143 (`_MSC_VER < 1930`).
- **Solution:** Upgrade to Visual Studio 2022 (MSVC v143 toolset).

### Intermittent crashes when modifying UI from an SKSE Message or Game Event Sink
- **Cause:** UI modifications are occurring on Skyrim's main gameplay thread while DirectX 11 Present is actively drawing the frame.
- **Solution:** Wrap the update inside `PerfUI::Client::RunOnUIThread([=]() { ... })`.

---

## 10. Standalone Sandbox Testing

To test UI widgets outside of Skyrim, build and run `PerfUI_Sandbox.exe`:
- **`[F8]`** — Toggle modder `CustomHealthBar` overlay.
- **`[J]`** — Deal 15 damage (tests health decrement animation and critical low-HP pulse).
- **`[H]`** — Restore 15 health.
- **`[F10]`** — Open fullscreen Main Menu.
- **`[F11]`** — Open Nordic Quest Journal.
