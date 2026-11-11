# Полный технический аудит декларативного слоя PerfUI (XML + Lua)

> **Дата аудита:** 11 ноября 2026 г.  
> **Версия кодовой базы:** PerfUI v0.1.0 (Обновлено после реализации PSS Этапов 0–2)  
> **Целевой файл:** `docs/xml-audit.md`  
> **Статус:** Этапы 0–2 PSS полностью реализованы. Детали см. в [`docs/pss.md`](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/docs/pss.md) и отчёте [`docs/pss-report.md`](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/docs/pss-report.md).

> [!NOTE]
> Замечания аудита о жестко зашитых состояниях кнопок, отсутствии селекторов, каскада, псевдоклассов и переменных успешно устранены подсистемой PSS (PerfUI Style Sheets) и каскадным разрешением стилей.

---

## 1. Что умеет XML сейчас

Текущий декларативный слой PerfUI построен как **дерево вызовов C++-фабрик виджетов**. XML-разбор преобразует иерархию узлов в дерево объектов `UIElement` через `WidgetRegistry`. Это не браузерный движок и не CSS-процессор: стили не каскадируются, селекторы отсутствуют, а атрибуты тегов напрямую вызывают методы мутации свойств создаваемых C++ объектов.

### 1.1. Используемые библиотеки и версии
- **XML-парсер:** `pugixml` **v1.16** (`PUGIXML_VERSION 1160` в [third_party/pugixml/pugixml.hpp:15](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/third_party/pugixml/pugixml.hpp#L15)). Используется DOM-парсер `load_buffer` с расчетом смещений для номеров строк (`node.offset_debug()`).
- **Скриптовый движок:** **Lua 5.4.7** (`LUA_VERSION_RELEASE "7"`, `LUA_VERSION_NUM 504` в [third_party/lua/lua.h:19-24](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/third_party/lua/lua.h#L19-L24)). Используется чистый C API Lua (`lua_*`, `luaL_*`); sol2, LuaBridge или другие сторонние биндинги не применяются.

---

### 1.2. Корневые теги
1. `<UI script="path/to/script.lua">` ([src/markup/MarkupLoader.cpp:461-473](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L461-L473))  
   Корневой узел декларативного интерфейса. Содержит атрибут `script`, связывающий разметку с файлом логики на Lua. Сам узел `<UI>` не создает отдельного визуального элемента; он возвращает первый созданный дочерний элемент как корень дерева.
2. `<Theme name="...">` ([src/core/Theme.cpp:21](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/core/Theme.cpp#L21), [src/markup/MarkupLoader.cpp:435-445](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L435-L445))  
   Декларативное описание темы оформления: цветовая палитра (`<Colors>`), метрики отступов и скруглений (`<Metrics>`), типографика (`<Typography>`).

---

### 1.3. Полный реестр поддерживаемых тегов и виджетов (41 тег)

В реестре виджетов `WidgetRegistry` ([src/markup/WidgetRegistry.cpp](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp)) зарегистрировано **39 виджетов** (плюс 2 корневых тега `<UI>` и `<Theme>`):

| № | Тег в XML | C++ Класс | Строка реестра | Назначение виджета |
|---|---|---|---|---|
| 1 | `<Panel>` | `Panel` | [WidgetRegistry.cpp:76](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L76) | Базовый прямоугольный контейнер, фон, границы, скругления, 9-slice, blur, clip |
| 2 | `<Text>` | `Text` | [WidgetRegistry.cpp:122](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L122) | Текстовый блок (шрифт, размер, цвет, перенос строк, binding) |
| 3 | `<Button>` | `Button` | [WidgetRegistry.cpp:153](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L153) | Кнопка с текстовой меткой и обработчиком `onClick` |
| 4 | `<ScrollView>` | `ScrollView` | [WidgetRegistry.cpp:182](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L182) | Контейнер с вертикальной прокруткой и скроллбаром |
| 5 | `<Checkbox>` | `Checkbox` | [WidgetRegistry.cpp:206](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L206) | Флажок переключателя с текстовой меткой (`onToggle`, `onChange`) |
| 6 | `<ProgressBar>` | `ProgressBar` | [WidgetRegistry.cpp:242](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L242) | Индикатор прогресса (value, min, max, цвета трека/заливки) |
| 7 | `<Slider>` | `Slider` | [WidgetRegistry.cpp:285](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L285) | Ползунок числового значения с шагом (`onValueChanged`) |
| 8 | `<Image>` | `Image` | [WidgetRegistry.cpp:322](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L322) | Растровое изображение по TextureId, с UV-координатами и tint |
| 9 | `<SvgImage>` | `SvgImage` | [WidgetRegistry.cpp:361](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L361) | Векторная SVG-графика из файла (`src`, `tint`) |
| 10 | `<Svg>` | `SvgImage` | [WidgetRegistry.cpp:361](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L361) | Алиас для `<SvgImage>` |
| 11 | `<TabBar>` | `TabBar` | [WidgetRegistry.cpp:405](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L405) | Панель горизонтальных вкладок (`tabs="A,B,C"`, `selectedTab`) |
| 12 | `<ComboBox>` | `ComboBox` | [WidgetRegistry.cpp:435](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L435) | Выпадающий список вариантов (`options="1,2,3"`) |
| 13 | `<TextInput>` | `TextInput` | [WidgetRegistry.cpp:468](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L468) | Однострочное поле ввода текста (`placeholder`, `onSubmit`) |
| 14 | `<MapCanvas>` | `MapCanvas` | [WidgetRegistry.cpp:494](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L494) | Специализированный SkyUI-совместимый холст карты мира |
| 15 | `<Divider>` | `Divider` | [WidgetRegistry.cpp:522](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L522) | Линия-разделитель (`orientation="horizontal|vertical"`) |
| 16 | `<Spacer>` | `Spacer` | [WidgetRegistry.cpp:556](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L556) | Промежуточный гибкий/фиксированный отступ |
| 17 | `<Switch>` | `Switch` | [WidgetRegistry.cpp:577](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L577) | Переключатель-тумблер (toggle switch) |
| 18 | `<RadioButton>` | `RadioButton` | [WidgetRegistry.cpp:595](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L595) | Радиокнопка с меткой и значением |
| 19 | `<RadioGroup>` | `RadioGroup` | [WidgetRegistry.cpp:620](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L620) | Группирующий контейнер для взаимоисключающих `RadioButton` |
| 20 | `<Label>` | `Label` | [WidgetRegistry.cpp:638](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L638) | Семантический алиас для текстовой подписи к контролам |
| 21 | `<ItemSlot>` | `ItemSlot` | [WidgetRegistry.cpp:674](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L674) | Игровой слот предмета инвентаря (иконка, счетчик, рамка редкости) |
| 22 | `<KeybindSelector>` | `KeybindSelector` | [WidgetRegistry.cpp:704](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L704) | Контрол назначения клавиши клавиатуры или геймпада |
| 23 | `<Accordion>` | `Accordion` | [WidgetRegistry.cpp:725](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L725) | Аккордеон со сворачиваемыми секциями |
| 24 | `<AccordionSection>` | `AccordionSection` | [WidgetRegistry.cpp:744](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L744) | Отдельная сворачиваемая секция внутри аккордеона |
| 25 | `<SearchableList>` | `SearchableList` | [WidgetRegistry.cpp:765](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L765) | Список элементов с встроенной строкой поиска и фильтрации |
| 26 | `<TextArea>` | `TextArea` | [WidgetRegistry.cpp:783](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L783) | Многострочный блок отображения или редактирования текста |
| 27 | `<SegmentedControl>` | `SegmentedControl` | [WidgetRegistry.cpp:811](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L811) | Сегментированный переключатель режимов |
| 28 | `<CategoryList>` | `CategoryList` | [WidgetRegistry.cpp:830](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L830) | Вертикальный список категорий (для интерфейсов инвентаря/магии) |
| 29 | `<QuestTracker>` | `QuestTracker` | [WidgetRegistry.cpp:850](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L850) | Виджет отслеживания целей активных квестов на HUD |
| 30 | `<Notification>` | `Notification` | [WidgetRegistry.cpp:870](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L870) | Всплывающая плашка игрового оповещения (toast banner) |
| 31 | `<Sidebar>` | `Sidebar` | [WidgetRegistry.cpp:901](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L901) | Боковая выдвижная или фиксированная панель |
| 32 | `<WorldAnchor>` | `WorldAnchor` | [WidgetRegistry.cpp:927](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L927) | 3D->2D маркер привязки к мировым координатам/актерам |
| 33 | `<SettingsView>` | `SettingsView` | [WidgetRegistry.cpp:1018](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1018) | Корневой экран настроек модификаций (MCM стиль) |
| 34 | `<SettingsSection>` | `SettingsSection` | [WidgetRegistry.cpp:1040](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1040) | Группа настроек внутри `SettingsView` |
| 35 | `<SettingToggle>` | `SettingToggle` | [WidgetRegistry.cpp:1088](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1088) | Элемент настройки: булев флажок |
| 36 | `<Toggle>` | `SettingToggle` | [WidgetRegistry.cpp:1089](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1089) | Алиас для `<SettingToggle>` |
| 37 | `<SettingSlider>` | `SettingSlider` | [WidgetRegistry.cpp:1092](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1092) | Элемент настройки: числовой ползунок |
| 38 | `<SettingChoice>` | `SettingChoice` | [WidgetRegistry.cpp:1124](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1124) | Элемент настройки: выбор из списка |
| 39 | `<SettingTextInput>` | `SettingTextInput` | [WidgetRegistry.cpp:1163](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1163) | Элемент настройки: текстовая строка |
| 40 | `<SettingButton>` | `SettingButton` | [WidgetRegistry.cpp:1192](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1192) | Элемент настройки: кнопка действия |
| 41 | `<SettingKeybind>` | `SettingKeybind` | [WidgetRegistry.cpp:1218](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L1218) | Элемент настройки: переназначение клавиши |

---

### 1.4. Общие атрибуты (поддерживаются на ВСЕХ тегах)
Обрабатываются в `applyCommonLayoutAttribute` ([src/markup/MarkupLayoutParser.cpp:269-413](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L269-L413)):

| Атрибут | Тип значения | Дефолт | Описание и ссылка на код |
|---|---|---|---|
| `name` | `string` | `""` | Имя элемента в иерархии для поиска через `findByName` и `ui.find` ([L273](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L273)) |
| `position` | `string` | `"relative"` | `"absolute"` переводит элемент в абсолютный режим; `"relative"` или `"static"` — flex ([L278](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L278)) |
| `left`, `right`, `top`, `bottom` | `float` / токен | `0.0f` | Смещения для абсолютного позиционирования ([L284-287](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L284-L287)) |
| `center-x` | `bool` | `true` | Центрирование абсолютного элемента по горизонтали ([L288](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L288)) |
| `direction`, `flex-direction` | `string` | `"column"` | `"row"`, `"horizontal"` или `"column"`, `"vertical"` ([L289](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L289)) |
| `align`, `align-items` | `string` | `"stretch"` | `"start"`, `"center"`, `"end"`, `"stretch"` ([L295](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L295)) |
| `justify`, `justify-content` | `string` | `"start"` | `"start"`, `"center"`, `"end"`, `"space-between"`, `"space-around"` ([L303](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L303)) |
| `width`, `height` | `DimensionConstraint` | `Auto` | Число (`px`), `"auto"`, `"flex"`, `"flex:N"`, `"N%"` ([L312-319](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L312-L319)) |
| `flex` | `float` | `1.0f` | Flex-коэффициент расширения по главной оси ([L320](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L320)) |
| `padding` | `float` (1/2/4 зн.) | `0` | Внутренние отступы: `all`, `v h`, `t r b l` ([L324](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L324)) |
| `margin` | `float` (1/2/4 зн.) | `0` | Внешние отступы: `all`, `v h`, `t r b l` ([L328](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L328)) |
| `margin-top`, `-bottom`, `-left`, `-right`| `float` | `0` | Раздельные внешние отступы ([L332-355](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L332-L355)) |
| `gap` | `float` / токен | `0` | Промежуток между дочерними элементами во flex ([L356](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L356)) |
| `min-width`, `max-width`, `min-height`, `max-height` | `float` | `0` / inf | Ограничители размера ([L360-375](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L360-L375)) |
| `visible` | `bool` | `true` | Видимость узла (`"true"`, `"false"`, `"1"`, `"0"`, `"yes"`, `"no"`) ([L376](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L376)) |
| `enabled` | `bool` | `true` | Доступность для инпута ([L380](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L380)) |
| `focusable` | `bool` | `false` | Участие в навигации фокуса геймпада/клавиатуры ([L384](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L384)) |
| `tooltip` | `string` | `""` | Текст всплывающей подсказки с локализацией ([L388](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L388)) |
| `class` | `string` | `""` | Список классов через пробел, сохраняемый в `m_classes` ([L392](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L392)) |
| `blend-mode`, `blendMode` | `string` | `"normal"` | Режим наложения: `"additive"`, `"multiply"`, `"screen"`, `"normal"` ([L401](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L401)) |
| `bind` | `string` | `""` | Ключ привязки данных (обрабатывается только для `<Text>`) ([L408](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L408)) |

> [!WARNING]
> Атрибут `opacity` **ОТСУТСТВУЕТ** в `isCommonLayoutAttribute` и `applyCommonLayoutAttribute`! При попытке указать `<Panel opacity="0.5">` парсер выдаст предупреждение о неизвестном атрибуте, и прозрачность **не применится**. В ядре метод `UIElement::setOpacity` есть, но в XML-слой он не проброшен.

---

### 1.5. Специфичные атрибуты ключевых виджетов

- **`<Panel>`** ([WidgetRegistry.cpp:83-113](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L83-L113), [MarkupLoader.cpp:230-258](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L230-L258)):
  - `bg`, `background`, `bg-color`: `Color` (цвет фона)
  - `border`, `border-color`: `Color` (цвет рамки)
  - `border-width`: `float` (толщина рамки)
  - `radius`, `corner-radius`: `float` (радиус скругления углов)
  - `shadow`: `bool` (дефолтная тень)
  - `liquid-glass`, `liquid_glass`: `bool` (шейдер размытого жидкого стекла)
  - `backdrop-blur`, `blur`, `glass`: `"liquid"` | `bool` | `float` (радиус размытия фона)
  - `clip-children`, `clipChildren`: `bool` (обрезка дочерних элементов scissor-прямоугольником)
  - `bg-image`: `path` (текстура фона, относительный путь разрешается от XML-файла или `Data/`)
  - `bg-uv`: `float float float float` (UV координаты текстуры: `u0 v0 u1 v1`)
  - `bg-slice`: `int int int int` (9-slice нарезка: `left top right bottom` в пикселях текстуры)
  - `bg-corner`: `int int int int` (размеры угловых зон на экране при 9-slice)
- **`<Text>`** ([WidgetRegistry.cpp:128-146](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L128-L146), [MarkupLoader.cpp:217-228](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L217-L228)):
  - `text`: `string` (текст с поддержкой токенов локализации `$KEY`)
  - `font`: `string` (гарнитура/семейство шрифта)
  - `font-size`, `fontSize`: `float` / токен темы (`$fontSmall`, `$fontBody`, `$fontTitle`)
  - `color`: `Color`
  - `bold`, `italic`: `bool`
  - `wrap`: `float` (ширина переноса строки; `0` — без переноса)
  - `bind`: `string` (ключ привязки для вызовов `loader.setBinding(key, val)`)
- **`<Button>`** ([WidgetRegistry.cpp:159-175](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L159-L175), [MarkupLoader.cpp:278-296](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L278-L296)):
  - `text`, `label`: `string` (надпись на кнопке)
  - `radius`: `float`
  - `onClick`: `string` (имя C++ колбэка или глобальной Lua-функции)
- **`<ScrollView>`** ([WidgetRegistry.cpp:188-199](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L188-L199)):
  - `scroll`, `scrollSpeed`, `scroll-speed`: `float` (скорость прокрутки)
  - `showScrollbar`, `show-scrollbar`: `bool`
  - `offset`: `float` (начальное смещение скролла)
- **`<Checkbox>`** ([WidgetRegistry.cpp:212-235](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L212-L235), [MarkupLoader.cpp:298-315](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L298-L315)):
  - `label`, `text`: `string`
  - `checked`: `bool`
  - `boxSize`, `box-size`: `float`
  - `checkColor`, `labelColor`: `Color`
  - `onToggle`, `onChange`: `string` (колбэк, передающий `1.0` или `0.0`)
- **`<Slider>`** ([WidgetRegistry.cpp:291-315](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L291-L315), [MarkupLoader.cpp:317-334](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L317-L334)):
  - `value`: `float`, `min`: `float` (дефолт 0), `max`: `float` (дефолт 100), `step`: `float` (дефолт 1)
  - `label`: `string`
  - `fillColor`, `trackColor`: `Color`
  - `onValueChanged`, `onChange`: `string` (колбэк с аргументом `double val`)
- **`<ProgressBar>`** ([WidgetRegistry.cpp:248-278](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L248-L278)):
  - `value`, `progress`: `float` (0.0 .. 1.0)
  - `min`, `max`: `float`
  - `showLabel`: `bool`, `text`: `string`
  - `fillColor`, `trackColor`, `borderColor`, `borderWidth`, `radius`: стили бара
- **`<Image>`** ([WidgetRegistry.cpp:328-354](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L328-L354)):
  - `texture`: `uint32_t` (числовой идентификатор текстуры `TextureId`)
  - `tint`: `Color` (по умолчанию белый)
  - `u0`, `v0`, `u1`, `v1`: `float` (UV-координаты среза)
  - `flip-y`: `bool`
- **`<SvgImage>` / `<Svg>`** ([WidgetRegistry.cpp:367-398](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L367-L398), [MarkupLoader.cpp:260-275](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L260-L275)):
  - `src`: `path` (путь к `.svg` файлу)
  - `tint`: `Color`
- **`<TabBar>`** ([WidgetRegistry.cpp:411-428](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L411-L428), [MarkupLoader.cpp:354-372](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L354-L372)):
  - `tabs`: список через запятую (например, `tabs="Характеристики, Навыки, Магия"`)
  - `selectedTab`: `size_t` (индекс активной вкладки)
  - `onTabChanged`, `onChange`: `string` (колбэк с индексом вкладки)
- **`<ComboBox>`** ([WidgetRegistry.cpp:441-461](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L441-L461), [MarkupLoader.cpp:336-353](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L336-L353)):
  - `options`: список через запятую (например, `options="Низкое, Среднее, Ультра"`)
  - `selectedIndex`: `size_t`
  - `onSelectionChanged`, `onChange`: `string` (колбэк с индексом)
- **`<TextInput>`** ([WidgetRegistry.cpp:474-487](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L474-L487), [MarkupLoader.cpp:374-410](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L374-L410)):
  - `placeholder`: `string`
  - `text`: `string`
  - `fontSize`: `float`
  - `onTextChanged`, `onChange`: `string` (колбэк с текущей строкой)
  - `onSubmit`, `onEnter`: `string` (колбэк при нажатии Enter со строкой)

---

### 1.6. Единицы измерения

Парсер размеров `parseDimension` ([src/markup/MarkupLayoutParser.cpp:179-197](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L179-L197)):
- **`px` (пиксели):** **ЕСТЬ**. Базовое число без суффикса или с вызовом `parseNumberOrToken` (например `width="300"`).
- **`%` (проценты):** **ЧАСТИЧНО**. Поддерживается **только** для `width` и `height` (например `width="100%"` транслируется в `DimensionConstraint::Percent(100.0f)`). Для `padding`, `margin`, `gap`, `font-size`, `left`, `top` проценты **НЕ поддерживаются** и будут прочитаны как `0.0`.
- **`auto`:** **ЧАСТИЧНО**. Поддерживается только для `width`/`height` (`DimensionConstraint::Auto()`).
- **`flex`, `flex:N`:** **ЧАСТИЧНО**. Поддерживается для `width`/`height` (`DimensionConstraint::Flex(weight)`).
- **`em`, `rem`, `vw`, `vh`:** **НЕТ**. В кодовой базе нет парсера типографических или viewport-единиц.
- **Отрицательные значения:** `std::strtof` технически распарсит `-10.0f`, но в макете они не имеют смысла и приводят к некорректным bounding boxes, за исключением абсолютных координат (`left="-50"`).

---

### 1.7. Как задаются цвета

Функция `parseColorOrToken` ([src/markup/MarkupLayoutParser.cpp:75-170](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L75-L170)) поддерживает следующие форматы:
1. **Hex:**
   - `#RGB` (например `#F00` -> `Color(255, 0, 0, 255)`)
   - `#RGBA` (например `#F008` -> `Color(255, 0, 0, 136)`)
   - `#RRGGBB` (например `#FF2040`)
   - `#RRGGBBAA` (например `#FF2040CC`)
2. **CSS функциональный синтаксис:**
   - `rgb(255, 128, 0)`
   - `rgba(255, 128, 0, 0.5)` (альфа в диапазоне 0.0..1.0 масштабируется в 0..255; поддерживаются разделители запятая, пробел, слэш)
3. **Именованные цвета:**
   - `white`, `black`, `transparent`, `gold` (NordicGold `#CC9933`)
4. **Токены темы (`$token`):**
   - `$surfaceBase`, `$surfaceElevated`, `$surfaceActive`
   - `$borderSubtle`, `$borderStrong`, `$borderFocus`
   - `$accent`, `$nordicGold`
   - `$textPrimary`, `$textSecondary`, `$textAccent`, `$textSuccess`, `$textDisabled`, `$textMuted`
   - `$focusGlow`, `$selectionFill`, `$white`, `$black`, `$transparent`

> [!IMPORTANT]
> Токены темы резолвятся **однократно** в момент парсинга строки XML. Это не CSS-переменные: при смене темы на лету через `Theme::Apply()` значения в уже созданных элементах **не обновятся**.

---

### 1.8. Шаблоны, компоненты, циклы и вложенность
- **Вложенность:** Произвольная глубина. Функция `buildElement` рекурсивно обходит всех детей `pugi::xml_node::children()` ([src/markup/MarkupLoader.cpp:490-493](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L490-L493)).
- **Повторяющиеся элементы, циклы, шаблоны:** **НЕТ**. В разметке нет тегов `<template>`, `<for>`, `<each>`, `<slot>`.
- **Импорт / Include:** **НЕТ**. Нельзя сделать `<include src="components/button.xml"/>`.
- **Условный рендеринг:** **НЕТ**. Нет тегов `<if>`, `<when>`, `<switch>`.

---

## 2. Стили

Сводный статус стилизации в текущей архитектуре:

| Возможность | Статус | Код в проекте | Пояснение и границы работы |
|---|---|---|---|
| Атрибут `class` | **Частично** | [MarkupLayoutParser.cpp:392-399](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L392-L399) | Парсится и пишется в `element->addClass(c)`, но селекторы и правила отсутствуют |
| Атрибут `id` | **Нет** | [MarkupLayoutParser.cpp:234](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L234) | Атрибута `id` нет. Используется атрибут `name` для поиска в C++ и Lua |
| Отдельный файл стилей | **Нет** | — | Нет файлов `.css`/`.pss`. Стили задаются только inline-атрибутами в XML |
| Селекторы (тег, класс, id, `>`, `:hover`) | **Нет** | — | Механизм выбора элементов и связывания стилей полностью отсутствует |
| Каскад и специфичность | **Нет** | — | Правил нет. Свойства применяются напрямую в C++ сеттеры в порядке разбора |
| Наследование свойств (`color`, `font`) | **Нет** | [UIElement.cpp:187-193](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/core/UIElement.cpp#L187-L193) | Наследования нет. Единственное исключение: `effectiveOpacity()` перемножает альфу по цепочке родителей |
| CSS-переменные (`var(--x)`) | **Нет** | [MarkupLayoutParser.cpp:39-55](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L39-L55) | Есть токены темы (`$spacingMD`, `$accent`), но они разово вычисляются при парсинге |
| Состояния (`:hover`, `:active`, `:disabled`) | **Частично** | [Style.h:100-150](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/Style.h#L100-L150) | `StateStyle<BoxStyle>` есть в C++, но `MarkupLoader` **не заполняет** `style()`, из XML задать нельзя |
| Слой применения стилей | **Inline** | [WidgetRegistry.cpp:84-112](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L84-L112) | Напрямую в поля C++ виджета (`panel->backgroundColor(...)`). Без слоя PSS/CSS |
| `@media`-запросы | **Нет** | — | Адаптивности под разрешения/aspect ratio из разметки нет |

### Подробный разбор состояния стилей

1. **`class` и `id`:**
   В XML разрешен атрибут `class="btn btn-primary"`. Парсер разбивает строку по пробелам и вызывает `element->addClass(c)` ([src/markup/MarkupLayoutParser.cpp:392-400](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L392-L400)). Классы хранятся в векторе `std::vector<std::string> m_classes` ([include/PerfUI/UIElement.h:252](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/UIElement.h#L252)). Однако в текущей версии с этим вектором ничего не происходит: нет движка стилей, который бы искал правила для этих классов. Атрибут `id` вовсе не распознается — вместо него используется `name="my_button"`.

2. **Где хранятся состояния и как переключаются:**
   В ядре C++ описан гибкий шаблон `StateStyle<T>` ([include/PerfUI/Style.h:100-150](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/Style.h#L100-L150)) с полями `normal`, `hover`, `pressed`, `focused`, `disabled`, `selected`. Структура `WidgetStyle` объединяет `StateStyle<BoxStyle>` и `StateStyle<TextStyleOverride>`.
   Однако **парсинг разметки вообще не трогает `element->style()`**! Все стили виджетов в XML являются хардкодными полями экземпляра (`Panel::m_bgColor`, `Text::m_color`). Виджеты вроде `Button` внутри метода `render()` сами рассчитывают цвет под курсором на базе темы, минуя XML:
   ```cpp
   // Пример из Button.cpp: расчет состояния зашит в C++ коде
   Color bg = isHovered() ? theme.colors.surfaceActive : theme.colors.surfaceElevated;
   ```
   Из XML невозможно написать `<Button hover-bg="#FF0000">` или `:hover { bg: #FF0000; }`.

3. **Dirty-инвалидация стилей:**
   Флаг `m_layoutDirty` существует в `UIElement` и помечается при смене размеров или вызове `markLayoutDirty()` ([include/PerfUI/UIElement.h:249](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/UIElement.h#L249)). Однако флага **StyleDirty** не существует: визуальные свойства не кэшируются в виде вычисленного объекта (ComputedStyle), а считываются напрямую из полей виджета каждый кадр в цикле рендеринга.

---

## 3. Анимации из разметки

### 3.1. Возможности анимаций из XML
- **`transition`:** **НЕТ**. В XML нельзя написать `transition="opacity 0.2s ease-out"`.
- **`@keyframes` / `<animation>`:** **НЕТ**. Декларативного описания ключевых кадров нет.
- **Связь XML с `ElementAnimator`, `ElementSpring`, `Timeline`:** **ОТСУТСТВУЕТ**. Все мощные анимационные возможности PerfUI доступны **исключительно из C++ кода**:
  ```cpp
  // Доступно ТОЛЬКО из C++:
  button->animate()
      .scale(1.05f, 0.15f, Easing::EaseOutQuad)
      .alpha(1.0f, 0.2f);
  
  panel->spring()
      .snappy()
      .position({100.0f, 200.0f});
  ```
- **Трансформы из разметки:**
  - `transform` (`offset`, `scale`, `rotation`): **НЕТ**. В XML нет атрибутов для задания визуального смещения или масштаба без участия layout.
  - `opacity`: **НЕТ в XML** (только в C++ и Lua).
  - `blend-mode`: **ДА**. Доступен атрибут `blend-mode="additive|multiply|screen|normal"` ([src/markup/MarkupLayoutParser.cpp:401](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L401)).

### 3.2. Что анимируемо в ядре C++, а что нет

В ядре PerfUI анимации разделены на 3 системы ([include/PerfUI/Animation.h](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/Animation.h)):
1. **`ElementAnimator`** ([include/PerfUI/Animation.h:449-560](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/Animation.h#L449-L560)):
   - **Анимируемо:** `position` (Point), `moveBy` (Point), `scale` (Point / float), `size` (Dimensions), `alpha` / `renderOpacity` (float), `color` (Color), `ProgressBar.progress`.
   - Поддерживает 30+ кривых `Easing` (Quad, Cubic, Sine, Back, Bounce, Elastic, Expo, Quart, Circ, CubicBezier).
   - Поддерживает `AnimPositionMode::RenderOffset` (анимация через визуальный сдвиг без повторного прогона flex-лейаута!) и `AnimPositionMode::BoundsLayout`.
2. **`ElementSpring`** ([include/PerfUI/Animation.h:563-635](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/Animation.h#L563-L635)):
   - Физические пружины с инерцией (stiffness, damping, mass).
   - **Анимируемо:** `position`, `size`, `scale`, `alpha`, `color`.
3. **`Timeline`** ([include/PerfUI/Animation.h:641-740](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/Animation.h#L641-L740)):
   - Секвенсор параллельных и последовательных треков, stagger-анимации групп списков.

**Что вообще НЕ анимируемо в PerfUI (даже в C++):**
- **`rotation` (вращение):** В структуре `VisualTransformData` ([src/core/UIElement.cpp:11-17](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/core/UIElement.cpp#L11-L17)) есть `offset`, `scale`, `pivot`, но **нет угла поворота**. Бэкенд ImGui не поддерживает произвольное вращение геометрии элементов без кастомных матриц трансформации вершин.
- **`padding`, `margin`, `gap`:** Нет треков интерполяции; изменение этих параметров вызывает полный пересчет Flexbox.
- **`border-radius`, `border-width`, `shadow`:** Не поддерживаются интерполятором `ElementAnimator`.

### 3.3. Доступность: `reducedMotion` и `timeScale`
- В `AnimationScheduler` ([include/PerfUI/AnimationScheduler.h:20,26](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/AnimationScheduler.h#L20-L26)) реализованы методы `setTimeScale(float)` и `setReducedMotion(bool)`.
- В `UIElement::update()` ([src/core/UIElement.cpp:341-356](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/core/UIElement.cpp#L341-L356)) логика подключена намертво:
  - Если включен `isReducedMotion()`, любые анимации `ElementAnimator` и `ElementSpring` мгновенно снапятся в финальную позицию (`finish()`), устраняя параллакс и укачивание.
  - Дельта времени масштабируется: `effectiveDt *= scheduler.timeScale()`.
- **Проблема:** ни `reducedMotion`, ни `timeScale` пока не выставлены в Lua API и не могут контролироваться из разметки.

---

## 4. События и Lua

### 4.1. Привязка событий из XML

В PerfUI события из разметки привязываются через интерфейс `ICallbackResolver` ([include/PerfUI/Markup/ICallbackResolver.h](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/include/PerfUI/Markup/ICallbackResolver.h)). Реализация `ScriptCallbackResolver` ([modules/lua/src/ScriptCallbackResolver.cpp](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/modules/lua/src/ScriptCallbackResolver.cpp)) ищет глобальную функцию с указанным именем в состоянии Lua.

**Полный список поддерживаемых событий в разметке (строго 6 типов):**

| Событие в XML | Поддерживаемые виджеты | Вызов в Lua | Файл и строка |
|---|---|---|---|
| `onClick` | `<Button>` | `fn()` (без аргументов) | [MarkupLoader.cpp:278-296](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L278-L296) |
| `onToggle`, `onChange` | `<Checkbox>` | `fn(1.0)` / `fn(0.0)` | [MarkupLoader.cpp:298-315](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L298-L315) |
| `onValueChanged`, `onChange` | `<Slider>` | `fn(double value)` | [MarkupLoader.cpp:317-334](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L317-L334) |
| `onSelectionChanged`, `onChange` | `<ComboBox>` | `fn(double index)` | [MarkupLoader.cpp:336-353](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L336-L353) |
| `onTabChanged`, `onChange` | `<TabBar>` | `fn(double tabIndex)` | [MarkupLoader.cpp:354-372](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L354-L372) |
| `onTextChanged`, `onChange` | `<TextInput>` | `fn(string text)` | [MarkupLoader.cpp:374-391](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L374-L391) |
| `onSubmit`, `onEnter` | `<TextInput>` | `fn(string text)` | [MarkupLoader.cpp:392-410](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L392-L410) |

> [!CAUTION]
> События `hover` (`onMouseEnter`/`Leave`), `focus` (`onFocus`), `drag`, `scroll`, `keyDown`, `keyUp`, `gamepadButton` в разметке **НЕ ПОДДЕРЖИВАЮТСЯ**. Они существуют только как виртуальные C++ методы `UIElement`.

---

### 4.2. Поверхность экспорта в Lua (Lua API)

Экспорт реализован в [modules/lua/src/ScriptHost.cpp:860-1154](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/modules/lua/src/ScriptHost.cpp#L860-L1154).

#### 1. Глобальная таблица `ui`
- `ui.find(name: string) -> LuaElementHandle | nil` — рекурсивный поиск элемента по имени `name`.
- `ui.toast(message: string, [type: string])` / `ui.toast(title, message, type)` — всплывающее уведомление (`"info"`, `"success"`, `"warning"`, `"error"`).
- `ui.timer(delaySeconds: number, callback: function) -> timerId: integer` — одноразовый таймер.
- `ui.interval(intervalSeconds: number, callback: function) -> timerId: integer` — периодический интервал.
- `ui.cancel(timerId: integer) -> boolean` — отмена таймера.
- `ui.translate(key: string, [fallback: string]) -> string` (алиас `ui.tr(...)`) — локализация.
- `ui.setLanguage(lang: string)` / `ui.getLanguage() -> string`.
- `ui.loadDictionary(lang: string, path: string) -> boolean`.
- `ui.theme.color(token: string) -> table | nil` — возвращает `{ r, g, b, a, hex }` для токена темы.

#### 2. Методы объекта `LuaElementHandle`
Каждый найденный элемент оборачивается в userdata с метатаблицей `PerfUI.ElementHandle`:
- **Идентификация и состояние:**
  - `handle:id() -> integer`
  - `handle:name() -> string`
  - `handle:isValid() -> boolean` (безопасная проверка: жив ли C++ объект)
  - `handle:isVisible() -> boolean`, `handle:setVisible(visible: boolean)`
  - `handle:isEnabled() -> boolean`, `handle:setEnabled(enabled: boolean)`
  - `handle:isFocused() -> boolean`
- **Визуальные параметры и размеры:**
  - `handle:opacity() -> number`, `handle:setOpacity(opacity: number)`
  - `handle:setColor(r, g, b, [a])` или `handle:setColor("#RRGGBBAA")` (алиас `setTint`)
  - `handle:position() -> x: number, y: number`, `handle:setPosition(x: number, y: number)`
  - `handle:setWidth(width: number)`, `handle:setHeight(height: number)`
  - `handle:setSize(width: number, height: number)`
  - `handle:setFontSize(size: number)`
- **Значения виджетов:**
  - `handle:text() -> string`, `handle:setText(text: string)` (для Text, Button, Checkbox, ProgressBar, TextInput, Slider)
  - `handle:value() -> number`, `handle:setValue(val: number)` (для Slider, ProgressBar, Checkbox)
  - `handle:isChecked() -> boolean`, `handle:setChecked(chk: boolean)` (для Checkbox)
  - `handle:selectedIndex() -> integer`, `handle:selectIndex(idx: integer)` (для ComboBox, TabBar)
- **Изображения и маркеры:**
  - `handle:setTexture(textureId: integer)`
  - `handle:setUV(u0, v0, u1, v1)`
  - `handle:setFlipY(flip: boolean)`
  - `handle:targetId()`, `handle:setTargetId(id)` (для WorldAnchor)
  - `handle:setScreenOffset(x, y)`, `handle:isClamped()`, `handle:clampedAngle()`, `handle:currentScale()`

---

### 4.3. Может ли Lua менять разметку на лету?
**НЕТ.**
- Lua **не может** создавать элементы (`ui.createElement` отсутствует).
- Lua **не может** добавлять/удалять узлы DOM (`element:addChild`, `element:remove` отсутствуют).
- Lua **не может** добавлять/снимать классы (`addClass`, `removeClass` не экспортированы).
- Lua **не может** подписываться на произвольные события через код (`element:on("click", fn)` отсутствует).
- **Вывод:** Дерево разметки фиксируется один раз при загрузке XML. Lua может только мутировать скалярные свойства существующих элементов по их именам.

---

### 4.4. Data Binding
- **Статус:** **Односторонний, крайне ограниченный**.
- Поддерживается **только** для тега `<Text bind="property_name">` ([src/markup/MarkupLoader.cpp:218-228, 413](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L218-L228)).
- Обновление происходит строго из C++: `loader.setBinding("player_hp", "100/100")`.
- Для всех остальных виджетов и атрибутов связывания данных нет. Реактивного стейта (State -> UI) нет.

---

### 4.5. Навигация фокусом и геймпадом
- В разметке задается только булев флаг: `focusable="true|false"`.
- Порядок обхода фокуса (`Tab` или D-Pad геймпада) определяется **строго порядком обхода дерева элементов в глубину** ([src/core/UIContext.cpp:420-530](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/core/UIContext.cpp#L420-L530)).
- В XML **нет**:
  - `tabindex` или `focus-index`.
  - Явных соседей перехода: `focus-up="btn1"`, `focus-down="btn2"`.
  - Фокусных групп или ловушек фокуса (focus trap для модальных окон).

---

### 4.6. Диагностика ошибок
- **XML:** Номер строки вычисляется через `calculateLineNumber(xml, offset)` ([src/markup/MarkupLoader.cpp:36-42](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L36-L42)). Ошибки синтаксиса и неизвестные атрибуты записываются в `LoadResult.errors` и `LoadResult.warnings` с указанием `[Line N]`. При Hot Reload ошибки выводятся прямо в игру через всплывающий `showToast` красного цвета ([MarkupLoader.cpp:531](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L531)).
- **Lua:** Защищенный вызов `lua_pcall` с дебаг-хуком `LimitHook` ([modules/lua/src/ScriptHost.cpp:1168-1185](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/modules/lua/src/ScriptHost.cpp#L1168-L1185)). Превышение лимита инструкций (`maxInstructionsPerCall`) или лимита времени кадра (`maxMillisecondsPerCall`) прерывает скрипт ошибкой. Если скрипт падает несколько раз подряд (`maxConsecutiveErrors`), он деактивируется для защиты игры от зависаний.

---

### 4.7. Безопасность и изоляция (Песочница Lua)
Реализована строгая изоляция ([modules/lua/src/ScriptHost.cpp:296-310](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/modules/lua/src/ScriptHost.cpp#L296-L310)):
- Занулены опасные библиотеки и функции: `io`, `os`, `package`, `debug`, `dofile`, `loadfile`, `load`.
- Функция `require` переписана: разрешает загружать модули только из безопасного каталога `scriptRoot`, запрещает символы `..`, ведущие и замыкающие точки (`isSafeModuleName`).
- Память ограничена: кастомный аллокатор `LuaAllocator` пресекает выделение свыше `limits.maxMemoryBytes`.
- Защита от крашей (use-after-free): `WeakElementRef` использует атомарный `m_aliveToken` (`std::shared_ptr<bool>`). Если C++ элемент был удален, обращение к нему из старого хэндла Lua безопасно пишет предупреждение в лог и не приводит к падению игры.

---

## 5. Hot Reload

### 5.1. Как устроен механизм Hot Reload
Реализован в `MarkupLoader::poll()` ([src/markup/MarkupLoader.cpp:501-611](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L501-L611)):
1. Отслеживание выполняется методом опроса (`poll`): раз в заданный интервал (`m_pollInterval`) проверяется метка времени изменения файла `std::filesystem::last_write_time(m_sourceFilePath)`.
2. Системные файловые вотчеры (`ReadDirectoryChangesW`) не используются: опрос происходит только тогда, когда игровой цикл вызывает `loader.poll()`.

### 5.2. Что перезагружается, что сохраняется, а что теряется

```
┌─────────────────────────────────────────────────────────────┐
│                 MarkupLoader::poll()                        │
└──────────────────────────────┬──────────────────────────────┘
                               ▼
 1. Сбор состояния: collectScrollOffsets(), collectActiveTabs(), focusedName
 2. Полный парсинг нового XML через pugixml (при ошибке -> Toast и аборт)
 3. Вызов хука beforeReload(m_root)
 4. УДАЛЕНИЕ СТАРОГО ДЕРЕВА: parent->removeChild(m_root)
 5. СОЗДАНИЕ НОВОГО ДЕРЕВА: parent->addChild(newRoot)
 6. Вызов onScriptReload(scriptAttr)
 7. Восстановление состояния по имени (name):
    ├── ScrollView offset     ──► Восстановлено (по имени)
    ├── TabBar selectedTab    ──► Восстановлено (по имени)
    └── Focused element       ──► Восстановлен (по имени)
 8. Вызов хука afterReload(m_root)
```

- **СОХРАНЯЕТСЯ:**
  - Положение скролла у всех `ScrollView`, имеющих атрибут `name`.
  - Выбранная вкладка у всех `TabBar`, имеющих атрибут `name`.
  - Фокус ввода на элементе, если элемент с таким же `name` существует в новом дереве и имеет `focusable="true"`.
  - Зарегистрированные C++ привязки данных (`setBinding`).
- **ТЕРЯЕТСЯ:**
  - Текст в полях ввода `TextInput`.
  - Состояние флажков `Checkbox` (сбрасываются в значение из XML).
  - Положение ползунков `Slider` (сбрасываются в XML дефолт).
  - Выбор в выпадающих списках `ComboBox`.
  - Все запущенные C++ анимации (`ElementAnimator`) и физические пружины (`ElementSpring`).
  - Все старые `LuaElementHandle`: ссылки инвалидируются (`handle:isValid()` становится `false`). Если Lua-скрипт сохранил хэндл в глобальную переменную, последующие вызовы не будут иметь эффекта.
  - Любые элементы, добавленные динамически из C++ кода.

### 5.3. Кэш текстур и шрифтов при reload
- Текстуры загружаются через `UIContext::loadTexture(path)` ([MarkupLoader.cpp:238](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L238)). Кэш текстур живет в `UIContext` и **НЕ сбрасывается**. При повторном открытии файла текстура мгновенно берется из видеопамяти по `TextureId`.
- Шрифты и атласы ImGui также сохраняются без перекомпиляции.

---

## 6. Рендер, доступный разметке

### 6.1. Сравнение графических возможностей

| Графическая возможность | В кодовой базе C++ | Доступно напрямую из XML | Примечание |
|---|---|---|---|
| Сплошная заливка цветом | Да | **Да** (`bg="#HEX"`) | [WidgetRegistry.cpp:85](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L85) |
| Скругления углов (border-radius) | Да | **Да** (`radius="8"`) | [WidgetRegistry.cpp:90](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L90) |
| Рамка (border-width, color) | Да | **Да** (`border="#...", border-width="1"`) | [WidgetRegistry.cpp:88](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L88) |
| 9-slice спрайты (рамки, свитки) | Да | **Да** (`bg-image`, `bg-slice`, `bg-corner`) | [MarkupLoader.cpp:245](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLoader.cpp#L245) |
| Размытие фона (Backdrop blur) | Да | **Да** (`backdrop-blur="1.5"`) | [WidgetRegistry.cpp:99](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L99) |
| Эффект жидкого стекла (Liquid glass) | Да | **Да** (`liquid-glass="true"`) | [WidgetRegistry.cpp:94](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L94) |
| Режимы наложения (Blend modes) | Да | **Да** (`blend-mode="additive"`) | [MarkupLayoutParser.cpp:401](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/MarkupLayoutParser.cpp#L401) |
| Scissor-обрезка (clip-children) | Да | **Да** (`clip-children="true"`) | [WidgetRegistry.cpp:110](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L110) |
| Векторный SVG | Да | **Да** (`<SvgImage src="..." tint="..."/>`) | [WidgetRegistry.cpp:361](file:///c:/Dev/C++/Skyrim%20mods/PerfUI/src/markup/WidgetRegistry.cpp#L361) |
| Линейные и радиальные градиенты | Да (`drawGradientRect`) | **НЕТ** | Нет синтаксиса в XML |
| Геометрические примитивы (линии, круги) | Да (`drawLine`, `drawCircle`) | **НЕТ** | Только через специфичные виджеты (`<Divider>`) |
| Тонкая настройка тени (blur, spread, offset, color) | Да (`ShadowStyle`) | **НЕТ** (только булев `shadow="true"`) | В XML зашит пресет тени |
| Раздельные скругления углов | Да (`DrawList::PathCorner`) | **НЕТ** | Только одно общее число `radius` |
| Прозрачность элемента (`opacity`) | Да (`UIElement::setOpacity`) | **НЕТ** | В XML атрибут `opacity` не читается |
| Визуальные оффсет и скейл (transforms) | Да (`VisualTransformData`) | **НЕТ** | В XML нет `transform`, `offset`, `scale` |

### 6.2. Произвольный Canvas в XML
- **Статус:** **ОТСУТСТВУЕТ**.
- Тег `<MapCanvas>` является специализированной картой мира SkyUI, а не холстом рисования.
- Элемента `<Canvas>` с колбэком `onDraw(painter)` в разметке нет. Скрипт на Lua не может рисовать процедурные линии, полигоны, графики или эффекты в окне без создания отдельного C++ виджета.

### 6.3. Что из C++ можно быстро выставить в разметку
Без изменения архитектуры ядра (буквально правкой 20 строк в `MarkupLayoutParser.cpp` и `MarkupLoader.cpp`) в разметку можно выставить:
1. `opacity="0.0 - 1.0"` — сразу пробросить в `element->setOpacity(...)`.
2. `offset="x y"` и `scale="sx sy"` — сразу выставить в `VisualTransformData` через `element->visualOffset(...)` и `element->visualScale(...)`.
3. Расширенный `shadow="color blur x y"` — распарсить 4 значения в `panel->dropShadow(...)`.
4. `corner-radius="tl tr br bl"` — независимые радиусы углов.

---

## 7. Таблица разрыва между текущим состоянием и подмножеством CSS

Оценка трудоемкости:
- **S (Small):** 1-2 дня. Локальные правки парсера или вызов существующих C++ методов.
- **M (Medium):** 3-5 дней. Добавление новой подсистемы (парсер, хранилище правил, инвалидация).
- **L (Large):** 1-2 недели. Комплексное изменение архитектуры ядра, многопроходный рендер или глубокий рефакторинг.

| № | Возможность | Статус | Что нужно сделать | Сложность | Что разблокирует |
|---|---|---|---|---|---|
| 1 | `class` и `id` в разметке | **Частично** | Добавить парсинг `id="..."` в `applyCommonLayoutAttribute` (сейчас только `name`). Подключить `m_classes` и `m_id` к реестру стилей. | **S** | Точечную адресацию элементов из CSS-правил |
| 2 | Отдельный файл стилей и его парсер | **Нет** | Написать лексер/парсер формата `.pss` / `.css` (токены, блоки правил `{ ... }`, свойства). Связать с `MarkupLoader`. | **M** | Разделение структуры (XML) и внешнего вида (CSS). Чистый XML без мусора атрибутов |
| 3 | Селекторы (`tag`, `.class`, `#id`, потомок, `>`) | **Нет** | Реализовать структуру `Selector` с операторами сопоставления с `UIElement` (проверка имени тега, классов, родителя). | **M** | Переиспользование стилей для типовых кнопок, списков, заголовков |
| 4 | Псевдоклассы (`:hover`, `:focus`, `:active`, `:disabled`) | **Нет** | Связать события мыши/фокуса в `UIElement` с флагами псевдоклассов и триггером пересчета стилей. | **M** | Интерактивные состояния кнопок и карточек без хардкода в C++ коде виджетов |
| 5 | Каскад и специфичность | **Нет** | Реализовать алгоритм специфичности CSS `(inline, id, class, tag)` и сортировку правил перед применением. | **M** | Корректное переопределение стилей (тема -> класс -> оверрайд инлайном) |
| 6 | Наследование свойств | **Нет** | Реализовать флаг `Inherited` для `color`, `font-family`, `font-size` при обходе дерева стилей сверху вниз. | **M** | Установка шрифта/цвета на весь экран одним правилом на корневом узле |
| 7 | CSS-переменные и темы | **Частично** | Перевести токены из однократного резолва в рантайм-хранилище переменных с динамическим обновлением при смене темы. | **M** | Мгновенное переключение тем оформления на лету без перезагрузки разметки |
| 8 | Единицы измерения (`rem`, `vw/vh`, `%`) | **Частично** | Добавить вычисление `rem` от базового шрифта темы и `vw/vh` от `UIContext::viewport()`. Поддержать `%` для margin/padding. | **M** | Адаптивную верстку под разные разрешения мониторов (1080p, 1440p, 4K, Ultrawide) |
| 9 | `transition` в разметке/стилях | **Нет** | При смене значения свойства (например, `bg` на `:hover`) проверять правило transition и запускать `ElementAnimator`. | **M** | Плавные анимации наведения, смены вкладок, нажатий прямо из декларативного слоя |
| 10 | `@keyframes` + `animation` | **Нет** | Парсер блоков `@keyframes` и связывание их с `Timeline` / `AnimationScheduler`. | **L** | Сложные зацикленные эффекты (пульсация, свечение, мерцание рун, вращение спиннеров) |
| 11 | `transform` (translate/scale/rotate) | **Частично** | Добавить `rotate` в `VisualTransformData`, парсить синтаксис `transform: translate(x,y) scale(s) rotate(deg)`. | **M** | Анимацию покачивания и масштабирования карточек без затрат на Flex-лейаут |
| 12 | `opacity`, `mix-blend-mode` | **Частично** | Добавить парсинг `opacity` в XML/CSS (сейчас только blend-mode). | **S** | Полноценный контроль полупрозрачности панелей и слоев |
| 13 | `background-image` (cover/contain), градиенты | **Частично** | Поддержать `background-size: cover/contain` и `linear-gradient(...)` в `BoxStyle` и `ImGuiRenderBackend`. | **M** | Красивые фоновые подложки, градиентные полосы здоровья/магии |
| 14 | `border-radius`, `box-shadow`, `outline` | **Частично** | Поддержать раздельные радиусы углов и полный синтаксис `box-shadow: x y blur spread color`. | **S** | Современный дизайн карточек, неоновое свечение, акцентные рамки фокуса |
| 15 | Hot reload стилей | **Нет** | Отдельный вотчер для `.css` файлов: обновляет стили на существующем DOM без пересоздания элементов! | **S** | Мгновенную итерацию дизайна в игре без потери состояния открытых окон |
| 16 | События разметки к Lua | **Частично** | Добавить поддержку `onHover`, `onFocus`, `onKeyDown`, `onGamepad` в XML и трансляцию в Lua. | **M** | Полноценный скриптинг интерактивных мини-игр и меню |
| 17 | Диагностика ошибок (линтер) | **Частично** | Расширить парсер валидацией типов свойств CSS с выводом точных сообщений об ошибках в Toast. | **S** | Быструю отладку верстки мододелами |
| 18 | `@media`-аналог (aspect / width) | **Нет** | Вычисление условий `@media (min-width: ...)` и `@media (aspect-ratio: ...)` относительно экрана игры. | **M** | Отдельную верстку для экранов 16:9, 16:10, 21:9 и 32:9 |

---

## 8. Рекомендация по архитектуре CSS-слоя

### 8.1. Где должен жить парсер и хранилище правил
1. **Новый модуль:** Создать изолированную подсистему `PerfUI_Style` в каталогах:
   - `include/PerfUI/Style/` (`StyleSheet.h`, `StyleRule.h`, `StyleSelector.h`, `ComputedStyle.h`)
   - `src/style/` (`StyleParser.cpp`, `StyleCascade.cpp`, `StyleMatcher.cpp`)
2. **Таблица правил (`StyleSheet`):**
   Плоский упорядоченный список правил:
   ```cpp
   struct StyleRule {
       Selector selector;          // .skill-node:hover, #perk-tree > Panel
       StyleDeclaration properties; // map<PropertyId, StyleValue>
       uint32_t specificity;        // (A, B, C): ID, Class/Pseudo, Tag
       uint32_t sourceOrder;        // Порядок появления в файле
   };
   ```
3. **Моменты применения каскада:**
   - **При загрузке/построении DOM:** Один проход сверху вниз после создания элемента.
   - **При смене класса или состояния (`:hover`, `:focus`, `:active`):** Инкрементально только для затронутого элемента и его потомков (если есть зависимые правила).
   - **При Hot Reload стилей:** Пересчет стилей на **живом дереве элементов** без пересоздания DOM!

---

### 8.2. Как каскад соединяется с `UIElement::style()`

Чтобы не ломать прямой код на C++, вводится **слоистый приоритет (Layered Style Resolution)**:

$$\text{Final Style} = \text{Default Theme} \longrightarrow \text{CSS Cascade (Tag, Class, ID)} \longrightarrow \text{Inline XML Attrs} \longrightarrow \text{C++ Direct Overrides}$$

```
                ┌──────────────────────────────┐
                │        Default Theme         │ (Базовые цвета темы)
                └──────────────┬───────────────┘
                               ▼
                ┌──────────────────────────────┐
                │   PSS / CSS Cascade Rules    │ (Селекторы, классы, псевдоклассы)
                └──────────────┬───────────────┘
                               ▼
                ┌──────────────────────────────┐
                │   Inline XML Attributes      │ (bg="#...", radius="4" в теге)
                └──────────────┬───────────────┘
                               ▼
                ┌──────────────────────────────┐
                │   C++ Direct Overrides       │ (Ручные вызовы btn->style().box...)
                └──────────────────────────────┘
```

Каждое поле в `ComputedStyle` хранит бит происхождения (`SourceOrigin`). Значение, установленное из C++ вручную, помечается флагом `ManualOverride` и не перезаписывается каскадом CSS.

---

### 8.3. Инвалидация и dirty-флаги
В `UIElement` добавляется битовая маска состояния:
```cpp
enum class DirtyFlags : uint16_t {
    Clean        = 0,
    Layout       = 1 << 0, // Изменились width, height, padding, gap
    Style        = 1 << 1, // Сменился класс, псевдокласс hover/active
    RenderVisual = 1 << 2, // Сменился цвет, прозрачность, blend-mode
    SubtreeStyle = 1 << 3  // Требуется проверить наследуемые свойства детей
};
```
- Смена псевдокласса (`m_hovered = true`) выставляет флаг `DirtyFlags::Style`.
- Если изменились только визуальные свойства (`background-color`, `opacity`), выставляется `RenderVisual`. **Flexbox-лейаут не дергается!**
- Если изменились `padding`, `margin`, `width`, дополнительно выставляется `Layout`.

---

### 8.4. Как избежать роста стоимости кадра (Zero-Alloc Hot Path)
- **Иммутабельный `ComputedStyle`:**
  Вычисленный стиль элемента кэшируется в `std::shared_ptr<const ComputedStyle>` внутри `UIElement`.
- **Проверка в цикле кадров:**
  В кадре рендеринга `render()` проверяется условие:
  ```cpp
  if (m_dirtyFlags == DirtyFlags::Clean) {
      backend.drawBox(m_bounds, *m_computedStyle); // 0 пересчетов, чтение из кэша
      return;
  }
  ```
  Если состояние элемента не изменилось, сопоставление селекторов и каскад в кадре занимают **0 микросекунд**.

---

### 8.5. Минимальный жизнеспособный набор (MVP: 7 свойств)
Для полноценной реализации дерева навыков (Skyrim Skill Tree) и игровых меню достаточно поддержать 7 ключевых свойств:
1. `background: <color> | <token>` (+ градиент).
2. `color: <color> | <token>` (цвет текста).
3. `border: <width> <color>`, `border-radius: <radius>` (рамки узлов перков).
4. `opacity: <float>` (затемнение заблокированных навыков).
5. `transform: scale(<f>) translate(<x>, <y>)` (эффект зума при наведении без сдвига сетки).
6. `box-shadow: <color> <blur> <offset>` (магическое свечение активного перка).
7. `transition: <prop> <time> <easing>` (плавный переход состояний hover/active).

---

### 8.6. Что НЕЛЬЗЯ сделать без изменений ядра C++
1. **Вращение (`transform: rotate`):** Требует добавления угла поворота в `VisualTransformData` и матрицы трансформации в `ImGuiRenderBackend` (вершинный шейдер или поворот quad вершин).
2. **Псевдоклассы мыши на любых элементах:** Сейчас только `Button` и некоторые контролы отслеживают hover. Базовый `UIElement` должен уметь перехватывать hit-test и менять `m_hovered` для любого `Panel`.
3. **Наследование свойств:** Требуется добавить в `UIElement` знание о наследуемом контексте стиля родителя.

---

### 8.7. План поэтапной реализации

```
┌────────────────────────────────────────────────────────────────────────┐
│ ЭТАП 1: Парсер стилей и классы (S-M, 4 дня)                            │
│ Цель: вынести цвета и рамки из XML в отдельный файл styles.pss.        │
│ Приемка: тест сопоставления правил вида .btn { bg: #FFF; } к тегам.    │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│ ЭТАП 2: Псевдоклассы и реактивность (M, 4 дня)                         │
│ Цель: поддержка :hover, :active, :focus, :disabled.                    │
│ Приемка: кнопка меняет цвет при наведении без C++ кода в Button.cpp.   │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│ ЭТАП 3: Transition на базе ElementAnimator (M, 3 дня)                  │
│ Цель: анимация смены свойств при hover/active.                         │
│ Приемка: плавное затухание/масштабирование кнопки по правилу CSS.      │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│ ЭТАП 4: Hot Reload стилей без перезагрузки DOM (S-M, 2 дня)            │
│ Цель: правка .pss файла мгновенно обновляет вид игры.                  │
│ Приемка: при сохранении CSS не сбрасывается скролл, инпуты и анимации. │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 9. Итог и вердикт

### 9.1. Насколько далеко мы от подмножества HTML/CSS?

> [!IMPORTANT]
> **Оценка готовности: ~25%.**
> Текущий XML-слой PerfUI — это **прямой сериализатор создания C++ виджетов**, а не движок стилей. В нем уже есть надежный парсер XML (`pugixml`), регистрация тегов, базовая flexbox-раскладка и песочница Lua. Однако в нем **полностью отсутствует механизм каскада, селекторов и правил стилей**: нет связи между классами и свойствами, стили задаются только жестко зашитыми inline-атрибутами тегов, а псевдосостояния (`hover`/`active`) захардкожены внутри C++ методов `render()`. Чтобы слой стал настоящим декларативным подмножеством CSS, требуется построить поверх существующего ядра архитектуру PSS (PerfUI Style Sheets).

---

### 9.2. Три самых больших блокера

1. **Отсутствие таблицы правил стилей и сопоставления селекторов:**
   Класс `class="..."` парсится в никуда. Нет концепции файла стилей, селекторов (`.class`, `:hover`) и каскада. Изменить внешний вид элементов без переписывания каждого тега XML невозможно.
2. **Отсутствие реактивности псевдосостояний из разметки:**
   Поведения `:hover`, `:active`, `:focus` зашиты в C++ логику конкретных классов (`Button.cpp`, `Checkbox.cpp`). Обычная панель `<Panel>` не может изменить цвет при наведении курсора средствами разметки.
3. **Деструктивный Hot Reload:**
   При любом изменении файла разметки текущее дерево элементов уничтожается целиком. Сбрасываются состояния полей ввода, прерываются анимации, инвалидируются ссылки в Lua. Дизайнер не может настраивать внешний вид сложного открытого экрана без постоянного сброса его контекста.

---

### 9.3. Что можно сделать за один короткий этап (2-3 дня) с максимальной пользой

Вывести в разметку недостающие свойства, которые **уже реализованы в C++ ядре**, но забыты в парсере:
1. Добавить атрибут `opacity="0.0 - 1.0"` в `applyCommonLayoutAttribute` (в C++ метод `setOpacity` уже есть).
2. Добавить атрибуты `transform-offset="x y"` и `transform-scale="sx sy"` (в C++ `VisualTransformData` уже есть).
3. Добавить атрибут `id="..."` как алиас к `name="..."`.
4. Добавить префиксные атрибуты состояний в `WidgetRegistry` для базовых виджетов: `hover-bg="..."`, `hover-border="..."`, `pressed-bg="..."`.

Это позволит сразу создавать интерактивные кнопки, всплывающие карточки и красивые полупрозрачные меню прямо в XML без написания единой строчки C++ кода.

---

### 9.4. Что делать первым делом и почему

**Первым делом необходимо реализовать Этап 1 и 2 из раздела 8: минимальный модуль PSS с поддержкой внешнего файла стилей, классов (`.class`) и псевдокласса `:hover`.**

**Почему:**
Сейчас любая стилизация интерфейса в проекте ведет к раздуванию XML миллионами повторяющихся атрибутов `bg="..." border="..." radius="..." font-size="..."`, а для любого изменения цвета наведения приходится идти в C++ код `Button.cpp`. Введение легкого PSS-слоя мгновенно сократит размер XML в 3-4 раза, позволит переиспользовать дизайн между модами и заложит правильный фундамент для системы плавных анимаций (`transition`) без необходимости переписывать ядро рендеринга.
