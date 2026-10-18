# 🎨 Дизайн-система и компоненты PerfUI (Styling & Component System)

PerfUI предоставляет гибкую, компонентную дизайн-систему, позволяющую авторам модов Skyrim SE/AE и standalone-приложений полностью контролировать визуальный облик своего интерфейса без навязывания жесткого стиля.

---

## 🏛️ Ключевые архитектурные принципы

1. **Независимость стилей и логики:** логика работы компонентов, система компоновки (Flexbox layout) и визуальное оформление полностью разделены.
2. **Любые визуальные стили:** поддержка ванильного Skyrim, тёмного фэнтези (Dark Fantasy), минимализма, киберпанка/футуризма или кастомных тем.
3. **Детерминированный каскад приоритетов (Cascade Order):**
   1. **Local Override** — свойства, заданные прямо на элементе (`element->style().box.normal = ...`, `btn->normalColor(...)`).
   2. **Widget State Style** — стиль конкретного состояния (`Normal`, `Hovered`, `Pressed`, `Focused`, `Disabled`, `Selected`).
   3. **Component Theme Style** — оформление типа компонента из активной темы (`Theme::Current().components.<widget>`).
   4. **Theme Tokens** — глобальные токены палитры, отступов и типографики (`Theme::Current().colors`, `metrics`, `typography`).
4. **Заменяемые темы (Theme Presets):** темы демонстрационные и легко переопределяются или полностью заменяются одной строчкой кода.

---

## 🎨 Темы оформления (Theme Presets)

В `Theme` встроены готовые пресеты, переключаемые вызовом `Theme::ApplyPreset("Имя")` или `Theme::SetCurrent(Theme::...)`:

| Пресет | Палитра | Радиусы | Стиль |
| :--- | :--- | :--- | :--- |
| **Nordic Fantasy** (по умолчанию) | Скандинавское золото (`#D4AF37`), глубокий сланец (`#0D1117`) | `4px / 8px / 12px` | Современный фэнтези-интерфейс |
| **Dark Fantasy** | Обсидиан (`#0A080C`), багровый/кровавый рубин (`#B41E28`) | `3px / 6px / 10px` | Готика, тёмные культы, вампиры |
| **Minimalist** | Нейтральный графит (`#121214`), индиго-акцент (`#5A78DC`) | `2px / 4px / 6px` | Чистый современный плоский дизайн |
| **Vanilla Skyrim** | Пергамент, тёмное железо (`#121110`), имперское золото (`#D8B964`) | `0px / 2px / 4px` | Аутентичный классический стиль Skyrim |
| **Futuristic / SciFi** | Глубокий кибер-чёрный (`#060A12`), неоновый циан (`#00E5FF`) | `0px / 3px / 6px` | Высокотехнологичные и sci-fi моды |

### Пример переключения темы в C++
```cpp
// Применить встроенный пресет
PerfUI::Theme::ApplyPreset("Dark Fantasy");

// Или создать полностью свою тему
PerfUI::Theme customTheme;
customTheme.colors.surfaceElevated = PerfUI::Color(25, 20, 30, 240);
customTheme.colors.borderFocus = PerfUI::Color(255, 120, 0, 255); // Огненный акцент
PerfUI::Theme::SetCurrent(customTheme);
```

---

## 📦 Базовые структуры стилей (`Style.h`)

### `BoxStyle`
Определяет параметры фона, рамки, скругления, прозрачности и тени прямоугольного контейнера:
```cpp
PerfUI::BoxStyle box;
box.bg(PerfUI::Color(30, 35, 45, 240))
   .border(PerfUI::Color(80, 100, 140, 200), 1.5f)
   .radius(8.0f)
   .dropShadow(PerfUI::Color(0, 0, 0, 180), 16.0f, { 0.0f, 4.0f })
   .opac(0.95f);
```

### `TextStyleOverride`
Определяет параметры шрифта и текста с возможностью перекрытия базовых значений:
```cpp
PerfUI::TextStyleOverride text;
text.col(PerfUI::Color::White())
    .size(16.0f)
    .setBold(true)
    .shadow(PerfUI::Color(0, 0, 0, 200), { 1.0f, 1.0f });
```

### `StateStyle<T>`
Содержит настройки для состояний:
- `normal`
- `hover`
- `pressed`
- `focused`
- `disabled`
- `selected`

При вызове `stateStyle.resolve(state, fallback)` действует детерминированный алгоритм отката:
- `Pressed` ➔ проверяет `pressed` ➔ если нет, `hover` ➔ если нет, `normal` ➔ `fallback`.
- `Hovered` ➔ проверяет `hover` ➔ если нет, `normal` ➔ `fallback`.
- `Focused` ➔ проверяет `focused` ➔ если нет, `hover` ➔ если нет, `normal` ➔ `fallback`.
- `Selected` ➔ проверяет `selected` ➔ если нет, `normal` ➔ `fallback`.

---

## 🧱 Новые компоненты

