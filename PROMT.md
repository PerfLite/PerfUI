Ты разрабатываешь новый open-source UI framework для Skyrim SE/AE под названием **PerfUI**.

## Главная идея

PerfUI должен стать самостоятельным современным UI-фреймворком для Skyrim, предназначенным для создания полноценных игровых интерфейсов: Journal, Main Menu, Inventory, Character, Map, HUD и других меню.

**Dear ImGui используется только как первый backend/rendering foundation.**

PerfUI НЕ должен быть простой обёрткой над ImGui.

Архитектура должна позволять в будущем полностью заменить ImGui на собственный renderer без переписывания пользовательских UI и компонентов.

Целевая архитектура:

```text
Skyrim
   ↓
SKSE / CommonLibSSE
   ↓
PerfUI
   ├── Core
   ├── Widget System
   ├── Layout
   ├── Styling
   ├── Animation
   ├── Input
   ├── Navigation
   ├── Skyrim Integration
   └── Backend
          ↓
        ImGui
          ↓
        D3D11
```

## Ключевой принцип

Публичный API PerfUI не должен зависеть от ImGui.

Плохо:

```cpp
ImGui::Begin(...)
ImGui::Button(...)
ImGui::SameLine(...)
```

Хорошо:

```cpp
PerfUI::Window(...)
PerfUI::Button(...)
PerfUI::Panel(...)
PerfUI::List(...)
```

Пользователь PerfUI вообще не должен знать, какой renderer используется внутри.

---

# 1. Core

Создай базовую архитектуру:

```text
PerfUI/
├── include/
│   └── PerfUI/
├── src/
│   ├── core/
│   ├── widgets/
│   ├── layout/
│   ├── style/
│   ├── animation/
│   ├── input/
│   ├── navigation/
│   ├── skyrim/
│   └── backends/
├── third_party/
│   └── imgui/
├── examples/
└── tests/
```

Core должен содержать:

* UIContext
* UIElement
* UIWindow
* UIView
* Widget
* Event system
* lifecycle
* update/render pipeline

Предусмотреть parent/child hierarchy:

```text
UIContext
 └── Root
      ├── Window
      │    ├── Panel
      │    ├── Text
      │    └── Button
      └── Window
```

---

# 2. Retained-mode UI

Хотя ImGui является immediate-mode библиотекой, PerfUI должен предоставлять **retained-mode UI tree**.

UI должен существовать как дерево объектов:

```cpp
auto journal = ui.createWindow("Journal");

auto sidebar = journal->add<Panel>("Sidebar");

auto quests = sidebar->add<QuestList>("QuestList");

auto details = journal->add<QuestDetails>("Details");
```

Не заставляй разработчика каждый кадр вручную пересоздавать UI.

PerfUI сам управляет:

* lifecycle;
* layout;
* state;
* events;
* visibility;
* focus;
* rendering.

---

# 3. Layout system

Не использовать ручное позиционирование как основной способ создания интерфейса.

Нужна собственная layout-система с поддержкой:

* width / height;
* min/max size;
* padding;
* margin;
* alignment;
* anchoring;
* horizontal/vertical layout;
* flexbox-подобного поведения;
* grid в будущем;
* percentage dimensions;
* automatic sizing;
* clipping;
* scrolling.

Пример:

```cpp
panel->layout().direction(LayoutDirection::Vertical);
panel->layout().padding(20);
panel->layout().gap(10);
```

Архитектуру layout сделать независимой от ImGui.

---

# 4. Styling

Не expose'ить наружу ImGuiStyle.

Создать собственную систему стилей:

```cpp
Style style;

style.background = Color(...);
style.borderRadius = 12;
style.padding = 16;
style.opacity = 0.95;
style.shadow = ...;
```

Поддержать:

* background;
* foreground;
* border;
* border radius;
* opacity;
* padding;
* margin;
* font;
* font size;
* shadows;
* gradients;
* transitions;
* states;
* hover;
* pressed;
* focused;
* disabled.

