<div align="center">

<img src="assets/banner.svg" alt="PerfUI" width="100%">

<p>
  <a href="#english">🇬🇧 English</a> ·
  <a href="#русский">🇷🇺 Русский</a>
</p>

<p>
  <img src="https://img.shields.io/github/v/release/PerfLite/PerfUI?style=flat-square" alt="release">
  <img src="https://img.shields.io/github/license/PerfLite/PerfUI?style=flat-square" alt="license">
  <img src="https://img.shields.io/github/languages/top/PerfLite/PerfUI?style=flat-square" alt="language">
  <img src="https://img.shields.io/github/stars/PerfLite/PerfUI?style=flat-square" alt="stars">
  <img src="https://img.shields.io/github/downloads/PerfLite/PerfUI/total?style=flat-square" alt="downloads">
</p>

<p>
  <a href="https://github.com/PerfLite/PerfUI/releases/latest"><b>⬇️ Download / Скачать</b></a> ·
  <a href="https://github.com/PerfLite/PerfUI/releases">📦 All releases / Все релизы</a>
</p>

</div>

## ⬇️ Download / Скачать

| File / Файл | Description / Описание | Link / Ссылка |
|---|---|---|
| `PerfUI-SKSE.zip` | Skyrim SE / AE Plugin & Assets · Плагин для Skyrim SE/AE | [Download](https://github.com/PerfLite/PerfUI/releases/latest/download/PerfUI-SKSE.zip) |
| `PerfUI-SDK.zip` | C++20 Headers, Libs & XML Markup Tools for Modders · SDK для разработчиков модов | [Download](https://github.com/PerfLite/PerfUI/releases/latest/download/PerfUI-SDK.zip) |

> Older versions and changelogs: [Releases](https://github.com/PerfLite/PerfUI/releases) ·  
> Старые версии и список изменений: [Релизы](https://github.com/PerfLite/PerfUI/releases)

---

## English

High-performance, retained-mode C++20 UI framework for *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition* and standalone DirectX 11 applications.

<p align="center">
  <img src="screenshots/2.png" alt="PerfUI Smart Compass & Quest Objective Overlay" width="49%" />
  <img src="screenshots/1.png" alt="PerfUI Status Bars & Character Stats Overlay" width="49%" />
</p>

### Features

- ⚡ **Retained-Mode Scene Graph:** Hierarchical DOM-like element tree (`UIElement`, `UIWindow`, `Panel`) with dirty-flag layout optimization (<0.2 ms per frame for 550+ active elements).
- 📐 **Flexbox & Stack Layout Engine:** Directional layouts (`Row` / `Column`), automatic flex-grow/shrink, alignment, padding, margins, and auto-sizing.
- 🎮 **Controller & Gamepad First:** Full 2D focus-graph navigation supporting D-Pad, Thumbsticks, Keyboard, and Mouse with automatic spatial focus transitions.
- 🗡️ **Authentic Nordic Sword Cursor:** Global hardware-accurate Skyrim sword cursor rendered in foreground draw list with soft drop shadows; hooks `CursorMenu` with zero Flash null-pointer dereferences or disappearance glitches.
- 🎨 **Native Vector SVG Graphics:** Embedded NanoSVG parser and rasterizer (`SvgImage` widget, `SvgService`) delivering crisp vector icons at any resolution without Chromium/CEF overhead.
- 🧩 **Comprehensive Modern Widget Library:**
  - **Containers & Windows:** `UIWindow`, `Panel`, `ScrollView`, `ModalDialog`, `ContextMenu`, `TabBar`, `Accordion`, `Sidebar`, `CategoryList`.
  - **Controls:** `Button`, `Checkbox`, `Slider`, `ComboBox`, `TextInput`, `ProgressBar`, `Switch`, `RadioButton`, `RadioGroup`, `KeybindSelector`, `SegmentedControl`.
  - **Data & Display:** `Text`, `TextArea`, `Image`, `SvgImage`, `Toast`, `Notification`, `ItemSlot`, `SearchableList`, `QuestTracker`, `MapCanvas` (radar & world modes), `Divider`, `Spacer`.
- 🎭 **Theme Tokens & Design System:** Built-in design tokens with 4 visual presets: `SkyrimClassic`, `NordicDark`, `GlassFrost`, `WarmParchment`.
- 🌐 **Cyrillic & Symbol Typography:** Native UTF-8 string support with Cyrillic glyph preloading and merged Segoe UI Symbol font for quest diamond markers, dingbats, and geometric symbols.
- 🔄 **Declarative XML Markup & Instant Hot-Reload:** Static library (`PerfUI_Markup`) allowing authors to declare UIs in XML and edit layouts live with instant state-preserving hot-reload without restarting the game.
- 🛡️ **Complete Backend Decoupling:** Dear ImGui is used strictly as an internal rasterization foundation (`ImDrawList`). The public API never exposes `<imgui.h>`, enabling future backend swaps (native D3D11, Vulkan, D3D12) with zero client code changes.

### Install

#### For Players & Mod Users:
1. Download `PerfUI-SKSE.zip` from [Releases](https://github.com/PerfLite/PerfUI/releases/latest).
2. Install via Mod Organizer 2 or Vortex.
3. Requires: Skyrim SE 1.5.97 or AE 1.6+, SKSE64, Address Library for SKSE Plugins.

#### For Mod Developers:
Include the repository as a submodule or link the SDK:

```
https://github.com/PerfLite/PerfUI
```

Include headers and link against `PerfUI.lib`:
```cpp
#include <PerfUI/PerfUI.h>
#include <PerfUI/UIWindow.h>
```

### Build

* **Compiler:** MSVC 2022 v143+ (C++20 `/std:c++20`), GCC 12+, Clang 15+
* **Build System:** [XMake](https://xmake.io/) (Recommended) or [CMake](https://cmake.org/) (3.20+)

#### Using XMake:

```powershell
git clone https://github.com/PerfLite/PerfUI.git
cd PerfUI
xmake build PerfUI
xmake run PerfUI_Test_Layout
xmake run PerfUI_Test_Independence
```

#### Using CMake:

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Documentation

* **Modder Integration Guide:** [English](docs/MODDER_GUIDE_EN.md) | [Русский](docs/MODDER_GUIDE.md)
* **Declarative XML Markup & Hot-Reload:** [English](docs/MARKUP_EN.md) | [Русский](docs/MARKUP.md)
* **Skyrim Input & Cursor Integration:** [English](docs/SKYRIM_INPUT_GUIDE_EN.md) | [Русский](docs/SKYRIM_INPUT_GUIDE.md)
* **MapCanvas Radar & World Widget:** [English](docs/MAPCANVAS_EN.md) | [Русский](docs/MAPCANVAS.md)
* **Architecture Overview:** [ARCHITECTURE.md](ARCHITECTURE.md)

### License

Licensed under the **MIT License** — see [LICENSE](LICENSE).

#### Third-Party Credits:
* **[Dear ImGui](https://github.com/ocornut/imgui)** (Omar Cornut) — [MIT](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)
* **[NanoSVG](https://github.com/memononen/nanosvg)** (Mikko Mononen) — [zlib](https://github.com/memononen/nanosvg/blob/master/LICENSE.txt)
* **[pugixml](https://github.com/zeux/pugixml)** (Arseny Kapoulkine) — [MIT](https://github.com/zeux/pugixml/blob/master/LICENSE.md)
* **[stb](https://github.com/nothings/stb)** (Sean Barrett) — [MIT / Public Domain](https://github.com/nothings/stb/blob/master/LICENSE)
* **[CommonLibSSE-ng](https://github.com/CharmedBaryon/CommonLibSSE-ng)** — [MIT](https://github.com/CharmedBaryon/CommonLibSSE-ng/blob/main/LICENSE)

<p align="right"><a href="#english">↑ Back to top</a> · <a href="#русский">Русский ↓</a></p>

---

## Русский

Высокопроизводительный retained-mode C++20 UI-фреймворк для *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition* и автономных графических приложений на DirectX 11.

<p align="center">
  <img src="screenshots/2.png" alt="PerfUI Smart Compass & Quest Objective Overlay" width="49%" />
  <img src="screenshots/1.png" alt="PerfUI Status Bars & Character Stats Overlay" width="49%" />
</p>

### Возможности

- ⚡ **Дерево элементов (Retained-Mode):** Иерархический граф сцены (`UIElement`, `UIWindow`, `Panel`) с dirty-флагами — вычисления геометрии и Flexbox происходят только при фактических изменениях (<0.2 мс на кадр при 550+ элементах).
- 📐 **Движок вёрстки (Flexbox & Box-Model):** Направления (`Row` / `Column`), адаптивное растяжение (`flex-grow`), выравнивание, отступы (`padding`, `margin`) и авто-размеры (`Auto`).
- 🎮 **Адаптация под геймпады (Controller-First):** Полноценная 2D-навигация по графу фокуса с помощью крестовины (D-Pad), стиков, клавиатуры и мыши с автоматическим пространственным переходом.
- 🗡️ **Аутентичный меч-курсор Скайрима:** Собственный векторный курсор в foreground-слое с мягкими тенями; хук `CursorMenu` исключает вылеты в Flash runtime и исчезновение курсора при диалогах.
- 🎨 **Нативная векторная графика SVG:** Встроенный легковесный парсер и растеризатор NanoSVG (`SvgImage`, `SvgService`) — чёткие векторные иконки в любом разрешении без оверхеда браузерных движков.
- 🧩 **Большая библиотека современных виджетов:**
  - **Окна и контейнеры:** `UIWindow`, `Panel`, `ScrollView`, `ModalDialog`, `ContextMenu`, `TabBar`, `Accordion`, `Sidebar`, `CategoryList`.
  - **Элементы управления:** `Button`, `Checkbox`, `Slider`, `ComboBox`, `TextInput`, `ProgressBar`, `Switch`, `RadioButton`, `RadioGroup`, `KeybindSelector`, `SegmentedControl`.
  - **Информационные виджеты:** `Text`, `TextArea`, `Image`, `SvgImage`, `Toast`, `Notification`, `ItemSlot`, `SearchableList`, `QuestTracker`, `MapCanvas` (радар и карта), `Divider`, `Spacer`.
- 🎭 **Стилизация и темы:** Дизайн-токены и 4 встроенные темы (`SkyrimClassic`, `NordicDark`, `GlassFrost`, `WarmParchment`), движок плавных анимаций.
- 🌐 **Поддержка кириллицы и значков:** Встроенная загрузка кириллических диапазонов глифов и объединённый шрифт Segoe UI Symbol для маркеров квестов и геометрических значков.
- 🔄 **Декларативный XML Markup & Hot-Reload:** Описание интерфейсов в XML с горячей перезагрузкой прямо в игре без перезапуска (`PerfUI_Markup`).
- 🛡️ **Полная изоляция бэкенда:** Библиотека ImGui изолирована внутри реализации (`ImDrawList`). Внешний API фреймворка на 100% независим от `<imgui.h>`.

### Установка

#### Для игроков:
1. Скачайте `PerfUI-SKSE.zip` из раздела [Релизы](https://github.com/PerfLite/PerfUI/releases/latest).
2. Установите через Mod Organizer 2 или Vortex.
3. Требования: Skyrim SE 1.5.97 или AE 1.6+, SKSE64, Address Library for SKSE Plugins.

#### Для разработчиков модов:
Подключите репозиторий или используйте SDK:

```
https://github.com/PerfLite/PerfUI
```

Подключите заголовки и библиотеку `PerfUI.lib`:
```cpp
#include <PerfUI/PerfUI.h>
#include <PerfUI/UIWindow.h>
```

### Сборка

* **Компилятор:** MSVC 2022 v143+ (C++20 `/std:c++20`), GCC 12+, Clang 15+
* **Система сборки:** [XMake](https://xmake.io/) (рекомендуется) или [CMake](https://cmake.org/) (3.20+)

#### Сборка через XMake:

```powershell
git clone https://github.com/PerfLite/PerfUI.git
cd PerfUI
xmake build PerfUI
xmake run PerfUI_Test_Layout
xmake run PerfUI_Test_Independence
```

#### Сборка через CMake:

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Документация

* **Руководство интеграции для мододелов:** [Русский](docs/MODDER_GUIDE.md) | [English](docs/MODDER_GUIDE_EN.md)
* **XML Markup и горячая перезагрузка:** [Русский](docs/MARKUP.md) | [English](docs/MARKUP_EN.md)
* **Ввод, мышь и синхронизация курсора:** [Русский](docs/SKYRIM_INPUT_GUIDE.md) | [English](docs/SKYRIM_INPUT_GUIDE_EN.md)
* **Виджет радара и карты MapCanvas:** [Русский](docs/MAPCANVAS.md) | [English](docs/MAPCANVAS_EN.md)
* **Архитектура фреймворка:** [ARCHITECTURE.md](ARCHITECTURE.md)

### Лицензия

MIT — см. [LICENSE](LICENSE).

#### Сторонние библиотеки:
* **[Dear ImGui](https://github.com/ocornut/imgui)** (Omar Cornut) — [MIT](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)
* **[NanoSVG](https://github.com/memononen/nanosvg)** (Mikko Mononen) — [zlib](https://github.com/memononen/nanosvg/blob/master/LICENSE.txt)
* **[pugixml](https://github.com/zeux/pugixml)** (Arseny Kapoulkine) — [MIT](https://github.com/zeux/pugixml/blob/master/LICENSE.md)
* **[stb](https://github.com/nothings/stb)** (Sean Barrett) — [MIT / Public Domain](https://github.com/nothings/stb/blob/master/LICENSE)
* **[CommonLibSSE-ng](https://github.com/CharmedBaryon/CommonLibSSE-ng)** — [MIT](https://github.com/CharmedBaryon/CommonLibSSE-ng/blob/main/LICENSE)

<p align="right"><a href="#русский">↑ Наверх</a> · <a href="#english">English ↑</a></p>
