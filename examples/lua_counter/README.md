# PerfUI Lua Counter Example

Пример интерактивного окна со счётчиком, кнопками, ползунком и периодическим таймером, логика которого полностью написана на Lua без единой строчки C++.

## Состав

- `counter.xml` — декларативная разметка интерфейса с привязкой `<UI script="main.lua">` и событийных обработчиков `onClick`, `onToggle`, `onValueChanged`.
- `main.lua` — безопасный скрипт логики:
  - ручка текстового элемента `ui.find("CounterDisplay")`;
  - изменение текста `display:setText(...)`;
  - уведомления `ui.toast(...)`;
  - периодический таймер автоинкремента `ui.interval(1.0, ...)`;
  - отмена таймера `ui.cancel(autoTimerId)`.

## Запуск из мода

```cpp
#include "PerfUI/PerfUIApi.h"
#include "PerfUI/Markup/MarkupLoader.h"
#include "PerfUI/Lua/ScriptCallbackResolver.h"

// 1. Получаем контекст и создаём хост скрипта
auto* ctx = PerfUI::Client::GetContext();
auto* host = PerfUI::Client::CreateScriptHost("CounterMod");

// 2. Связываем разметку и резолвер
PerfUI::ScriptCallbackResolver resolver(*host);
PerfUI::MarkupLoader loader(*ctx);
loader.setCallbackResolver(&resolver);
loader.enableHotReload(true);

loader.onScriptReload([host](const std::string& scriptRelPath) {
    host->reset();
    host->loadFile("Data/Interface/CounterMod/" + scriptRelPath);
});

// 3. Загружаем разметку и скрипт
loader.loadFile("Data/Interface/CounterMod/counter.xml");
host->loadFile("Data/Interface/CounterMod/main.lua");
```