Позже предусмотреть theme system:

```text
themes/
├── Skyrim/
├── Modern/
└── Cyberpunk/
```

---

# 5. Widget system

Создать базовые компоненты:

```text
Widget
├── Panel
├── Text
├── Image
├── Button
├── IconButton
├── List
├── ScrollView
├── Tabs
├── ProgressBar
├── Slider
├── Checkbox
├── Dropdown
├── Tooltip
└── Input
```

Но архитектура должна позволять модам создавать собственные widgets:

```cpp
class QuestCard : public PerfUI::Widget
{
};
```

---

# 6. Input

PerfUI должен иметь собственную abstraction layer:

```text
Keyboard
Mouse
Gamepad
Controller
```

Не отдавать пользователю ImGui input API.

Нужны:

* focus;
* keyboard navigation;
* mouse interaction;
* gamepad navigation;
* directional navigation;
* submit;
* cancel;
* focus transitions;
* input capture;
* input propagation;
* modal windows.

Особенно важно сделать нормальную controller navigation для Skyrim.

---

# 7. Animation

Создать независимую animation system:

```cpp
widget->animate()
    .opacity(0.0f, 1.0f)
    .duration(200ms)
    .ease(Easing::OutCubic);
```

Предусмотреть:

* opacity;
* position;
* scale;
* rotation;
* size;
* color;
* custom properties.

Animation system не должен зависеть от ImGui.

---

# 8. Rendering abstraction

Создать интерфейс:

```cpp
class UIRenderBackend
{
public:
    virtual void beginFrame() = 0;
    virtual void drawText(...) = 0;
    virtual void drawRect(...) = 0;
    virtual void drawImage(...) = 0;
    virtual void drawRoundedRect(...) = 0;
    virtual void endFrame() = 0;
};
```

Первый backend:

```text
ImGuiBackend
```

Но PerfUI Core никогда не должен напрямую зависеть от ImGui.

Все зависимости должны находиться внутри:

```text
src/backends/imgui/
```

Цель: в будущем иметь возможность заменить его:

```text
ImGuiBackend
NativeD3D11Backend
```

без изменения пользовательского UI.

---

# 9. Skyrim integration

Создать отдельный слой:

```text
PerfUI::Skyrim
```

Он будет взаимодействовать с:

* SKSE;
* CommonLibSSE;
* Skyrim UI/menu system;
* quests;
* actors;
* inventory;
* player;
* world;
* events.

Начать с Quest API.

Например:

```cpp
auto quests = Skyrim::Quests::active();

for (auto& quest : quests)
{
    quest.name();
    quest.description();
    quest.stage();
    quest.objectives();
    quest.isCompleted();
    quest.isActive();
}
```

Не смешивать Skyrim API с базовыми UI widgets.

---

# 10. Первый реальный тест

После создания базового framework сделать демонстрационный **Journal prototype**.

Он должен содержать:

```text
┌────────────────────────────────────────────┐
│ JOURNAL                                    │
├───────────────┬────────────────────────────┤
│ Main Quests   │ The Way of the Voice       │
│ Side Quests   │                            │
│ Miscellaneous │ Objectives                │
│ Completed     │ ✓ Speak to Arngeir        │
│               │ ○ Learn the Whirlwind     │
│               │                            │
└───────────────┴────────────────────────────┘
```

Journal должен демонстрировать:

* layout;
* scrolling;
* selection;
* focus;
* controller navigation;
* animations;
* styling;
* quest data;
* custom widgets.

---

# 11. Главное архитектурное требование

НЕ превращай проект в:

```text
PerfUI
   ↓
ImGui wrapper
```

Нужен:

```text
PerfUI
   ↓
UI abstraction
   ↓
ImGui backend
```

ImGui должен быть заменяемым.

Также НЕ надо сразу пытаться реализовать абсолютно всё.

Сначала сделать минимальный вертикальный slice:

