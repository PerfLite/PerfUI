# Слой настроек модов PerfUI (Mod Settings Layer / MCM)

[English](SETTINGS_EN.md) | [Русский](SETTINGS.md)

PerfUI предоставляет модульный, высокопроизводительный слой конфигурации модов для Skyrim SE/AE/VR, служащий современной, быстрой и независимой от Flash/Scaleform альтернативой SkyUI MCM и MCM Helper.

Слой настроек PerfUI объединяет:
1. **Универсальное потокобезопасное хранилище `SettingsStore`** с валидацией, приведением типов, асинхронной записью в фоновом потоке и атомарным сохранением в JSON.
2. **Декларативную схему страниц `perfui.settings.json`** с обязательным маркером `"perfuiSchema": 1`, поддержкой локализации, подстраниц и зависимостей (`visibleIf`, `enabledIf`).
3. **Единое окно настроек `ModSettingsWindow`** с боковой панелью модов, быстрым поиском и PSS-стилизацией.
4. **ABI-стабильный C-API `IPerfUI_v4` и `IPerfUI_v5`** и Lua-интерфейс для сторонних SKSE-плагинов и скриптов.
5. **Систему хоткеев с персистентностью и автоматическим разрешением конфликтов**.

---

## 1. Архитектура хранилища `SettingsStore`

Хранилище `SettingsStore` является потокобезопасным синглтоном, отвечающим за регистрацию определений, проверку диапазонов, кэширование в памяти и запись на диск.

### 1.1 Модель пространства имён
Все настройки изолированы по идентификатору мода `modId`:
```
SettingsStore
 ├── "SkyrimUpscaler"
 │    ├── "enabled"          -> bool
 │    ├── "renderScale"      -> float [50.0 .. 100.0]
 │    └── "hotkey"           -> keycode (uint32)
 └── "TrueDirectionalMovement"
      ├── "targetLockKey"    -> keycode (uint32)
      └── "cameraDistance"   -> float [100.0 .. 600.0]
```

### 1.2 Поддерживаемые типы данных
* `bool` — логические переключатели (`Toggle`).
* `int32_t` — целочисленные параметры.
* `float` — дробные значения и слайдеры (`Slider`, `Stepper`).
* `std::string` — текстовые строки (`Input`, `Menu/Enum`).
* `Color` — RGBA-цвета (упакованный `uint32_t 0xAABBGGRR` или `0xRRGGBBAA`).
* `KeyCode` (`uint32_t`) — коды клавиш мыши/клавиатуры/геймпада.

### 1.3 Валидация и зажим диапазонов (`SettingDef`)
Для каждой настройки регистрируется определение `SettingDef`:
```cpp
SettingDef def;
def.key = "renderScale";
def.type = SettingType::Float;
def.defaultValue = 100.0f;
def.minValue = 50.0f;
def.maxValue = 100.0f;
def.step = 5.0f;
def.formatString = "%.0f%%";
```
При установке значения через `set*` хранилище:
* Приводит типы (например, `int` к `float` или строковый hex-цвет к числу).
* Зажимает числовые значения в интервал `[minValue, maxValue]`.
* Округляет значение согласно заданному шагу `step`.

### 1.4 Персистентность, фоновая запись и безопасность файлов
* Настройки каждого мода сохраняются в индивидуальный JSON-файл:  
  `Data/SKSE/Plugins/PerfUI/Settings/<modId>.json`.
* **Атомарная запись:** данные сериализуются во временный файл `<modId>.json.tmp.PID_TID`, вызывается `flush`, и затем происходит атомарное переименование в целевой файл через замену (`std::filesystem::rename`). Это полностью исключает повреждение файла при аварийном завершении игры.
* **Асинхронная отложенная запись (Debounce & Background Worker):** частые изменения слайдеров накапливаются в памяти. При истечении таймера тишины (400 мс) задача сериализации JSON и дисковой записи отправляется в выделенный рабочий поток `std::jthread`. Повторные сохранения одного и того же мода объединяются (coalescing). Поток рендеринга игры освобождается мгновенно (нагрузка на кадр составляет ~0.012 мс вместо 5–10 мс синхронной дисковой записи).
* **Сброс при закрытии (Flush):** при вызове `flushAll()` (или завершении игры) фоновый поток дожидается завершения всех накопившихся дисковых операций.
* **Защита от повреждения:** при синтаксической ошибке JSON битый файл автоматически бэкапится в `<modId>.json.bak`, в лог пишется предупреждение, а значения инициализируются значениями по умолчанию.

