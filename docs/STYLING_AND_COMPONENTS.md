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