### 1. `Divider`
Горизонтальная или вертикальная разделительная черта.
```cpp
auto* div = panel->add<PerfUI::Divider>(PerfUI::LayoutDirection::Horizontal);
div->thickness(1.5f);
div->color(PerfUI::Color(80, 80, 90, 180));
```

### 2. `Spacer`
Гибкий или фиксированный промежуток в Flexbox-контейнере.
```cpp
// Фиксированный отступ 16px
panel->add<PerfUI::Spacer>(16.0f);

// Или растягивающийся flex-разделитель (прижимает кнопки к краям)
auto* flexSpacer = panel->add<PerfUI::Spacer>();
flexSpacer->flex(1.0f);
```

### 3. `Switch`
Интерактивный тумблер (toggle switch) с плавной физической анимацией ползунка и цвета трека.
```cpp
auto* toggle = panel->add<PerfUI::Switch>(true);
toggle->onToggle([](bool checked) {
    SetSettingEnabled(checked);
});
```

### 4. `RadioButton` и `RadioGroup`
Взаимоисключающий выбор опции из группы.
```cpp
auto* group = panel->add<PerfUI::RadioGroup>();
group->addOption(1, "Низкое качество");
group->addOption(2, "Среднее качество");
group->addOption(3, "Ультра");
group->select(2);

group->onSelectionChanged([](int selectedValue) {
    ApplyQualityPreset(selectedValue);
});
```

### 5. `Label`
Семантический текстовый ярлык с готовыми типографическими стилями:
- `LabelVariant::Heading` (крупный заголовок)
- `LabelVariant::Body` (основной текст)
- `LabelVariant::Caption` (мелкий пояснительный текст)
- `LabelVariant::Accent` (акцентный текст темы)

```cpp
panel->add<PerfUI::Label>("Настройки графики", PerfUI::LabelVariant::Heading);
panel->add<PerfUI::Label>("Изменения вступят в силу немедленно", PerfUI::LabelVariant::Caption);
```

### 6. `Panel::clipChildren`
Поддержка аппаратной обрезки дочерних элементов через стек Scissor Rect:
```cpp
auto* card = panel->add<PerfUI::Panel>();
card->clipChildren(true); // Все выходящие за границы card элементы будут аккуратно обрезаны
```

### 7. `ItemSlot`
Игровой слот инвентаря для предметов и снаряжения.
- Поддержка редкости предмета (`ItemRarity`: `Common`, `Fine`, `Superior`, `Epic`, `Legendary`) с автоматической окраской контуров и фонов.
- Отображение количества предметов в стаке (badge с тенью).
- Индикатор экипированного предмета (ромбовидный маркер `equipped`).
- Обработчики клика, перетаскивания и тултипы.
```cpp
auto* slot = grid->add<PerfUI::ItemSlot>(52.0f);
slot->setItem("textures/items/iron_sword.png", 1, PerfUI::ItemRarity::Superior);
slot->setEquipped(true);
slot->onClick([] {
    EquipOrUseItem();
});
```

### 8. `KeybindSelector`
Интерактивный селектор клавиши для меню настроек модов (MCM):
- Режим прослушивания нажатия (listening state) с пульсирующей подсказкой «Press any key...».
- Отмена при нажатии `Esc` или клике вне элемента.
- Преобразование DirectX / Windows Virtual Key кодов в человекочитаемые названия клавиш (`KeyCodeToString`).
- Callback `onKeyChanged(uint32_t newKey)`.
```cpp
auto* binder = panel->add<PerfUI::KeybindSelector>(VK_F4);
binder->onKeyChanged([](uint32_t key) {
    SaveHotkeyToIni(key);
});
```

### 9. `Accordion` и `AccordionSection`
Складной контейнер с анимированным раскрытием/схлопыванием:
- Плавная интерполяция высоты с отсечением через Scissor rect.
- Индикатор-шеврон (`▼` / `►`).
- Опциональный режим `exclusive(true)` (при открытии секции остальные автоматически сворачиваются).
```cpp
auto* accordion = panel->add<PerfUI::Accordion>();
accordion->exclusive(true);

auto* sec1 = accordion->addSection("Основные настройки", true);
sec1->content()->add<PerfUI::Switch>(true);

auto* sec2 = accordion->addSection("Дополнительно");
sec2->content()->add<PerfUI::Button>("Сброс параметров");
```

### 10. `SearchableList`
Универсальный компонент поиска и фильтрации элементов в реальном времени:
- Встроенный `TextInput` и `ScrollView`.
- Регистронезависимый поиск по подстроке.
- Быстрый выбор элементов с подсветкой и callback `onItemSelected(index, text)`.
```cpp
auto* list = panel->add<PerfUI::SearchableList>();
list->setItems({ "Эбонитовый меч", "Даэдрический топор", "Железный кинжал", "Стеклянный лук" });
list->onItemSelected([](size_t idx, const std::string& item) {
    SelectWeapon(item);
});
```