---

## 2. Декларативная схема страницы (`perfui.settings.json`)

Моддерам не обязательно писать код на C++ или Lua — достаточно положить файл `perfui.settings.json` в папку мода `Data/SKSE/Plugins/<ModName>/perfui.settings.json` (или `Data/SKSE/Plugins/perfui.settings.json`). Автозагрузчик PerfUI обнаружит его и смонтирует в общее меню.

> [!IMPORTANT]
> **Обязательный маркер схемы:** Корневой объект JSON **обязан** содержать поле `"perfuiSchema": 1`.  
> Файлы со старыми или сторонними именами (`config.json`), а также JSON без маркера `"perfuiSchema": 1` автозагрузчиком игнорируются, что исключает конфликты с внутренними конфигурациями других плагинов.

### 2.1 Пример `perfui.settings.json`
```json
{
  "perfuiSchema": 1,
  "modName": "MyDynamicHUD",
  "displayName": "$MOD_TITLE",
  "pages": [
    {
      "pageDisplayName": "$PAGE_GENERAL",
      "content": [
        {
          "type": "header",
          "text": "Display Settings"
        },
        {
          "type": "toggle",
          "id": "enableDynamicHud",
          "text": "Enable Status Bars",
          "help": "Shows or hides the status bar overlay.",
          "valueOptions": {
            "default": true
          }
        },
        {
          "type": "slider",
          "id": "hudScale",
          "text": "HUD Scale",
          "help": "Adjust HUD scaling factor.",
          "visibleIf": "enableDynamicHud",
          "valueOptions": {
            "min": 50.0,
            "max": 150.0,
            "step": 5.0,
            "default": 100.0,
            "formatString": "%.0f%%"
          }
        },
        {
          "type": "stepper",
          "id": "tickRate",
          "text": "Tick Rate",
          "visibleIf": "enableDynamicHud",
          "valueOptions": {
            "min": 15.0,
            "max": 60.0,
            "step": 15.0,
            "default": 30.0,
            "formatString": "%.0f Hz"
          }
        },
        {
          "type": "color",
          "id": "accentColor",
          "text": "Theme Accent Color",
          "valueOptions": {
            "default": "0x3399FFCC"
          }
        },
        {
          "type": "menu",
          "id": "anchorPreset",
          "text": "Screen Position",
          "valueOptions": {
            "default": "top_left",
            "options": [
              { "id": "top_left", "text": "Top Left" },
              { "id": "top_right", "text": "Top Right" },
              { "id": "bottom_center", "text": "Bottom Center" }
            ]
          }
        },
        {
          "type": "keymap",
          "id": "toggleKey",
          "text": "Toggle Hotkey",
          "help": "Key to toggle HUD visibility during combat.",
          "valueOptions": {
            "default": 65
          }
        },
        {
          "type": "input",
          "id": "profileName",
          "text": "Active Profile",
          "valueOptions": {
            "default": "Standard"
          }
        }
      ]
    }
  ]
}
```

### 2.2 Поддерживаемые типы элементов контента
| Тип | Описание | Виджет PerfUI |
|---|---|---|
| `header` | Заголовок группы настроек | `Text` с классом `settings-header` |
| `text` | Пояснительный текст | `Text` с классом `settings-help` |
| `empty` | Вертикальный отступ-разделитель | `Panel` (высота 12px) |
| `toggle` | Флаг / чекбокс (bool) | `Checkbox` |
| `slider` | Плавный непрерывный ползунок (float) | `Slider` + метка значения |
| `stepper` | Дискретный шаг переключения | Кнопки [-] и [+] со счетчиком |
| `menu` / `enum` | Выпадающий список выбора из вариантов | `ComboBox` |
| `color` | Выбор цвета RGBA | Превью цвета + hex-редактор |
| `keymap` | Назначение клавиши клавиатуры/мыши | `KeybindButton` с детектором конфликтов |
| `input` | Однострочный ввод текста | `TextInput` |

