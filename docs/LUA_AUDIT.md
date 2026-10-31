# Аудит: поддержка Lua в PerfUI (Этап 0)

Дата: 2026-10-31. Ветка `main`, коммит `babe5d0`, локальный тег `before-lua` создан на этом коммите.
Рабочее дерево: изменений кода нет; единственный незакоммиченный файл — сам `SPEC_Lua.md` (untracked), к заданию не относится.

## Базовый замер (для сравнения по этапам)

| Метрика | Значение |
| :--- | :--- |
| `PerfUI.dll` (Release, x64) | **1 846 784 байт** |
| Полная сборка с нуля (`PerfUI` target) | **89 с** (xmake: 88.7 с) |
| Каталог сборки | `..\.builds\PerfUI-release` (вне репозитория; `build/` не используется) |
| Конфигурация | `xmake f -p windows -m release`; CommonLibSSE-NG берётся из xmake-пакета (предсобранный `.lib`) |

После сборки `after_build` обновляет `sdk/lib/PerfUI.dll` и `sdk/include/PerfUI/*` (каталог `sdk/` git-игнорируется).

---

## 1. Как `PerfUI_Markup` привязывает колбэки и куда встроить Lua

**Существующий механизм.** Колбэки — обычная карта `CallbackMap m_callbacks` в загрузчике (`include/PerfUI/Markup/MarkupLoader.h:26,48`), заполняется через `MarkupLoader::bindCallback(name, cb)` (`src/markup/MarkupLoader.cpp:104-106`).

**Что реально привязывается сейчас — только `onClick` у `Button`.** В `MarkupLoader::setupElementProperties` (`src/markup/MarkupLoader.cpp:173-188`):
- `attrs["onClick"]` ищется в `m_callbacks`;
- если найден — `btn->onClick(cb)`;
- если **не найден** — предупреждение «has unbound callback» и заглушка `btn->onClick([]{})` (`:183-185`).

Ветка `:183-185` — готовая точка входа для Lua: «не нашли в C++ → спросить у резолвера».

**Пробел, который надо учесть на Этапе 2.** Атрибуты `onToggle`, `onValueChanged`, `onSelectionChanged`, `onTabChanged`, `onTextChanged`, `onSubmit` объявлены «известными» в `isKnownWidgetAttribute` (`src/markup/MarkupLoader.cpp:43-55`), но **нигде не биндятся** — поиск по `src/markup/` не находит их обработки в `WidgetRegistry.cpp`. Сегодня они молча игнорируются (варнинга нет, потому что атрибут «известен»). Для Lua нужно добавить их привязку, иначе `onChange`-сценарии из ТЗ работать не будут.

**Заготовки.** Атрибут `bind=` — заглушка с варнингом (`MarkupLoader.cpp:190-194`); `BindingRegistry::registerBinding` — no-op заглушка (`include/PerfUI/Markup/MarkupTypes.h:22-35`). Корневой узел `<UI>` читается без атрибутов (`MarkupLoader.cpp:221-230`), поэтому `<UI script="main.lua">` сегодня просто игнорируется — это место для разбора `script`.

**Предложение (без Lua в Markup).** Небольшой интерфейс в Markup:

```cpp
class ICallbackResolver {
public:
    virtual ~ICallbackResolver() = default;
    virtual bool has(std::string_view name) const = 0;
    virtual bool call(std::string_view name) = 0;                       // onClick, onFocus
    virtual bool callNumber(std::string_view name, double value) = 0;   // onToggle(bool), onValueChanged(float)
    virtual bool callString(std::string_view name, std::string_view value) = 0; // onSubmit, onTextChanged
};
```

`MarkupLoader::setCallbackResolver(ICallbackResolver*)` — единственное добавление. Порядок вызова: `m_callbacks` (C++) → резолвер (Lua) → варнинг. Существующие сигнатуры не меняются. Реализация резолвера — в `PerfUI_Lua`; `PerfUI_Markup` про Lua не знает.

---

## 2. Поиск элементов по имени и проверка живости

**Имена.** `name` — общий атрибут разметки (`src/markup/MarkupLayoutParser.cpp:183,299`), применяется к `UIElement::name` (`include/PerfUI/UIElement.h:69-71`).

**Поиск.** `MarkupLoader::findByName` — DFS от корня документа, первый совпавший (`MarkupLoader.cpp:61-70,117-122`). Глобального индекса «имя → элемент» нет ни в Markup, ни в `UIContext`. Уникальность имён не проверяется: два элемента с одинаковым `name` найдутся по первому. Для Lua: искать в поддереве документа, привязанного к хосту, и предупреждать о дубликатах.