```text
SKSE plugin
    ↓
PerfUI Context
    ↓
Window
    ↓
Panel
    ↓
Text/Button
    ↓
Layout
    ↓
ImGui backend
    ↓
D3D11
```

После того как это работает, постепенно добавлять остальные системы.

---

# 12. Development rules

Перед реализацией:

1. Изучи актуальный Dear ImGui API и backend architecture.
2. Изучи CommonLibSSE и актуальный SKSE plugin structure.
3. Определи, как безопасно интегрировать rendering в Skyrim D3D11.
4. Изучи существующие Skyrim UI frameworks, включая PrismaUI, Meridian UI и PangUI, но НЕ копируй их API.
5. Составь архитектурный документ `ARCHITECTURE.md`.
6. Объясни, какие части относятся к Core, а какие к backend.
7. Только после этого начинай реализацию.

Не делай преждевременную оптимизацию.

Не добавляй зависимости без необходимости.

Не связывай Core с ImGui.

Каждую новую систему проектируй так, чтобы она могла существовать независимо от конкретного renderer.

## Конечная цель

Создать не просто красивый Journal, а основу для нового поколения Skyrim UI:

```text
PerfUI
 ├── Journal
 ├── Main Menu
 ├── Inventory
 ├── Character
 ├── Map
 ├── HUD
 └── Third-party Mods
```

Другие моддеры должны впоследствии иметь возможность подключить PerfUI и создавать собственные интерфейсы, не работая напрямую с ImGui, Scaleform/SWF или браузером.


# 13. Поэтапный план разработки

Разработка должна идти строго поэтапно. Не пытайся реализовать весь framework сразу.

Каждый этап должен заканчиваться рабочим состоянием проекта.

---

## Phase 0. Исследование и архитектура

Цель: понять окружение до написания большого количества кода.

Изучить:

* актуальный Dear ImGui;
* ImGui backends;
* D3D11;
* CommonLibSSE;
* SKSE;
* Skyrim menu lifecycle;
* существующие Skyrim UI frameworks;
* PrismaUI;
* Meridian UI;
* PangUI.

Создать:

```text
ARCHITECTURE.md
ROADMAP.md
DESIGN.md
```

Определить:

* lifecycle PerfUI;
* rendering pipeline;
* input pipeline;
* UI tree;
* ownership;
* memory management;
* event system;
* backend abstraction;
* Skyrim integration boundary.

На этом этапе **не создавать полноценные widgets**.

---

# Phase 1. Минимальный SKSE + ImGui prototype

Цель: получить первое окно внутри Skyrim.

Сделать:

```text
SKSE
 ↓
PerfUI
 ↓
ImGui
 ↓
D3D11
```

Минимальный функционал:

* загрузка SKSE plugin;
* создание PerfUI context;
* initialization;
* shutdown;
* frame begin;
* frame end;
* ImGui backend;
* D3D11 rendering;
* базовый input;
* открытие/закрытие тестового UI.

Результат:

```text
Нажал hotkey
      ↓
Открылось окно PerfUI
      ↓
Оно корректно рисуется в Skyrim
      ↓
Закрылось без краша
```

**Не переходить дальше, пока lifecycle и rendering не стабильны.**

---

# Phase 2. Backend abstraction

Цель: сделать ImGui заменяемым.

Создать:

```cpp
class UIRenderBackend;
class UIInputBackend;
```

и:

```text
PerfUI Core
     │
     ├── Renderer
     └── Input
          │
          ↓
       ImGuiBackend
```

Core не должен включать ImGui headers.

Только backend имеет право обращаться к:

```cpp
ImGui::
```

Результат:

```text
PerfUI Core
      ↓
Abstract Backend
      ↓
ImGui
```

---

# Phase 3. UI tree

Цель: перестать писать UI непосредственно через ImGui.

Создать:

```text
UIContext
 └── Root
      ├── Window
      ├── Window
      └── Window
```

