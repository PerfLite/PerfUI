# Слой настроек модов PerfUI (Mod Settings Layer / MCM)

[English](SETTINGS_EN.md) | [Русский](SETTINGS.md)

PerfUI предоставляет модульный, высокопроизводительный слой конфигурации модов для Skyrim SE/AE/VR, служащий современной, быстрой и независимой от Flash/Scaleform альтернативой SkyUI MCM и MCM Helper.

Слой настроек PerfUI объединяет:
1. **Универсальное потокобезопасное хранилище `SettingsStore`** с валидацией, приведением типов и атомарным сохранением в JSON.
2. **Декларативную схему страниц `config.json`** с поддержкой локализации, подстраниц и зависимостей (`visibleIf`, `enabledIf`).
3. **Единое окно настроек `ModSettingsWindow`** с боковой панелью модов, быстрым поиском и PSS-стилизацией.
4. **ABI-стабильный C-API `IPerfUI_v4`** и Lua-интерфейс для сторонних SKSE-плагинов и скриптов.
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

### 1.4 Персистентность и безопасность файлов
* Настройки каждого мода сохраняются в индивидуальный JSON-файл:  
  `Data/SKSE/Plugins/PerfUI/Settings/<modId>.json`.
* **Атомарная запись:** данные сериализуются во временный файл `<modId>.json.tmp.PID_TID`, вызывается `flush`, и затем происходит атомарное переименование в целевой файл через замену (`std::filesystem::rename`). Это полностью исключает повреждение файла при аварийном завершении игры.
* **Отложенная запись (Debounce):** частые изменения слайдеров накапливаются в памяти и сбрасываются на диск через 300–500 мс тишины, не вызывая микрофризов дискового ввода-вывода.
* **Защита от повреждения:** при синтаксической ошибке JSON битый файл автоматически бэкапится в `<modId>.json.bak`, в лог пишется предупреждение, а значения инициализируются значениями по умолчанию.

---

## 2. Декларативная схема страницы (`config.json`)

Моддерам не обязательно писать код на C++ или Lua — достаточно положить файл `config.json` в папку мода `Data/SKSE/Plugins/<ModName>/config.json`. Автозагрузчик PerfUI обнаружит его и смонтирует в общее меню.

### 2.1 Пример `config.json`
```json
{
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

## 5. ABI-стабильный C-API (`IPerfUI_v4`)

Для взаимодействия со сторонними SKSE-плагинами PerfUI предоставляет бинарно-совместимый интерфейс без передачи классов стандартной библиотеки (`std::string`, `std::vector`, `std::function`) через границу DLL.

### 5.1 Инициализация и регистрация из стороннего плагина
```cpp
#include "PerfUI/PerfUIApi.h"

// 1. Декларативная регистрация через JSON
PerfUI::Client::RegisterModSettingsJson(
    "TrueHud",
    "True HUD",
    "Data/SKSE/Plugins/TrueHUD/config.json"
);

// 2. Чтение настроек на лету с минимальным оверхедом
bool isEnabled = PerfUI::Client::GetSettingBool("TrueHud", "enableBars", true);
float scale = PerfUI::Client::GetSettingFloat("TrueHud", "scale", 100.0f);

// 3. Реактивная подписка на изменение слайдера пользователем
uint64_t subId = PerfUI::Client::SubscribeSettingChanged("TrueHud", "scale", [](void* userData, const char* modId, const char* key) {
    float newScale = PerfUI::Client::GetSettingFloat(modId, key, 100.0f);
    ApplyScaleToGameHud(newScale);
}, nullptr);

// 4. Программное открытие меню настроек на нужной вкладке
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