### 2.3 Условная видимость и доступность
* `visibleIf: "keyName"` — элемент виден только когда булевая настройка `keyName` равна `true`. При префиксе `!` (`visibleIf: "!keyName"`) условие инвертируется.
* `enabledIf: "keyName"` — элемент активен только когда булевая настройка `keyName` истинна; иначе он блокируется (получает класс `settings-disabled`).
* `groupControl` / `groupCondition` — совместимый с MCM Helper синтаксис управления дочерней группой элементов.

### 2.4 Локализация
Тексты с символом `$` в начале (например, `"$MY_MOD_TITLE"`) автоматически переводятся через подсистему `Localization::Get()`. Если ключ не найден в текущем языке, префикс `$` отсекается, и текст выводится как есть.

---

## 3. Классы PSS для стилизации страниц

Все элементы настроек автоматически размечаются CSS-подобными классами для гибкой настройки внешнего вида через `.pss`:

```css
/* Общий контейнер страницы */
.settings-page {
    padding: 16px;
    gap: 8px;
}

/* Строка настройки (содержит метку слева и элемент управления справа) */
.settings-row {
    height: 36px;
    flex-direction: row;
    align-items: center;
    justify-content: space-between;
    padding: 4px 8px;
    border-radius: 4px;
    transition: background-color 0.15s ease;
}

.settings-row:hover {
    background-color: rgba(255, 255, 255, 0.05);
}

/* Заголовок группы настроек */
.settings-header {
    font-size: 16px;
    color: #e5c158;
    margin-top: 12px;
    margin-bottom: 4px;
}

/* Название настройки слева */
.settings-label {
    font-size: 13px;
    color: #dcdcdc;
}

/* Контейнер виджета управления справа */
.settings-control {
    flex-direction: row;
    align-items: center;
    gap: 8px;
}

/* Подсказка / описание настройки */
.settings-help {
    font-size: 11px;
    color: #8a9ba8;
}

/* Подсветка настройки, значение которой отличается от значения по умолчанию */
.settings-row.settings-modified {
    border-left: 2px solid #e5c158;
}

/* Заблокированная настройка (disabled) */
.settings-row.settings-disabled {
    opacity: 0.4;
}

/* Индикатор конфликта назначенной клавиши */
.settings-conflict {
    color: #ff4d4f;
    border: 1px solid #ff4d4f;
}
```

---

## 4. Хоткеи и разрешение конфликтов

### 4.1 Персистентность и приоритет пользователя
Когда мод регистрирует хоткей:
```cpp
context.registerHotkey("MyMod:openMenu", 0x43 /* F9 */, []() { ... });
```
1. Хоткей автоматически регистрируется в `SettingsStore` под пространством имён `MyMod` с дефолтным значением `0x43`.
2. Если пользователь ранее переназначил эту клавишу в меню настроек (например, на `F10`), активируется значение пользователя, а не дефолт автора мода.
3. При изменении клавиши в меню настроек хоткей в реальном времени подменяется в `UIContext` без необходимости перезапуска игры или перезагрузки скриптов.

### 4.2 Обнаружение коллизий
`ModSettingsManager` предоставляет методы для анализа конфликтов клавиш:
```cpp
// Поиск модов, использующих ту же клавишу
auto conflicts = ModSettingsManager::GetSingleton().findKeybindConflicts("MyMod", "openMenu", keyCode);
for (const auto& c : conflicts) {
    std::cout << "Conflict with Mod: " << c.modId << ", Key: " << c.key << std::endl;
}

// Освобождение конфликтующих клавиш (сброс в 0 / Unbound)
ModSettingsManager::GetSingleton().unbindConflictingKeybinds("MyMod", "openMenu", keyCode);
```
Виджет `KeybindButton` подсвечивает коллизию красной меткой и применяет класс `.settings-conflict`.

> [!NOTE]
> В текущей версии поддерживаются одиночные виртуальные коды клавиш (включая Shift, Ctrl, Alt как самостоятельные клавиши). Составные аккорды (например, `Ctrl + Shift + F`) будут добавлены в расширении системы ввода.