**Живость.** Готовый механизм уже есть:
- `ElementId` — глобальный атомарный счётчик, id не переиспользуются (`src/core/UIElement.cpp:10-13`);
- `UIElement::aliveToken()` — `shared_ptr<bool>`, обнуляется в деструкторе (`include/PerfUI/UIElement.h:125,190`; `src/core/UIElement.cpp:21-25`);
- `WeakElementRef` поверх указателя и токена: `get()/isValid()/operator bool` (`include/PerfUI/Animation.h:235-246`).

`UIContext::notifyElementDestroyed` очищает только hover/press/focus (`src/core/UIContext.cpp:535-545`), скриптам ничего не сообщает. Значит, ручка обязана проверять себя сама.

**Ручка Lua (v1):** `{ ElementId id; std::string name; WeakElementRef ref; }` — без сырых указателей. Перед каждым методом: `ref.isValid()`; если элемент мёртв — предупреждение в лог, вызов возвращает `nil/false`, падения нет. Хот-релоад пересобирает поддерево (`MarkupLoader.cpp:312-320`), старые ручки автоматически мертвы, скрипт должен сделать `ui.find` заново.

---

## 3. Что безопасно открывать скриптам

**Общее для всех элементов** (`UIElement`):
- чтение: `name()`, `id()`, `isVisible()`, `isEnabled()`, `isFocused()`, `opacity()`, `bounds()/position()/size()`, `currentState()`, `tooltip()`;
- запись (v1): `setVisible`, `setEnabled`, `setOpacity`.

**Виджеты (методы уже существуют):**

| Виджет | Чтение | Запись |
| :--- | :--- | :--- |
| `Text` | `text()` | `text(string)` (`Text.h:16-17`) |
| `Button` | `text()/label()` | `text(string)` (`Button.h:18-22`) |
| `Slider` | `value()`, `min()`, `max()` | `value(float)` (`Slider.h:24-31`) |
| `Checkbox` | `isChecked()` | `checked(bool)` (`Checkbox.h:18-19`) |
| `ComboBox` | `selectedIndex()`, `selectedText()` | `selectIndex(size_t)` (`ComboBox.h:17-22`) |
| `ProgressBar` | `progress()/value()` | `progress(float)` (`ProgressBar.h:45-52`) |
| `TabBar` | `selectedTab()` | `selectTab(size_t)` (`TabBar.h:23-24`) |
| `ScrollView` | `scrollOffset()`, `contentHeight()` | `scrollTo/scrollBy` (`ScrollView.h:14-16`) |
| `TextInput` | `text()`, `placeholder()` | `text(string)` (`TextInput.h:18-19`) |

**Тема — только чтение:** `Theme::Current()` даёт 12 именованных токенов (`include/PerfUI/Theme.h:30-45`); менять тему из скрипта нельзя (`Theme::MutableCurrent`, `SetCurrent`, `ApplyPreset` не открывать).

**Не открывать в v1:**
- структурные операции: `add/addChild/removeChild/clearChildren` — ломают хот-релоад и состояние Markup;
- `layout()` и `style()` — конфликт с движком раскладки и разметкой;
- `animate()/spring()` — отложить до стабилизации API;
- текстуры/SVG, хоткеи, оверлеи, создание окон — это C++-слой мода;
- любые указатели/ссылки на игровые объекты (в т.ч. через `registerModule` — только кеш-данные);
- фокус (`UIContext::setFocus`) — можно добавить позже, если понадобится навигация из скрипта.

---

## 4. SDK/ABI: доступ к `ScriptHost` без утечки Lua

**Сейчас.** `IPerfUI_v1` + `IPerfUI_v2` (v2 = v1 + `SetCaptureKeyboard`, `include/PerfUI/PerfUIApi.h:57-64`); `ClientABIInfo { structSize, apiStructSize, mscVer, iteratorDebugLevel }`; сервер `RequestPluginAPIEx` принимает v1/v2 и сверяет размеры (`src/skyrim/Plugin.cpp:258-307`).

**Замечание по безопасности ABI (починить до v3):** проверка `iteratorDebugLevel` читает поле по смещению 12 **до** проверки `structSize` (`Plugin.cpp:282` против guard на `:294`). Для клиента со старым 12-байтным `ClientABIInfo` это чтение за границей структуры. Порядок guard'ов надо поменять: сначала `structSize`, затем поля.

