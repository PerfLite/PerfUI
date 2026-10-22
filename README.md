# PerfUI

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CI](https://github.com/PerfLite/PerfUI/actions/workflows/build.yml/badge.svg)](https://github.com/PerfLite/PerfUI/actions)
[![Version: 0.1.0--alpha](https://img.shields.io/badge/Version-0.1.0--alpha-orange.svg)](ROADMAP.md)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-f34b7d.svg)](https://en.cppreference.com/w/cpp/20)
[![DirectX 11](https://img.shields.io/badge/Renderer-DirectX%2011-0078D6.svg)](https://learn.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
[![Skyrim SE / AE](https://img.shields.io/badge/Skyrim-SE%201.5.97%20%7C%20AE%201.6%2B-555555.svg)](https://skse.silverlock.org/)
[![Backend: Dear ImGui](https://img.shields.io/badge/Backend-Dear%20ImGui-success.svg)](https://github.com/ocornut/imgui)

![PerfUI Smart Compass & Quest Objective Overlay](screenshots/2.png)

<p align="center">
  <img src="screenshots/1.png" alt="PerfUI Status Bars & Character Stats Overlay" />
</p>

**PerfUI** is an independent, high-performance retained-mode C++20 user interface framework built specifically for *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition*, as well as standalone DirectX 11 applications.

It provides mod authors and game developers with an intuitive, modern object-oriented API for building complex, fluid, gamepad-friendly in-game menus, HUD overlays, journal windows, and configuration interfaces.

> **Status: Alpha (`v0.1.0-alpha`)** — Core architecture, retained scene graph, layout engine, SVG vector rendering, XML markup, and Skyrim DirectX 11 hooks are fully functional. Active development and in-game pilot testing are ongoing.

---

## 🌟 Acknowledgments & Third-Party Credits

PerfUI utilizes several proven open-source libraries as underlying engine foundations while maintaining strict interface abstraction:

* **[Dear ImGui](https://github.com/ocornut/imgui)** (Omar Cornut and contributors) — Primary 2D rendering backend foundation (`ImDrawList` vertex buffer pipeline). Licensed under [MIT](https://github.com/ocornut/imgui/blob/master/LICENSE.txt).
* **[NanoSVG](https://github.com/memononen/nanosvg)** (Mikko Mononen) — Ultra-lightweight C99 SVG vector parser and rasterizer. Licensed under [zlib](https://github.com/memononen/nanosvg/blob/master/LICENSE.txt).
* **[pugixml](https://github.com/zeux/pugixml)** (Arseny Kapoulkine) — Fast, light C++ XML parser used for declarative UI markup and hot-reload. Licensed under [MIT](https://github.com/zeux/pugixml/blob/master/LICENSE.md).
* **[stb](https://github.com/nothings/stb)** (Sean Barrett) — Public domain / MIT single-header image decoding (`stb_image.h`).
* **[CommonLibSSE-ng](https://github.com/CharmedBaryon/CommonLibSSE-ng)** (CharmedBaryon and contributors) — Reverse-engineered Skyrim engine wrapper. Licensed under [MIT](https://github.com/CharmedBaryon/CommonLibSSE-ng/blob/main/LICENSE).

### The Decoupling Principle
While Dear ImGui powers low-level font rendering and 2D vector primitives, **PerfUI's public API never exposes `<imgui.h>`**. All client and widget code interacts exclusively with PerfUI's retained scene graph and the abstract `UIRenderBackend` interface. This allows future renderer backends (native D3D11, Vulkan, or D3D12) to be swapped in seamlessly without modifying a single line of client UI code.

---

## ⚡ Key Features

* **Retained-Mode Scene Graph:** Hierarchical DOM-like element tree (`UIElement`, `UIWindow`, `Panel`) with dirty-flag layout optimization.
* **Flexbox & Stack Layout Engine:** Automatic layout calculation, flex-grow/shrink, alignment, padding, margins, and auto-sizing.
* **Controller & Gamepad First:** Full focus-graph navigation supporting D-Pad, Thumbsticks, Keyboard, and Mouse with automatic spatial focus transitions.
* **Authentic Masterwork Nordic Sword Cursor:**
  * Global hardware-accurate Skyrim sword cursor rendered in the foreground draw list with soft drop shadows.
  * Zero-conflict Scaleform integration: hooks Skyrim's `CursorMenu` without Flash null-pointer dereferences or cursor disappearance bugs.
* **Native Vector SVG Graphics:**
  * Embedded NanoSVG parser and rasterizer (`SvgImage` widget, `SvgService`).
  * Crisp, resolution-independent vector rendering at any display scale or 4K/ultrawide resolution with zero browser runtime overhead.
  * Direct declarative XML support (`<SvgImage src="..." width="32" height="32" />`).
* **Rich Modern Widget Library:**
  * **Containers & Windows:** `UIWindow`, `Panel`, `ScrollView`, `ModalDialog`, `ContextMenu`, `TabBar`, `Accordion`, `Sidebar`, `CategoryList`.
  * **Controls:** `Button`, `Checkbox`, `Slider`, `ComboBox`, `TextInput`, `ProgressBar`, `Switch`, `RadioButton`, `RadioGroup`, `KeybindSelector`, `SegmentedControl`.
  * **Display & Data:** `Text`, `TextArea`, `Image`, `SvgImage`, `Toast`, `Notification`, `ItemSlot`, `SearchableList`, `QuestTracker`, `MapCanvas` (radar & world modes), `Divider`, `Spacer`.
* **Theme Tokens & Skyrim Aesthetics:** Built-in design token system with 4 visual presets: `SkyrimClassic`, `NordicDark`, `GlassFrost`, `WarmParchment`.
* **Animation & State Engine:** Smooth easing functions, transitions, hover/active glow effects, and auto-fade mechanisms.
* **Cyrillic & Typography Ready:** Native UTF-8 string support with Cyrillic glyph range preloading (`GetGlyphRangesCyrillic`) and merged Segoe UI Symbol font for quest diamond markers, dingbats, and geometric symbols.
* **Skyrim Game Services:** Safe bridge for playing native UI sound descriptors, querying quests/stats, and hooking into the DirectX 11 Present loop.
* **Declarative XML Markup & Instant Hot-Reload:** Static library (`PerfUI_Markup`) allowing authors to declare complex UIs in XML, bind C++ callbacks by name, and edit layouts live with instant state-preserving hot-reload. See [docs/MARKUP.md](docs/MARKUP.md).

---

## ⚡ Performance, Architecture & Benchmarks

### Dirty-Flag Layout Caching vs. Immediate-Mode Rendering
To provide extreme responsiveness without stutter, PerfUI uses a two-stage decoupled execution model:
1. **Layout Pass (Cached via Dirty Flags):**
   - Expensive geometry measurements (`LayoutEngine::Measure`) and box-model arrangements (`LayoutEngine::Arrange`) only run when a widget's geometry or contents actually change (via `markLayoutDirty()`).
   - When the widget tree is static, layout calculations are **100% skipped** (`isLayoutDirty() == false`), reducing CPU overhead to practically zero.
2. **Render Pass (Immediate-Mode Primitive Streaming):**
   - In each frame, `UIElement::render()` traverses the visible hierarchy and emits 2D vector commands to `UIRenderBackend` (mapping directly to `ImDrawList`).
   - This ensures 60 FPS / 144+ FPS animation smoothness, easing curves, and glow interpolations without the memory footprint or synchronization overhead of caching complex vertex buffers.

### Benchmark Data (550+ Active Retained Elements)
Measurements taken on Windows 11 x64 (MSVC 2022 v143, Release Build):

| Metric | 550+ Elements Hierarchy | Overhead / Note |
| :--- | :--- | :--- |
| **Cold Layout + Render** | **~0.21 ms** (208 µs) | Initial tree build and full measurement |
| **Layout Cache Pass (Dirty-Flag Hit)** | **~0.05 ms** (50 µs) | Layout calculation skipped via cache; pure draw command submission |
| **Full Tree Re-Layout** | **~0.11 ms** (109 µs) | Invalidation of root + flexbox re-arrangement |
| **Mock Draw Call Submission** | **1,042 draw calls in 0.04 ms** | Sub-microsecond per draw command |

*Impact on Game Frame Rate:* At 60 FPS (16.6 ms budget), a typical PerfUI menu with hundreds of interactive widgets takes **less than 0.3% of the frame budget**, guaranteeing zero framerate drops in Skyrim.

---

## 🏗️ Architecture

```text
┌────────────────────────────────────────────────────────────────────────┐
│                          Game / Mod Logic                              │
│              Custom HUD, Menus, Mod Configuration Overlays             │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        PerfUI Public API Layer                         │
│   UIContext, UIWindow, Panel, Button, Text, SvgImage, MapCanvas, etc.  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                              PerfUI Core                               │
│  ├── Retained UI Tree (UIElement, Hierarchy, Memory Lifecycle)         │
│  ├── Layout Engine (Flex / Box Model / Anchoring)                     │
│  ├── Styling & Theme Engine (Tokens, Brushes, Palettes, Fonts)         │
│  ├── Animation Engine (Interpolation, Easing Curves)                   │
│  ├── Input & Focus Graph (Gamepad D-Pad, Mouse, Keyboard Capture)      │
│  ├── Vector Graphics Service (NanoSVG rasterization)                   │
│  └── Event System (Bubbling, Tunneling, Direct Callbacks)              │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       UIRenderBackend Interface                        │
│   beginFrame(), drawRect(), drawRoundedRect(), drawText(), drawLine()  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                 Backend Implementation (ImDrawList)                    │
│             [Dear ImGui](https://github.com/ocornut/imgui) (MIT)       │
│                                   │                                    │
│                                   ▼                                    │
│                         DirectX 11 Hardware                            │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start Example

Creating an interactive window with `UIWindow` and flexbox layout:

```cpp
#include <PerfUI/PerfUI.h>
#include <PerfUI/UIWindow.h>

void CreateMyCustomMenu(PerfUI::UIContext& context) {
    // 1. Create a top-level window with title bar and close button
    auto* window = context.root()->add<PerfUI::UIWindow>("My Mod Configuration");
    window->setBounds(PerfUI::Rect{ 100.0f, 100.0f, 480.0f, 360.0f });
    window->layout()
        .direction(PerfUI::LayoutDirection::Vertical)
        .padding(16.0f)
        .gap(12.0f);

    // 2. Add description text
    auto* desc = window->add<PerfUI::Text>("Retained-mode modern UI for Skyrim modders.");
    desc->color(PerfUI::Color(220, 225, 235)).fontSize(12.0f);

    // 3. Add Interactive Controls
    auto* slider = window->add<PerfUI::Slider>(50.0f, 0.0f, 100.0f);
    slider->onValueChanged([](float val) {
        // Handle value change
    });

    auto* checkbox = window->add<PerfUI::Checkbox>("Enable Dynamic Status HUD", true);
    checkbox->onToggle([](bool enabled) {
        // Toggle feature
    });

    auto* button = window->add<PerfUI::Button>("Save & Apply Settings");
    button->onClick([window]() {
        window->close();
    });
}
```

---

## 📂 Repository Structure

```text
PerfUI/
├── include/PerfUI/         # Public header files (No ImGui headers exposed!)
│   ├── Accordion.h         # Collapsible accordion container
│   ├── Animation.h         # Tweening & easing helpers
│   ├── Button.h            # Button widget
│   ├── CategoryList.h      # Categorized list with icons
│   ├── Checkbox.h          # Checkbox widget
│   ├── ComboBox.h          # Dropdown combo box widget
│   ├── ContextMenu.h       # Popup context menu
│   ├── Divider.h           # Visual separator line
│   ├── ItemSlot.h          # Inventory item slot with rarity border
│   ├── KeybindSelector.h   # Keyboard/gamepad key binding control
│   ├── MapCanvas.h         # Interactive map & radar canvas widget
│   ├── ModalDialog.h       # Modal dialog window
│   ├── Notification.h      # Toast & banner notifications
│   ├── Panel.h             # Flexible container panel
│   ├── ProgressBar.h       # Progress bar widget
│   ├── QuestTracker.h      # Quest tracking HUD widget
│   ├── RadioButton.h       # Single choice radio button
│   ├── RadioGroup.h        # Radio button grouping container
│   ├── ScrollView.h        # Scrollable container view
│   ├── SearchableList.h    # Filterable item list
│   ├── SegmentedControl.h  # Segmented button group
│   ├── Sidebar.h           # Collapsible navigation sidebar
│   ├── Slider.h            # Numeric slider control
│   ├── Spacer.h            # Flexible layout spacer
│   ├── SvgImage.h          # Native vector SVG rendering widget
│   ├── SvgService.h        # SVG parsing and rasterization cache
│   ├── Switch.h            # Toggle switch control
│   ├── TabBar.h            # Tab navigation widget
│   ├── Text.h              # Text display widget
│   ├── TextArea.h          # Multi-line text display & editor
│   ├── TextInput.h         # Single-line text input box
│   ├── Theme.h             # Design tokens, metrics, and themes
│   ├── Toast.h             # Pop-up toast notifications
│   ├── UIContext.h         # Root framework context
│   ├── UIElement.h         # Base retained element
│   └── UIWindow.h          # Top-level window with titlebar & close button
│   └── Markup/             # Declarative XML Markup headers
│       ├── MarkupLoader.h  # XML loader with hot-reload engine
│       ├── MarkupTypes.h   # Markup parser structures
│       └── WidgetRegistry.h# Tag-to-widget registration factory
├── src/                    # Implementation files
│   ├── backends/           # Render backends (ImGui, Mock headless backend)
│   ├── core/               # Core runtime, context, memory lifecycle
│   ├── layout/             # Flexbox and box-model layout engine
│   ├── skyrim/             # DirectX 11 hook, SKSE plugin, CursorMenu hook, audio bridge
│   └── widgets/            # Widget implementations
├── tests/                  # Automated unit test suite
│   ├── BackendIndependenceTest.cpp # Proof of 100% backend independence
│   ├── LayoutTest.cpp      # Flex-grow, padding, auto-size & 500-widget benchmark
│   └── MarkupTest.cpp      # Declarative XML markup and hot-reload tests
├── examples/               # Sample implementations for modders
│   ├── modder_custom_hud/  # Custom HUD widget example
│   ├── modder_menu_plugin/ # Interactive SKSE mod menu window example
│   └── sandbox/            # Standalone Win32/D3D11 desktop showcase sandbox
├── third_party/
│   ├── imgui/              # Dear ImGui library (Omar Cornut - MIT License)
│   ├── nanosvg/            # NanoSVG vector parser & rasterizer (Mikko Mononen - zlib License)
│   ├── pugixml/            # Fast XML parser (Arseny Kapoulkine - MIT License)
│   └── stb/                # STB single-header libraries (Sean Barrett - MIT / Public Domain)
├── docs/                   # Documentation and guides
├── CMakeLists.txt          # CMake build configuration
├── xmake.lua               # XMake build script
├── LICENSE                 # MIT License
└── README.md               # Project documentation
```

---

## 🛠️ Build & Testing

* **OS:** Windows 10 / 11 (64-bit), Linux (Core & Tests)
* **Compiler:** Microsoft Visual C++ (MSVC) 2022 v143+, GCC 12+, Clang 15+ with C++20 support (`/std:c++20` / `-std=c++20`)
* **Build System:** [XMake](https://xmake.io/) (Recommended) or [CMake](https://cmake.org/) (3.20+)
* **Dependencies:** DirectX 11 SDK (included with Windows SDK for Skyrim plugin and Win32 sandbox; Core & Tests are dependency-free)

### Building with XMake (Fastest)

```powershell
# Clone the repository
git clone https://github.com/PerfLite/PerfUI.git
cd PerfUI

# Build & run layout and focus test suite + benchmark
xmake run PerfUI_Test_Layout

# Build & run backend independence test
xmake run PerfUI_Test_Independence

# Build & run markup tests
xmake run PerfUI_Test_Markup
```

### Building with CMake

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target PerfUI_Test_Layout PerfUI_Test_Independence PerfUI_Test_Markup

# Run tests
ctest --test-dir build -C Release --output-on-failure
```

---

## 📚 Documentation & Guides

* **Modder Integration Guide:** [English](docs/MODDER_GUIDE_EN.md) | [Русский](docs/MODDER_GUIDE.md)
* **Declarative XML Markup & Hot-Reload:** [English](docs/MARKUP_EN.md) | [Русский](docs/MARKUP.md)
* **Skyrim Input, Mouse & Cursor Integration:** [English](docs/SKYRIM_INPUT_GUIDE_EN.md) | [Русский](docs/SKYRIM_INPUT_GUIDE.md)
* **MapCanvas Radar & World Widget:** [English](docs/MAPCANVAS_EN.md) | [Русский](docs/MAPCANVAS.md)
* **Core Architecture Overview:** [ARCHITECTURE.md](ARCHITECTURE.md)
* **Visual & Widget Design Specification:** [DESIGN.md](DESIGN.md)

---

## 🇷🇺 Описание проекта (Russian Overview)

**PerfUI** — это независимый, высокопроизводительный retained-mode UI-фреймворк на современном стандарте **C++20**, разработанный специально для создания модификаций и интерфейсов для игры *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition*, а также для автономных графических приложений на DirectX 11.

> **Текущий статус: Альфа (`v0.1.0-alpha`)** — архитектура ядра, Flexbox-раскладка, векторная графика SVG, XML Markup и хуки рендера DirectX 11 полностью функциональны и тестируются в реальной игре.

Фреймворк предоставляет авторам модов и разработчикам удобный объектно-ориентированный C++ API для построения плавных, отзывчивых и удобных для управления с геймпада внутриигровых меню: журналов заданий, кастомных HUD-полосок, экранов настроек, диалоговых окон и окон конфигурации модов.

### 🌟 Ключевые особенности

* **Дерево элементов (Retained-Mode):** Иерархический граф сцены (`UIElement`, `UIWindow`, `Panel`) с dirty-флагами — вычисления геометрии и Flexbox происходят только при фактических изменениях (< 0.2 мс на кадр при 550+ элементах).
* **Движок вёрстки (Flexbox & Box-Model):** Автоматическое распределение элементов, гибкие направления (`Row` / `Column`), адаптивное растяжение (`flex-grow`), отступы (`padding`, `margin`) и авто-размеры (`Auto`).
* **Адаптация под геймпады (Controller-First):** Полноценная навигация по графу фокуса с помощью крестовины (D-Pad), стиков, клавиатуры и мыши с автоматическим пространственным переходом фокуса.
* **Аутентичный меч-курсор Скайрима:**
  * Собственный векторный меч-курсор в foreground-слое с мягкими тенями.
  * Бесшовная интеграция со Scaleform `CursorMenu`: исключает вылеты в Flash runtime и исчезновение курсора при диалогах и переключении окон.
* **Нативная векторная графика SVG:**
  * Встроенный легковесный парсер и растеризатор NanoSVG (`SvgImage`, `SvgService`).
  * Абсолютная чёткость и независимость от разрешения экрана (Full HD, 2K, 4K, Ultrawide) без оверхеда браузерных движков.
  * Поддержка тега `<SvgImage src="..." />` прямо в XML Markup.
* **Большая библиотека современных виджетов:**
  * **Окна и контейнеры:** `UIWindow`, `Panel`, `ScrollView`, `ModalDialog`, `ContextMenu`, `TabBar`, `Accordion`, `Sidebar`, `CategoryList`.
  * **Элементы управления:** `Button`, `Checkbox`, `Slider`, `ComboBox`, `TextInput`, `ProgressBar`, `Switch`, `RadioButton`, `RadioGroup`, `KeybindSelector`, `SegmentedControl`.
  * **Информационные виджеты:** `Text`, `TextArea`, `Image`, `SvgImage`, `Toast`, `Notification`, `ItemSlot`, `SearchableList`, `QuestTracker`, `MapCanvas` (радар и карта), `Divider`, `Spacer`.
* **Стилизация в эстетике Скайрима:** Дизайн-токены и 4 встроенные темы (`SkyrimClassic`, `NordicDark`, `GlassFrost`, `WarmParchment`), движок плавных анимаций (появление, пульсация, hover-эффекты).
* **Поддержка кириллицы и символов:** Встроенная загрузка кириллических диапазонов глифов ImGui, системных шрифтов Windows и объединённый шрифт Segoe UI Symbol для маркеров квестов и геометрических значков.
* **Изоляция бэкенда:** Проект использует библиотеку [Dear ImGui](https://github.com/ocornut/imgui) **исключительно** как начальный фундамент растеризации вершинных буферов (`ImDrawList`). Внешний API `PerfUI` полностью независим и не подключает заголовочные файлы ImGui, что позволяет в будущем бесшовно заменить бэкенд на прямой D3D11/D3D12/Vulkan без переписывания пользовательского кода интерфейсов.

---

## 🔥 Рисуй интерфейс без перекомпиляции (XML Markup & Hot-Reload)

С библиотекой **`PerfUI_Markup`** интерфейсы описываются в декларативном XML без необходимости пересобирать плагин или перезапускать игру:

```xml
<UI>
  <Panel name="Settings" direction="column" width="420" padding="20" gap="10" bg="$surfaceElevated">
    <Text text="Настройки мода" font="title" color="$nordicGold"/>
    <Slider name="Volume" label="Громкость" min="0" max="100" value="80"/>
    <Checkbox name="EnableCompass" label="Показывать расширенный компас" checked="true"/>
    <Panel direction="row" gap="8" justify="end">
      <Button text="Отмена" onClick="cancelAction"/>
      <Button text="Сохранить" onClick="saveAction" class="primary"/>
    </Panel>
  </Panel>
</UI>
```

```cpp
#include <PerfUI/Markup/MarkupLoader.h>

PerfUI::MarkupLoader loader(context);
loader.bindCallback("cancelAction", [&]() { /* ... */ });
loader.bindCallback("saveAction", [&]() { /* ... */ });

loader.enableHotReload(true);
loader.loadFile("Data/Interface/PerfUI/settings.xml");

// В каждом кадре:
loader.poll(); // Автоматически перезагрузит изменённый XML!
```

* **Мгновенный отклик:** файл опрашивается каждые ~500 мс без блокировок и фоновых потоков.
* **Сохранение состояния:** при перезагрузке автоматически сохраняется фокус ввода, позиция скролла (`ScrollView`) и выбранные вкладки (`TabBar`).
* **Устойчивость к ошибкам:** при синтаксической ошибке в XML предыдущее рабочее дерево не ломается, а на экран выводится Toast с номером строки ошибки.
* **Подробное руководство:** см. [docs/MARKUP.md](docs/MARKUP.md).

---

## 📜 License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for the full text.

### Third-Party Licenses

* **[Dear ImGui](https://github.com/ocornut/imgui)**  
  Copyright (c) 2014-2026 Omar Cornut.  
  Licensed under the [MIT License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt).
* **[NanoSVG](https://github.com/memononen/nanosvg)**  
  Copyright (c) 2013-2014 Mikko Mononen.  
  Licensed under the [zlib License](https://github.com/memononen/nanosvg/blob/master/LICENSE.txt).
* **[pugixml](https://github.com/zeux/pugixml)**  
  Copyright (c) 2006-2025 Arseny Kapoulkine.  
  Licensed under the [MIT License](https://github.com/zeux/pugixml/blob/master/LICENSE.md).
* **[stb](https://github.com/nothings/stb)** (stb_image)  
  Copyright (c) 2017 Sean Barrett.  
  Licensed under the [MIT License / Public Domain](https://github.com/nothings/stb/blob/master/LICENSE).
* **[CommonLibSSE-ng](https://github.com/CharmedBaryon/CommonLibSSE-ng)**  
  Licensed under the [MIT License](https://github.com/CharmedBaryon/CommonLibSSE-ng/blob/main/LICENSE).