### 11. `TextArea`
Многострочный текстовый блок с поддержкой прокрутки:
- Автоматическая разбивка строк и скроллинг колесиком мыши.
- Scissor-rect отсечение текста, не влезающего в область видимости.
- Индикатор вертикального скроллбара.
```cpp
auto* textLog = panel->add<PerfUI::TextArea>("Первая строка лога...\nВторая строка...");
textLog->append("\nСобытие выполнено успешно.");
```

### 12. `SegmentedControl`
Горизонтальный сегментированный переключатель (pill selector) с плавно скользящим индикатором выбора:
- Анимированный бегунок активного сегмента (`AnimatedFloat`).
- Разделительные линии между неактивными элементами.
- Поддержка значений и подписей, callback `onSelectionChanged(index, text)`.
```cpp
auto* filter = panel->add<PerfUI::SegmentedControl>();
filter->addSegment("Все", "all")
      .addSegment("Оружие", "wpn")
      .addSegment("Броня", "arm")
      .addSegment("Зелья", "pot");
filter->onSelectionChanged([](size_t index, const std::string& label) {
    FilterInventory(label);
});
```

### 13. `CategoryList`
Список категорий для меню настроек модов (MCM) и навигационных панелей:
- Активный левый индикатор темы оформления (`borderFocus`).
- Поддержка бейджей (например, "NEW", "3") и иконок.
- Callback `onCategoryChanged(id)`.
```cpp
auto* catList = panel->add<PerfUI::CategoryList>();
catList->addCategory("general", "Общие настройки")
       .addCategory("graphics", "Графика", "", "NEW")
       .addCategory("audio", "Звук");
catList->onCategoryChanged([](const std::string& catId) {
    LoadSettingsPage(catId);
});
```

### 14. `QuestTracker`
Игровой компонент отслеживания заданий для HUD и журналов Skyrim:
- Заголовок квеста с аутентичным скайримским ромбовидным маркером.
- Список целей с маркерами прогресса `(current/max)`, отметками выполнения и опциональными метками `(Optional)`.
- Интерактивный клик по целям с `onObjectiveClicked(idx)`.
```cpp
auto* tracker = hud->add<PerfUI::QuestTracker>();
tracker->setQuestTitle("Путь Голоса")
       .setQuestCategory("Основной сюжет")
       .addObjective("Поговорить с Седобородыми")
       .addObjective("Продемонстрировать Безжалостную силу", false, 0, 3)
       .addObjective("Выслушать Арнгейра", false, 0, 0, true);
```

### 15. `Notification`
Всплывающий баннер уведомления с индикатором таймера и кнопкой действия:
- Уровни важности: `NotificationLevel::Info`, `Success`, `Warning`, `Error`.
- Цветовая индикация темы, прогресс-бар оставшегося времени.
- Кнопка закрытия `[X]` и настраиваемая кнопка действия (например, "Открыть меню").
```cpp
auto* notif = panel->add<PerfUI::Notification>();
notif->setTitle("Новый уровень!")
     .setMessage("Достигнут 20 уровень навыка Разрушение.")
     .setLevel(PerfUI::NotificationLevel::Success)
     .setAction("Способности", [] { OpenPerkTree(); })
     .setDuration(5.0f);
```

### 16. `Sidebar`
Сворачиваемая боковая навигационная панель:
- Анимированное плавное переключение между компактным (только иконки) и развернутым режимом.
- Шапка с кнопкой сворачивания (`<<` / `>>`), список навигационных разделов с бейджами.
- Интеграция с темой и каскадом стилей.
```cpp
auto* sidebar = panel->add<PerfUI::Sidebar>();
sidebar->setTitle("Skyrim UI")
       .addItem("dashboard", "Обзор")
       .addItem("inventory", "Инвентарь", "", "12")
       .addItem("spells", "Заклинания");
sidebar->onItemSelected([](const std::string& id) {
    SwitchView(id);
});
```

---

## 🎬 Анимации и Easing (`Animation.h`)

В систему анимаций добавлены:
- **`AnimatedColor`**: плавная интерполяция цветов RGBA с заданной скоростью `setSpeed()` или мгновенным перемещением `snapTo()`.
- **Функции Easing**:
  - `Linear`
  - `EaseInQuad`, `EaseOutQuad`, `EaseInOutQuad`
  - `EaseInCubic`, `EaseOutCubic`, `EaseInOutCubic`
  - `EaseOutElastic` (пружинящий эффект для появления окон и кнопок)

---

## 📝 Разметка в XML (PerfUI Markup)

Все новые компоненты и стили доступны в XML-разметке:
```xml
<Panel name="SettingsDialog" clip-children="true" bg="$surfaceElevated" corner-radius="12" padding="20">
    <Label text="Параметры мода" variant="heading" />
    <Divider thickness="1" color="$borderSubtle" />

    <Switch name="HardcoreMode" checked="true" />
    <Spacer size="12" />

    <RadioGroup name="HudStyle" selected="1">
        <RadioButton value="1" label="Классический Skyrim" />
        <RadioButton value="2" label="Минималистичный HUD" />
        <RadioButton value="3" label="Компактный оверлей" />
    </RadioGroup>
</Panel>
```