---

## 5. ABI-стабильный C-API (`IPerfUI_v4` и `IPerfUI_v5`)

Для взаимодействия со сторонними SKSE-плагинами PerfUI предоставляет строго бинарно-совместимый C-интерфейс без передачи классов стандартной библиотеки (`std::string`, `std::vector`, `std::function`) через границу DLL.

### 5.1 Версии ABI и расширение
* **`IPerfUI_v4`** (376 байт): полный набор функций управления настройками (`RegisterModSettingsJson`, `GetSetting*`, `SetSetting*`, `SubscribeSettingChanged`, `OpenSettings`).
* **`IPerfUI_v5`** (384 байта): обратно совместимое расширение, добавляющее `GetSettingStringCopy` для полностью безопасного копирования строк в буфер вызывающей стороны.
* Размеры и смещения структур зафиксированы проверками `static_assert(sizeof(...) == ...)`.

### 5.2 Безопасность времени жизни строк: `GetSettingString` vs `GetSettingStringCopy`
* **`GetSettingString(modId, key, fallback)`:**  
  Возвращает `const char*`, указывающий на внутренний `thread_local`-буфер. Строка гарантированно валидна в вызывающем потоке до следующего вызова `GetSettingString*`. Идеально для мгновенных локальных проверок внутри кадра (zero-allocation).
* **`GetSettingStringCopy(modId, key, buffer, bufferSize, fallback)`:**  
  Копирует строку в буфер вызывающей стороны с гарантированным нуль-терминатором (возвращает количество записанных байт). Если `buffer == nullptr`, возвращает требуемый размер буфера (включая `\0`). Полностью безопасно для долговременного сохранения и передачи между потоками.
* Клиентская обёртка `PerfUI::Client::GetSettingString` возвращает безопасный `std::string`:
  ```cpp
  std::string myName = PerfUI::Client::GetSettingString("MyMod", "profileName", "Default");
  ```

### 5.3 Жизненный цикл SKSE и готовность API (`IsApiReady`)
* Резолвинг интерфейса PerfUI атомарен и **не кэширует неудачу навсегда**: если мод попытался запросить API до инициализации PerfUI, последующие вызовы успешно получат интерфейс, как только PerfUI будет готов.
* Метод `PerfUI::Client::IsApiReady(minVersion = 4)` позволяет проверить готовность перед вызовом.
* **Рекомендуемый момент регистрации настроек:** в обработчике события `SKSE::MessagingInterface::kDataLoaded` (или `kPostLoad`), когда все плагины гарантированно загружены в адресное пространство процесса.

### 5.4 Потоковая модель подписок и безопасность реентерабельности
* Колбэки `SubscribeSettingChanged` вызываются **синхронно в вызывающем потоке**, выполнившем мутацию (`SetSetting*`).
* **Безопасная реентерабельность:** изнутри колбэка допускается:
  1. Вызывать `SetSetting*` для изменения других ключей (взаимные дедлоки исключены, повторный вызов для того же ключа подавляется).
  2. Отписывать себя через `UnsubscribeSettingChanged(subId)` — итератор не повреждается.
  3. Отписывать другие активные подписки — отписанный колбэк немедленно исключается из очереди текущего оповещения.

### 5.5 Пример использования из стороннего плагина
```cpp
#include "PerfUI/PerfUIApi.h"

// 1. Проверка готовности API
if (!PerfUI::Client::IsApiReady(4)) {
    // PerfUI еще не загружен или не установлен
}

// 2. Декларативная регистрация через perfui.settings.json
PerfUI::Client::RegisterModSettingsJson(
    "TrueHud",
    "True HUD",
    "Data/SKSE/Plugins/TrueHUD/perfui.settings.json"
);

// 3. Чтение настроек на лету
bool isEnabled = PerfUI::Client::GetSettingBool("TrueHud", "enableBars", true);
float scale = PerfUI::Client::GetSettingFloat("TrueHud", "scale", 100.0f);
std::string profile = PerfUI::Client::GetSettingString("TrueHud", "profile", "Default");

// 4. Безопасное чтение в Си-буфер (API v5)
char nameBuf[128];
PerfUI::Client::GetSettingStringCopy("TrueHud", "profile", nameBuf, sizeof(nameBuf), "Default");

// 5. Реактивная подписка на изменение слайдера пользователем
uint64_t subId = PerfUI::Client::SubscribeSettingChanged("TrueHud", "scale", [](void* userData, const char* modId, const char* key) {
    float newScale = PerfUI::Client::GetSettingFloat(modId, key, 100.0f);
    ApplyScaleToGameHud(newScale);
}, nullptr);

// 6. Программное открытие меню настроек на нужной вкладке
PerfUI::Client::OpenSettings("TrueHud");
```

