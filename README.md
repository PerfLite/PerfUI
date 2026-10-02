# PerfUI

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL%203.0-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/Language-C%2B%2B23-f34b7d.svg)](https://en.cppreference.com/w/cpp/23)
[![DirectX 11](https://img.shields.io/badge/Renderer-DirectX%2011-0078D6.svg)](https://learn.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
[![Skyrim SE / AE](https://img.shields.io/badge/Skyrim-SE%201.5.97%20%7C%20AE%201.6%2B-555555.svg)](https://skse.silverlock.org/)
[![Backend: Dear ImGui](https://img.shields.io/badge/Backend-Dear%20ImGui-success.svg)](https://github.com/ocornut/imgui)

**PerfUI** is an independent, high-performance retained-mode C++23 user interface framework built specifically for *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition*, as well as standalone DirectX 11 applications.

It provides mod authors and game developers with an intuitive, modern object-oriented API for building complex, fluid, gamepad-friendly in-game menus, HUD overlays, journal windows, and configuration interfaces.

---

## 🌟 Acknowledgments & Third-Party Credits

PerfUI utilizes **[Dear ImGui](https://github.com/ocornut/imgui)** (created by [Omar Cornut](https://github.com/ocornut) and contributors) as its primary rendering backend foundation (`ImDrawList` vertex buffer pipeline).

* **[Dear ImGui GitHub Repository](https://github.com/ocornut/imgui)**
* **License:** Dear ImGui is licensed under the [MIT License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt).

### The Decoupling Principle
While Dear ImGui powers low-level font rendering and 2D vector primitives, **PerfUI's public API never exposes `<imgui.h>`**. All client and widget code interacts exclusively with PerfUI's retained scene graph and the abstract `UIRenderBackend` interface. This allows future renderer backends (native D3D11, Vulkan, or D3D12) to be swapped in seamlessly without modifying a single line of client UI code.

---

## ⚡ Key Features

* **Retained-Mode Scene Graph:** Hierarchical DOM-like element tree (`UIElement`, `UIWindow`, `Panel`) with dirty-flag optimization (layout, style, and render caches).
* **Flexbox & Stack Layout Engine:** Automatic layout calculation, flex-grow/shrink, alignment, padding, margins, and auto-sizing.
* **Controller & Gamepad First:** Full focus-graph navigation supporting D-Pad, Thumbsticks, Keyboard, and Mouse with automatic spatial focus transitions.
* **Rich Modern Widget Library:**
  * Containers: `Panel`, `ScrollView`, `ModalDialog`, `ContextMenu`, `TabBar`
  * Controls: `Button`, `Checkbox`, `Slider`, `ComboBox`, `TextInput`, `ProgressBar`
  * Display: `Text`, `Image`, `Toast` notifications
* **Nordic Skyrim Styling & Aesthetics:** Built-in Skyrim-authentic themes (parchment, dark charcoal slate, gold and silver trim, runes, ambient glass depth).
* **Animation & State Engine:** Smooth easing functions, transitions, hover/active glow effects, and auto-fade mechanisms.
* **Skyrim Game Services:** Optional bridge for playing native UI sound descriptors, querying quests/stats, and hooking into the DirectX 11 Present loop.

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
│     UIContext, UIWindow, Panel, Button, Text, ScrollView, Slider...    │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                              PerfUI Core                               │
│  ├── Retained UI Tree (UIElement, Hierarchy, Memory Lifecycle)         │
│  ├── Layout Engine (Flex / Box Model / Anchoring)                     │
│  ├── Styling & Theme Engine (Brushes, Palettes, Metrics, Fonts)        │
│  ├── Animation Engine (Interpolation, Easing Curves)                   │
│  ├── Input & Focus Graph (Gamepad D-Pad, Mouse, Keyboard Capture)      │
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

Building a custom interactive window with PerfUI is simple and expressive:

```cpp
#include <PerfUI/UIContext.h>
#include <PerfUI/UIElement.h>
#include <PerfUI/Panel.h>
#include <PerfUI/Button.h>
#include <PerfUI/Text.h>
#include <PerfUI/Slider.h>
#include <PerfUI/Checkbox.h>

void CreateMyCustomMenu(PerfUI::UIContext& context) {
    // 1. Create a top-level window
    auto* window = context.root()->add<PerfUI::Panel>();
    window->setSize({ 480.0f, 320.0f });
    window->setPosition({ 100.0f, 100.0f });
    window->setStyleProperty("background-color", PerfUI::Color(20, 24, 32, 240));
    window->setStyleProperty("border-color", PerfUI::Color(140, 160, 190, 200));
    window->setStyleProperty("border-width", 1.0f);
    window->setStyleProperty("padding", 16.0f);
    window->setLayoutDirection(PerfUI::FlexDirection::Column);

    // 2. Add Title Header
    auto* title = window->add<PerfUI::Text>("My Custom Modern Menu");
    title->setFontSize(20.0f);
    title->setColor(PerfUI::Color(255, 235, 175)); // Nordic Gold

    // 3. Add Interactive Controls
    auto* slider = window->add<PerfUI::Slider>(0.0f, 100.0f, 50.0f);
    slider->onValueChanged([](float val) {
        // Handle value change
    });

    auto* button = window->add<PerfUI::Button>("Confirm Action");
    button->onClick([]() {
        // Trigger action
    });
}
```

---

## 📂 Repository Structure

```text
PerfUI/
├── include/PerfUI/         # Public header files (No ImGui headers exposed!)
│   ├── Animation.h         # Tweening & easing helpers
│   ├── Button.h            # Button widget
│   ├── Checkbox.h          # Checkbox widget
│   ├── ComboBox.h          # Dropdown combo box widget
│   ├── ContextMenu.h       # Popup context menu
│   ├── ModalDialog.h       # Modal dialog window
│   ├── Panel.h             # Flexible container panel
│   ├── ProgressBar.h       # Progress bar widget
│   ├── ScrollView.h        # Scrollable container view
│   ├── Slider.h            # Numeric slider control
│   ├── TabBar.h            # Tab navigation widget
│   ├── Text.h              # Text display widget
│   ├── TextInput.h         # Text input box
│   ├── Theme.h             # Colors, metrics, and themes
│   ├── Toast.h             # Pop-up toast notifications
│   ├── UIContext.h         # Root framework context
│   └── UIElement.h         # Base retained element
├── src/                    # Implementation files
│   ├── backends/           # Render backends (ImGui, Mock, D3D11)
│   ├── core/               # Core runtime, context, memory lifecycle
│   ├── layout/             # Flexbox and box-model layout engine
│   ├── skyrim/             # DirectX 11 hook, SKSE plugin, audio bridge
│   └── widgets/            # Widget implementations
├── third_party/
│   └── imgui/              # Dear ImGui library (Omar Cornut - MIT License)
├── examples/               # Sandbox & sample mod implementations
├── docs/                   # Architecture, design specifications, and roadmap
├── CMakeLists.txt          # CMake configuration
├── xmake.lua               # XMake build script
├── LICENSE                 # GNU General Public License v3.0
└── README.md               # Project documentation
```

---

## 🛠️ Build Requirements

* **OS:** Windows 10 / 11 (64-bit)
* **Compiler:** Microsoft Visual C++ (MSVC) 2022 v143+ with `/std:c++23` support
* **Build System:** [XMake](https://xmake.io/) (Recommended) or [CMake](https://cmake.org/) (3.23+)
* **Dependencies:** DirectX 11 SDK (included with Windows SDK)

### Building with XMake (Fastest)

```powershell
# Clone the repository
git clone https://github.com/<your-username>/PerfUI.git
cd PerfUI

# Build Release mode
xmake f -m release
xmake -y
```

### Building with CMake

```powershell
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

---

## 🇷🇺 Описание проекта (Russian Overview)

**PerfUI** — это независимый, высокопроизводительный retained-mode UI-фреймворк на современном стандарте **C++23**, разработанный специально для создания модификаций и интерфейсов для игры *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition*, а также для автономных графических приложений на DirectX 11.

Фреймворк предоставляет авторам модов и разработчикам удобный объектно-ориентированный C++ API для построения плавных, отзывчивых и удобных для управления с геймпада внутриигровых меню: журналов заданий, кастомных HUD-полосок, экранов настроек, диалоговых окон и инвентарей.

### 🌟 Ключевые особенности

* **Дерево элементов (Retained-Mode):** Иерархический граф сцены (`UIElement`, `UIWindow`, `Panel`) с оптимизацией dirty-флагов — пересчёт стилей и геометрии происходит только при фактических изменениях.
* **Движок вёрстки (Flexbox & Box-Model):** Автоматическое распределение элементов, гибкие направления (`Row` / `Column`), адаптивное растяжение, отступы (`padding`, `margin`) и якоря (`anchors`).
* **Адаптация под геймпады (Controller-First):** Полноценная навигация по графу фокуса с помощью крестовины (D-Pad), стиков, клавиатуры и мыши с автоматическим поиском ближайших элементов.
* **Библиотека готовых виджетов:**
  * **Контейнеры:** `Panel` (панели), `ScrollView` (области прокрутки), `ModalDialog` (модальные окна), `ContextMenu` (всплывающие контекстные меню), `TabBar` (вкладки).
  * **Элементы управления:** `Button` (кнопки), `Checkbox` (флажки), `Slider` (ползунки), `ComboBox` (выпадающие списки), `TextInput` (ввод текста), `ProgressBar` (полосы прогресса).
  * **Информационные виджеты:** `Text` (форматированный текст), `Image` (текстуры/картинки), `Toast` (всплывающие уведомления).
* **Стилизация в эстетике Скайрима:** Встроенные темы оформления (тёмный сланец, пергамент, золото, серебряные окантовки, нордические орнаменты) и движок анимаций (плавное появление, пульсация, hover-эффекты).
* **Изоляция бэкенда:** Проект использует библиотеку [Dear ImGui](https://github.com/ocornut/imgui) **исключительно** как начальный фундамент растеризации вершинных буферов (`ImDrawList`). Внешний API `PerfUI` полностью независим и не подключает заголовочные файлы ImGui, что позволяет в будущем бесшовно заменить бэкенд на прямой D3D11/D3D12/Vulkan без переписывания пользовательского кода интерфейсов.

---

## 📜 License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.  
See the [LICENSE](LICENSE) file for the full text.

### Third-Party Licenses

* **[Dear ImGui](https://github.com/ocornut/imgui)**  
  Copyright (c) 2014-2026 Omar Cornut.  
  Licensed under the [MIT License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt).
* **[CommonLibSSE-ng](https://github.com/CharmedBaryon/CommonLibSSE-ng)**  
  Licensed under the [MIT License](https://github.com/CharmedBaryon/CommonLibSSE-ng/blob/main/LICENSE).