Базовые классы:

```text
UIElement
Widget
Container
Window
```

Поддержать:

* parent;
* child;
* visibility;
* enabled;
* lifecycle;
* IDs;
* state;
* traversal.

Пример:

```cpp
auto window = ui.createWindow("Test");

auto panel = window->add<Panel>();

panel->add<Text>("Hello");
panel->add<Button>("Click");
```

Результат: пользователь PerfUI больше не взаимодействует с ImGui напрямую.

---

# Phase 4. Layout engine

Цель: создать собственную систему layout.

Начать с:

```text
Size
Position
Padding
Margin
Alignment
```

Затем:

```text
Horizontal layout
Vertical layout
Anchoring
Min/max size
Auto size
Clipping
Scrolling
```

После этого добавить:

```text
Flexbox-like layout
```

Grid оставить на более поздний этап.

Каждый layout должен работать независимо от renderer.

Результат:

```text
Panel
 ├── Header
 ├── Content
 └── Footer
```

без ручного `SetCursorPos()`.

---

# Phase 5. Core widgets

Реализовать минимальный набор:

```text
Panel
Text
Image
Button
IconButton
List
ScrollView
Checkbox
Slider
ProgressBar
Tabs
Tooltip
```

Каждый widget должен:

* иметь собственный state;
* поддерживать layout;
* поддерживать style;
* генерировать события;
* работать с keyboard/mouse;
* быть пригодным для наследования.

---

# Phase 6. Styling system

Цель: полностью отделить внешний вид от ImGui.

Создать:

```text
Style
StyleState
Theme
ThemeManager
```

Поддержать:

```text
background
foreground
border
border radius
padding
margin
opacity
font
font size
shadow
gradient
```

Состояния:

```text
normal
hovered
pressed
focused
disabled
selected
```

Создать первую тестовую тему:

```text
PerfUI Default
```

Она не должна визуально копировать стандартный ImGui.

---

# Phase 7. Input и navigation

Цель: сделать UI пригодным для реального Skyrim menu.

Реализовать abstraction:

```text
Keyboard
Mouse
Gamepad
```

Добавить:

* focus;
* focus order;
* directional navigation;
* keyboard navigation;
* controller navigation;
* input capture;
* modal input;
* input propagation;
* cancel;
* submit.

Особенно тщательно протестировать:

```text
↑
↓
←
→
A / Enter
B / Escape
```

для gamepad.

---

# Phase 8. Animation system

Добавить:

```text
Animation
Timeline
Easing
Transition
```

Поддержать:

```text
opacity
position
scale
rotation
size
color
```

Пример:

```cpp
widget->animate()
    .opacity(0.0f, 1.0f)
    .duration(200ms)
    .ease(Easing::OutCubic);
```

Animation system не должен зависеть от ImGui.

---

# Phase 9. Rendering improvements

После того как базовая архитектура работает, улучшить rendering backend.

Добавить:

* rounded rectangles;
* gradients;
* shadows;
* clipping;
* image rendering;
* texture atlas;
* custom fonts;
* SVG/icon support;
* masking;
* custom shaders.

На этом этапе определить, какие функции лучше реализовать через ImGui draw lists, а какие через собственный D3D11 renderer.

Не переписывать renderer целиком без необходимости.

---

# Phase 10. Skyrim integration

Создать:

```text
PerfUI::Skyrim
```

Начать с событий:

```text
MenuOpen
MenuClose
Update
Input
QuestChanged
```

Затем API:

```text
Quest
Objective
Actor
Inventory
Player
World
Map
```

Первым сделать:

```text
Quest API
```

Например:

```cpp
auto quests = Skyrim::Quests::active();

for (auto& quest : quests)
{
    quest.name();
    quest.description();
    quest.stage();
    quest.objectives();
}
```

Skyrim-specific code не должен проникать в generic UI Core.

---

# Phase 11. Journal prototype