---

## 6. Lua API (`perfui.settings`)

Для модов, использующих скриптовый движок Lua:
```lua
-- Чтение значения с дефолтным фоллбэком
local isHudEnabled = perfui.settings.get("MyMod", "enableDynamicHud", true)
local hudScale = perfui.settings.get("MyMod", "hudScale", 100.0)

-- Подписка на изменение пользователем в меню
perfui.settings.onChanged("MyMod", "hudScale", function(modId, key, newValue)
    print(string.format("Scale adjusted to: %.1f%%", newValue))
end)

-- Программное изменение настройки
perfui.settings.set("MyMod", "hudScale", 120.0)
```

---

## 7. Сравнение с SkyUI MCM и MCM Helper

| Характеристика | SkyUI MCM | MCM Helper | PerfUI Settings Layer |
|---|---|---|---|
| **Среда выполнения** | Flash / Scaleform 42 | C++ + Scaleform UI | Native C++20 + DirectX 11 |
| **Скриптовый оверхед** | Тяжелые Papyrus Quest-скрипты | C++ бэкенд, но меню во Flash | Нулевой (0 Papyrus-скриптов) |
| **Время открытия меню** | 1.5 – 5.0 секунд (лаги загрузки) | ~1 секунда | **< 16 миллисекунд (мгновенно)** |
| **FPS при открытом меню** | 20 – 40 FPS (ограничение ActionScript) | 30 – 50 FPS | **60 – 144+ FPS (родная кадровая частота)** |
| **Стилизация** | Невозможна (зашито в SWF) | Невозможна (зашито в SWF) | **Полная через PSS (CSS-подобные стили)** |
| **Обнаружение конфликтов** | Нет | Ограниченное | Встроенное с опцией «Освободить» |
| **Формат схемы** | Код на Papyrus (`OnConfigInit`) | `config.json` | Декларативный JSON / C++ API / Lua |
| **Зависимости** | SkyUI.esp, SKSE | SkyUI.esp, MCM Helper.dll | Только PerfUI.dll |

---

## 8. Парсер SimpleJson и гарантии безопасности

Для разбора манифестов `perfui.settings.json` и сохранения значений в PerfUI встроен легковесный заголовочный парсер `src/settings/SimpleJson.h`, обладающий следующими гарантиями:
* **Защита от переполнения стека:** глубина рекурсии ограничена 64 уровнями вложенности. При превышении возвращается ошибка парсинга вместо аварийного завершения.
* **Декодирование UTF-16 surrogate pairs:** последовательности `\uD800..\uDBFF` + `\uDC00..\uDFFF` корректно преобразуются в 4-байтные последовательности UTF-8 (смайлики, редкие символы). Одиночные или битые суррогаты отклоняются.
* **Строгая валидация чисел (RFC 8259):** запрещены ведущие нули (`012`), неэкранированные `NaN` и `Infinity`, валидируются экспоненты (`1e+5`).
* **Пропуск UTF-8 BOM:** файлы с сигнатурой `\xEF\xBB\xBF` считываются прозрачно.
* **Разрешение дубликатов ключей:** стратегия *«last key wins»* (последний ключ в объекте перезаписывает предыдущие).
* **Точная диагностика ошибок:** все сообщения об ошибках содержат номер строки, номер колонки и точное байтовое смещение.
* **Устойчивость к некорректным данным:** подтверждена стресс-тестом на 2,000 случайных и мутированных входов (битые escapes, незакрытые скобки/кавычки, гигантские числа, null-байты).