**План (Этап 3):**
```cpp
namespace PerfUI { class ScriptHost; }

struct IPerfUI_v3 {
    IPerfUI_v2 v2;
    ScriptHost* (*CreateScriptHost)(const char* modName, unsigned long luaApiVersion);
    void        (*DestroyScriptHost)(ScriptHost* host);
};
```
- `class ScriptHost;` — forward declaration, заголовки Lua в SDK не попадают.
- Публичный заголовок `PerfUI/Lua/ScriptHost.h` (из `modules/lua/include`) кладётся в SDK; внутри — только STL/`std::function`, без `lua_State*`.
- `PERFUI_ENABLE_LUA=OFF` → API v3 не выдаётся (`RequestPluginAPIEx(3)` → `nullptr`), v1/v2 работают как раньше.
- Клиентский хелпер `Client::CreateScriptHost(name)` сверяет `PERFUI_LUA_API_VERSION` и возвращает `nullptr` при несовпадении.
- Границы ABI те же, что уже действуют для `RegisterHotkey(std::function)`: MSVC v143, `/MD`, одинаковый `_ITERATOR_DEBUG_LEVEL`; `lua_State*` через границу DLL не передаётся.
- Владение: хосты принадлежат `PerfUI_Lua`; `DestroyScriptHost` или выгрузка контекста гасят их; мод не хранит указатели после destroy.

---

## 5. Структура, цели сборки, риски

**Файлы (предложение):**
```
modules/lua/include/PerfUI/Lua/   ScriptHost.h, ScriptTypes.h (ScriptLimits/ScriptArgs/ScriptStatus), ModuleBuilder.h
modules/lua/src/                  ScriptHost.cpp, LuaSandbox.cpp (_ENV, require), LuaUiApi.cpp, LuaLog.cpp, CallbackResolver.cpp
modules/lua/tests/                ScriptHostTest.cpp, LuaUiApiTest.cpp
third_party/lua/                  Lua 5.4.x из upstream (MIT), без lua.c/luac.c; собирается как C
```

**Цели.** `PerfUI_Lua` (static) в CMake и xmake; `PerfUI.dll` линкует её при `PERFUI_ENABLE_LUA=ON`. Опция CMake `PERFUI_ENABLE_LUA` (default ON) и xmake-опция с тем же смыслом. `PerfUI_Core`/`PerfUI_Markup` от Lua не зависят: в Markup добавляется только `ICallbackResolver`.

**Тесты.** CMake — `add_test` по образцу существующих (`CMakeLists.txt:358-366`). В xmake `add_tests` сейчас **не используется** (тесты — отдельные target'ы); для новых тестов либо добавить `add_tests`, либо сохранить текущий стиль и не разъезжаться между CMake/xmake — предложение: завести `add_tests` именно для `PerfUI_Lua`-тестов и отметить это в отчёте Этапа 1.

**CI.** Сейчас только `windows-latest` + MSVC (`.github/workflows/build.yml`). Требование ТЗ «MSVC, GCC, Clang» потребует отдельной матрицы; не все цели портируемы из-за `user32`/D3D11 (Skyrim/ImGui-таргеты). Оценка: тесты Core/Markup/Lua можно гонять на Linux, целевой Windows-skyrim-таргет — нет. Вынести в отдельную задачу CI, не блокируя Этап 1.

**Тик для скриптов (`update(dt)`).** Сейчас в `UIContext` нет общего покадрового хука (есть только `setOverlayRender` и `runOnUIThread`, `include/PerfUI/UIContext.h:92-106,140`). Варианты:
1. мод сам вызывает `host->update(dt)` в своём кадре — без изменений в Core;
2. добавить аддитивный `UIContext::addFrameCallback(std::function<void(float)>)` (Lua-агностичный) — PerfUI_Lua ставит его при первом хосте и снимает при последнем. Это позволяет работать разметке+скрипту без кадрового кода мода.

Рекомендация: вариант 2 (публичный API только пополняется, Core остаётся без Lua). Решение зафиксировать в отчёте Этапа 1.

**Риски.**
- *Аллокатор и лимит памяти*: кастомный `lua_Alloc` возвращает `NULL`, ошибка всплывает через `lua_error` внутри `lua_pcall`; проверить, что OOM не превращается в `abort`.
- *Хуки*: счётчик инструкций и таймер ставятся на вызов, сбрасываются после; хуки не должны срабатывать между `pcall`-границей и возвратом.
- *Корутины*: разрешены ТЗ, но `yield` через C-границу ограничен; в v1 не давать скриптам `coroutine.wrap` вокруг C-функций без проверки.
- *Граница исключений*: все C++-колбэки, вызываемые из Lua, обязаны ловить всё и конвертировать в `lua_error`; опционально — SEH-watchdog по образцу оверлеев (`src/core/OverlayManager.cpp`).
- *Поток*: `UIContext::isUIThread()` уже есть (`src/core/UIContext.cpp:557`) — использовать в debug-assert.
- *Хот-релоад*: порядок «снять таймеры → закрыть состояние → загрузить скрипт → пересобрать ручки»; дерево пересоздаётся в `MarkupLoader::poll` — резолвер должен пережить это (ручки через токены).
- *Размер/время сборки*: Lua 5.4 добавит ориентировочно +250–300 КБ к DLL и несколько секунд к полной сборке; базовые числа выше.

---

**Статус:** Этап 0 завершён, код не менялся. Жду подтверждения перед Этапом 1.