Теперь построить первый полноценный интерфейс.

Структура:

```text
Journal
├── Sidebar
│   ├── Main Quests
│   ├── Side Quests
│   ├── Miscellaneous
│   └── Completed
│
└── Quest Details
    ├── Title
    ├── Description
    ├── Objectives
    └── Status
```

Journal должен использовать только публичный PerfUI API.

Внутри Journal **не должно быть прямых вызовов ImGui**.

Это главный архитектурный тест.

---

# Phase 12. Stress test

Проверить framework на сложных сценариях:

* 100+ widgets;
* длинный список квестов;
* большое количество текстовых элементов;
* scrolling;
* animations;
* открытие/закрытие меню;
* быстрое переключение экранов;
* keyboard;
* gamepad;
* потеря/возврат focus;
* смена разрешения;
* alt-tab;
* загрузка/выгрузка игры;
* несколько одновременно существующих UI windows.

Проверить:

```text
CPU
GPU
Memory
Frame time
Input latency
```

Не оптимизировать вслепую. Сначала измерить.

---

# Phase 13. Main Menu

После успешного Journal реализовать:

```text
Main Menu
├── Continue
├── New Game
├── Load
├── Settings
└── Quit
```

Цель этого этапа: проверить, насколько PerfUI подходит для UI, который живёт непосредственно в menu lifecycle Skyrim.

---

# Phase 14. Developer API

Сделать framework удобным для других моддеров.

Документировать:

```text
Getting Started
Creating Widgets
Layouts
Styling
Themes
Input
Gamepad
Animations
Events
Skyrim API
```

Добавить:

```text
examples/
```

с маленькими примерами.

Например:

```cpp
auto window = PerfUI::CreateWindow("Example");

window->add<Text>("Hello Skyrim");
window->add<Button>("Click");
```

---

# Phase 15. Backend independence test

Создать экспериментальный второй backend или mock backend.

Например:

```text
PerfUI
├── ImGuiBackend
└── TestBackend
```

TestBackend не обязан полноценно рендерить UI.

Его задача: доказать, что Core действительно не зависит от ImGui.

Если удаление ImGui из Core ломает архитектуру, исправить abstraction.

---

# Phase 16. PerfUI SDK

После стабилизации API разделить проект на:

```text
PerfUI Core
PerfUI Widgets
PerfUI Skyrim
PerfUI ImGui Backend
PerfUI SDK
```

Другие моддеры должны иметь возможность подключить framework и написать UI без знания внутренней архитектуры.

---

# Phase 17. Долгосрочное развитие

После стабильного SDK можно рассматривать:

```text
Custom renderer
Advanced text engine
GPU effects
Grid layout
Virtualized lists
Declarative UI
UI inspector
Live reload
Visual UI editor
Debug overlay
Performance profiler
```

Особенно интересны:

### UI Inspector

Позволяет видеть:

```text
Root
 └── Journal
      ├── Sidebar
      │    └── QuestList
      └── QuestDetails
```

и выбирать элементы прямо в игре.

### Live reload

Изменение UI/стилей без перезапуска Skyrim.

### UI profiler

Показывать:

```text
Widget count
Draw calls
Layout time
Render time
Animation time
Memory usage
```

---

# Правило завершения каждого этапа

После каждой Phase:

1. Проект должен собираться.
2. Skyrim должен запускаться.
3. Не должно быть известных критических crashes.
4. Должен существовать минимальный working example.
5. Архитектурные изменения должны быть задокументированы.
6. Не переходить к следующей Phase, если предыдущая фундаментальная система нестабильна.

Не реализовывать Phase 10–17 до того, как Phase 1–4 доказали жизнеспособность архитектуры.

Главная цель первых этапов:

**сначала доказать renderer → затем UI tree → затем layout → затем widgets → затем Skyrim integration.**

Не начинать с Journal только потому, что он является конечной целью. Journal должен стать первым большим потребителем уже проверенного PerfUI.

