# PerfUI Style Sheets (PSS) Specification & Modder Guide

[English](PSS_EN.md) | [Русский](pss.md)

PerfUI Style Sheets (`.pss`) — это легковесный декларативный движок стилей, реализующий подмножество CSS специально для высокопроизводительных игровых интерфейсов Skyrim SE/AE/VR.

PSS полностью отделяет оформление и визуальные эффекты от структуры разметки (`.xml`) и логики поведения (`.lua` / C++), исключая необходимость встраивания тяжелых webview-движков.

---

## 1. Подключение стилей

### Из XML разметки
Стили подключаются через атрибут `styles` в корневом элементе `<UI>` или тег `<Styles>`:

```xml
<UI styles="theme.pss custom.pss" script="menu.lua">
    <!-- Или явный импорт внутри документа -->
    <Styles src="extra.pss" />

    <Panel class="card">
        <Text class="title" text="Заголовок" />
    </Panel>
</UI>
```

Файлы стилей применяются по порядку следования. При равной специфичности селекторов свойства из более позднего файла переопределяют более ранние.

### Из Lua API
```lua
-- Динамическая перезагрузка всех подключенных файлов стилей без пересоздания дерева виджетов
ui.reloadStyles()

-- Чтение и установка CSS-переменных в рантайме
ui.setVar("--accent", "#00d2ff")
local accent = ui.getVar("--accent")

-- Управление классами элементов
local card = ui.find("MainCard")
if card then
    card:addClass("highlight")
    card:removeClass("collapsed")
    card:toggleClass("active")
    if card:hasClass("highlight") then
        print("Card is highlighted")
    end
end
```

### Из C++ API
```cpp
// Перезагрузка стилей
context.reloadStyles();

// Установка CSS переменных
context.setCssVariable("--accent", "#ff5500");

// Управление классами
element->addClass("active");
element->removeClass("disabled");
element->toggleClass("expanded");
```

---

## 2. Селекторы и специфичность

PSS поддерживает следующие типы селекторов:

| Селектор | Пример | Описание |
| :--- | :--- | :--- |
| `*` | `* { opacity: 1; }` | Универсальный селектор |
| Тег виджета | `Button { height: 32; }` | По типу виджета (регистронезависимо) |
| Класс | `.card { radius: 8; }` | По классу элемента |
| ID | `#submitBtn { width: 200; }` | По идентификатору элемента (`id` / `name`) |
| Составной | `Panel.card.big` | Элемент типа `Panel`, содержащий оба класса |
| Потомок | `Panel .desc` | Элемент с классом `.desc` внутри любого предка `Panel` |
| Дочерний | `Panel > Text.title` | Прямой потомок `Text.title` внутри `Panel` |
| Список | `Button, .clickable` | Применение блока правил к нескольким селекторам |

### Псевдоклассы состояний
- `:hover` — курсор мыши находится над элементом (или его дочерними узлами).
- `:active` — кнопка мыши зажата над элементом.
- `:focus` — элемент выбран в фокус ввода (клавиатура или геймпад).
- `:disabled` — элемент отключен (`enabled="false"` или `setEnabled(false)`).
- `:checked` — чекбокс или радиокнопка включена (`checked="true"` или `setChecked(true)`).
- `:first-child` — первый дочерний элемент у своего родителя.
- `:last-child` — последний дочерний элемент у своего родителя.

### Расчёт специфичности (Specificity)
Специфичность рассчитывается как кортеж `(idCount, classAndPseudoCount, tagCount)`:
1. `#id` имеет высший вес `(1, 0, 0)`.
2. `.class` и `:pseudo-class` имеют вес `(0, 1, 0)`.
3. Теги элементов (`Panel`, `Button`) имеют вес `(0, 0, 1)`.
4. Универсальный селектор `*` имеет вес `(0, 0, 0)`.

При равной специфичности побеждает правило, объявленное **позже** по исходному тексту (Source Order).

---

## 3. Переменные (`:root`)

CSS-переменные объявляются в блоке `:root` и подставляются с помощью `var(--name)` или с резервным значением `var(--name, fallback)`:

```css
:root {
    --gold: #c8a45a;
    --dark-bg: rgba(14, 18, 26, 0.94);
    --border-radius: 8;
    --card-pad: 16;
}

Panel.card {
    background: var(--dark-bg);
    border: 1 var(--gold);
    radius: var(--border-radius);
    padding: var(--card-pad);
}

Text.label {
    color: var(--missing-token, #ffffff);
}
```

Мутация переменной через `ui.setVar("--gold", "#00ffff")` мгновенно инвалидирует дерево стилей и обновляет зависимые свойства за **единицы микросекунд** без пересоздания виджетов.

---

## 4. Поддерживаемые свойства PSS

| PSS свойство | XML атрибут | Описание |
| :--- | :--- | :--- |
| `width`, `height` | `width`, `height` | Размеры (`px`, `%`, `auto`) |
| `min-width`, `min-height` | `min-width`, `min-height` | Минимальные размеры |
| `max-width`, `max-height` | `max-width`, `max-height` | Максимальные ограничения |
| `flex` | `flex` | Коэффициент расширения во flexbox |
| `flex-direction` / `direction` | `direction` | Направление компоновки (`row`, `column`) |
| `align-items` | `align-items` | Выравнивание по поперечной оси (`start`, `center`, `end`, `stretch`) |
| `justify-content` | `justify-content` | Распределение по главной оси (`start`, `center`, `end`, `space-between`, `space-around`, `space-evenly`) |
| `gap` | `gap` | Зазор между элементами |
| `padding` | `padding` | Внутренние отступы (1, 2 или 4 числа) |
| `margin` | `margin` | Внешние отступы |
| `background`, `background-color`, `bg` | `bg`, `background` | Фоновый цвет (Hex, RGBA, Named) или `linear-gradient(...)` |
| `border`, `border-color` | `border`, `border-color` | Составная граница (`[width] [color]` или `[color]`) |
| `border-width` | `border-width` | Толщина рамки |
| `border-radius`, `radius` | `radius` | Радиус скругления углов |
| `outline` | `outline` | Сокращенная запись контура (`outline: 1.5px solid #c8a45a`) |
| `outline-color` | `outline-color` | Цвет контура (Hex, RGBA, Named, var) |
| `outline-width` | `outline-width` | Толщина контура (`px`) |
| `outline-offset` | `outline-offset` | Смещение контура внутрь (`-3.5px`) или наружу (`4px`) |
| `border-image` | `border-image` | Текстурная 9-slice рамка (`"textures/frame.png" 16px`) |
| `border-image-slice` | `border-image-slice` | Нарезка текстуры на 9 квадов (1, 2 или 4 числа) |
| `shadow` | `shadow` | Мягкая тень (`true`, `false`) |
| `color` | `color` | Цвет текста |
| `font-size` | `font-size` | Размер шрифта в пикселях |
| `bold`, `font-weight` | `bold` | Полужирное начертание |
| `italic`, `font-style` | `italic` | Курсивное начертание |
| `wrap` | `wrap` | Автоматический перенос текста по ширине |
| `opacity` | `opacity` | Прозрачность (`0.0` .. `1.0`) |
| `blend-mode` | `blend-mode` | Режим наложения (`normal`, `additive`, `multiply`, `screen`) |
| `transform-scale` | `transform-scale` | Визуальный масштаб без изменения layout |
| `transform-offset` | `transform-offset` | Визуальное смещение (`px` или `%`) |
| `transform-pivot` | `transform-pivot` | Точка трансформации (`0.5 0.5`, `center`, `top left`, etc.) |
| `transition` | `transition` | Сокращенная запись плавных переходов |
| `transition-property` | `transition-property` | Список анимируемых свойств |
| `transition-duration` | `transition-duration` | Длительность переходов (`0.2s`, `150ms`) |
| `transition-timing-function` | `transition-timing-function` | Функция сглаживания (кривая) |
| `transition-delay` | `transition-delay` | Задержка перед началом анимации |

---

## 5. Иерархия приоритета источников (Cascading Priority)

Значения свойств элемента разрешаются строго в следующем порядке (от слабейшего к сильнейшему):

1. **Дефолтные значения виджета и темы** (`Theme::Current()`)
2. **Стили PSS** (правила селекторов с учетом специфичности и порядка загрузки файлов)
3. **Inline-атрибуты XML** (`<Panel width="300" bg="#ff0000">` побеждают правила PSS)
4. **Рантайм-вызовы C++ и Lua** (прямой вызов `element->setWidth(400)` или активная анимация `ElementAnimator` имеют абсолютный приоритет и не затираются каскадом PSS)

---

## 6. Плавные переходы (Transitions)

PSS поддерживает полностью декларативные CSS-переходы (CSS Transitions Level 1) при смене состояний элементов (`:hover`, `:active`, `:focus`, изменение классов или мутация переменных `:root`).

### Синтаксис `transition`

#### Shorthand
```css
/* Один переход */
Button {
    transition: background 0.2s ease;
}

/* Несколько свойств через запятую */
Button {
    transition: background 0.2s ease, border-color 0.2s ease, transform-scale 0.15s ease-out;
}

/* Универсальный селектор всех свойств с кастомным cubic-bezier */
Panel.card {
    transition: all 0.3s cubic-bezier(0.25, 0.1, 0.25, 1.0);
}

/* Отключение переходов */
Panel.instant {
    transition: none;
}
```

#### Longhands
```css
Button {
    transition-property: background, border-color, opacity;
    transition-duration: 0.2s, 150ms;
    transition-timing-function: ease, linear;
    transition-delay: 0s, 20ms;
}
```

### Анимируемые свойства
- `background` / `background-color` (Цвет, `Color`)
- `border-color` (Цвет рамки, `Color`)
- `border-width` (Толщина рамки, `float`)
- `corner-radius` / `radius` (Радиус скругления, `float`)
- `outline-color` (Цвет контура, `Color`)
- `outline-width` (Толщина контура, `float`)
- `outline-offset` (Смещение контура, `float`)
- `color` (Цвет текста, `Color`)
- `opacity` (Прозрачность, `float`)
- `transform-scale` (Масштаб, `Point`)
- `transform-offset` / `transform-offset-x` / `transform-offset-y` (Смещение, `Point` / `float`)
- `width` / `height` (Размеры, `float`, с мягкой инвалидацией layout)
- `all` (Применяется ко всем перечисленным свойствам)

### Функции сглаживания (Timing Functions)
- `linear` — равномерное движение
- `ease` — стандартное сглаживание CSS (`cubic-bezier(0.25, 0.1, 0.25, 1.0)`)
- `ease-in` — медленный старт
- `ease-out` — плавное замедление к концу
- `ease-in-out` — плавный старт и замедление
- `cubic-bezier(x1, y1, x2, y2)` — произвольная кубическая кривая Безье
- Игровые кривые PerfUI: `ease-out-bounce`, `ease-out-elastic`, `ease-in-quad`, `ease-out-quad`, `ease-in-cubic`, `ease-out-cubic`.
- **Физические пружины `spring(...)`**:
  - Именованные параметры: `spring(stiffness: 190, damping: 18, mass: 1)`
  - Позиционные параметры: `spring(200, 15)`
  - Физические пресеты: `spring(snappy)`, `spring(gentle)`, `spring(wobbly)`
  - Пружины используют высокоточный интегратор `PerfUI::SpringFloat` с реалистичным овершутом и автоматическим сохранением физической скорости при смене направления (reversal).

### Бесшовный реверс (Seamless Mid-Transition Reversal)
Если состояние элемента изменилось во время активного перехода (например, курсор быстро покинул кнопку при 40% завершения анимации), переключение разворачивается **от текущего визуального значения** назад к исходному. Скачков, мерцаний или телепортаций значений не происходит.

### Приоритет и доступность
1. **Приоритет явных анимаций C++**: Если для элемента запущена анимация через `ElementAnimator` (например, `btn->animate().alpha(0.0f, 0.3f)`), PSS transitions для этого свойства автоматически подавляются.
2. **Reduced Motion**: При включении режима доступности `isReducedMotion()` в `AnimationScheduler` все переходы завершаются мгновенно без интерполяции.
3. **Холодный старт**: При первичной загрузке дерева элементов стили выставляются мгновенно (без ложных переходов от 0 или черного цвета).

---

## 7. Производительность и ленивая инвалидация

1. **Dirty Subtree Marking**: При наведении мыши, нажатии или смене класса помечается `m_styleDirty = true` только на целевом элементе, а его предки получают легкий флаг `m_childStyleDirty = true`.
2. **Индексированные селекторы**: Таблица правил индексируется по ID, классам и тегам. При наведении на элемент перебираются только релевантные кандидаты правил.
3. **Разделение визуальных изменений и Layout**: Если вычисленный стиль изменил только цвета, прозрачность или рамку (`!hasLayoutDifferences`), пересчёт flex-раскладки не вызывается. Раскладка пересчитывается только при изменении размеров, отступов или flex-свойств.
4. **Zero-overhead StyleTransitionController**: Контроллер переходов выделяется лениво в виде `std::unique_ptr` только для тех элементов, у которых в стилях задан `transition`. Для статичных элементов накладные расходы составляют ровно 1 указатель (8 байт в памяти и 0 накладных расходов в цикле обновления).
5. **Замеры скорости**:
   - 5 000 элементов: холодный пересчёт всего дерева занимает **~4.5 мс**.
   - 5 000 элементов: пересчёт при смене класса/состояния одного элемента занимает **~0.12 мс**.

---

## 8. Продвинутая стилизация (Outline, Градиенты, 9-Slice)

### 8.1. Вложенные и внешние контуры (`outline`, `outline-offset`)
В отличие от `border`, свойство `outline` не занимает места в потоке flexbox-раскладки и поддерживает отрицательный `outline-offset`. Это позволяет легко создавать стильные двойные и вложенные окантовки:

```css
.card {
    border: 1.5px solid rgba(48, 58, 74, 0.85); /* Внешняя сталь */
    outline: 1px solid rgba(218, 175, 90, 0.25); /* Внутреннее золото */
    outline-offset: -3.5px; /* Смещение рамки внутрь элемента */
}
```

### 8.2. Линейные градиенты (`linear-gradient`)
Свойство `background` поддерживает 2-stop и 3-stop линейные градиенты без шейдерных аллокаций:

```css
/* Мягкое растворение арта в темноту */
.art-fade {
    background: linear-gradient(to bottom, transparent, rgba(11, 14, 20, 0.95));
}

/* 3-stop градиентный разделитель (transparent -> золото -> transparent) */
.gold-separator {
    background: linear-gradient(to right, transparent, #daaf5a, transparent);
}
```

Поддерживаются направления: `to bottom`, `to top`, `to right`, `to left`.

### 8.3. Текстурные рамки 9-Slice (`border-image`)
Позволяет оформлять интерфейсы через текстурные атласы рамок без единой строчки C++:

```css
.scroll-panel {
    border-image: "textures/frame.png" 16px;
}

/* Раздельная нарезка (top, right, bottom, left) */
.custom-panel {
    border-image: "textures/frame.png" 10px 12px 14px 16px;
}
```

Текстура автоматически загружается через контекст и нарезается на 9 квадов.

### 8.4. Пример комплексной стилизации меню-карточки

```css
/* Базовое состояние карточки */
.menu-card {
    width: 235px;
    height: 350px;
    border-radius: 8px;
    background-color: rgba(14, 18, 26, 0.9);
    
    /* Двойная рамка: внешняя сталь + вложенное золото */
    border: 1.5px solid rgba(48, 58, 74, 0.85);
    outline: 1px solid rgba(218, 175, 90, 0.25);
    outline-offset: -3.5px;
    
    /* Тень */
    box-shadow: true;
    
    /* Физика пружины для подъёма */
    transform-offset-y: 0px;
    transition: transform-offset-y 0.35s spring(stiffness: 190, damping: 18),
                transform-scale 0.35s spring(stiffness: 190, damping: 18),
                outline-color 0.2s ease;
}

/* При наведении — физический подъём и микро-зум */
.menu-card:hover {
    transform-offset-y: -22px;
    transform-scale: 1.03;
    outline: 1px solid rgba(218, 175, 90, 0.65);
}

/* Soft gradient fade for banner */
.card-image-fade {
    height: 68px;
    background: linear-gradient(to bottom, transparent, rgba(11, 14, 20, 0.95));
}
```

---

## 9. Единицы измерения экрана (Viewport Units: `vw`, `vh`, `vmin`, `vmax`)

PSS и разметка XML поддерживают адаптивные единицы экрана, автоматически рассчитываемые от текущего разрешения окна рендера (`UIContext::viewportSize()`):

| Единица | Описание | Пример |
| :--- | :--- | :--- |
| `vw` | Процент от ширины экрана (1vw = 1%) | `width: 50vw;` (половина ширины экрана) |
| `vh` | Процент от высоты экрана (1vh = 1%) | `height: 80vh;` (80% высоты экрана) |
| `vmin` | Процент от наименьшей стороны экрана | `width: 25vmin;` |
| `vmax` | Процент от наибольшей стороны экрана | `max-width: 90vmax;` |

### Применение в свойствах
Viewport units поддерживаются для:
- Размеров: `width`, `height`, `min-width`, `max-width`, `min-height`, `max-height`.
- Абсолютного позиционирования и якорей: `left`, `right`, `top`, `bottom`.

> [!NOTE]
> Свойства `padding`, `margin`, `gap`, `border-radius`, `font-size` принимают значения в пикселях или токены темы ($spacing, $radius). Свойство `transform-offset` принимает пиксели и проценты (`px`, `%`). При изменении разрешения экрана или переключении на ультравайд мониторы (21:9 `3440×1440`, 32:9 `5120×1440`) все единицы `vw/vh/vmin/vmax` динамически пересчитываются без перезагрузки интерфейса.

```css
.modal-window {
    width: 60vw;
    height: 70vh;
    left: 20vw;
    top: 15vh;
}
```

---

## 10. Ключевые кадры и анимации (`@keyframes` и `animation`)

Для сложных циклических и многошаговых визуальных эффектов (пульсация, вращение, мигание маркеров, свечение) PSS поддерживает директиву `@keyframes` и свойство `animation`, полностью согласующиеся со стандартами CSS.

### 10.1. Объявление `@keyframes`

```css
@keyframes pulseGlow {
    0% {
        opacity: 0.4;
        transform-scale: 1.0;
        animation-timing-function: ease-out;
    }
    50% {
        opacity: 1.0;
        transform-scale: 1.08;
        animation-timing-function: ease-in;
    }
    100% {
        opacity: 0.4;
        transform-scale: 1.0;
    }
}

@keyframes slideInFromLeft {
    to {
        transform-offset-x: 0px;
        opacity: 1;
    }
}
```

Особенности синтаксиса и разбора:
- Ключевые слова `from` (эквивалентно `0%`) и `to` (эквивалентно `100%`).
- Промежуточные точки (`0%`..`100%`) в любом порядке: парсер автоматически сортирует шаги по возрастанию `offset`.
- Дубликаты смещений (`offset`): при повторе процентного шага свойства объединяются, а при совпадении свойств побеждает объявленное позже.
- Предупреждения: использование неанимируемых свойств в теле `@keyframes` генерирует диагностическое предупреждение с указанием `файл:строка:колонка`.

### 10.2. Свойство `animation`

Сокращённый синтаксис (shorthand):
```css
/* name duration timing-function delay iteration-count direction fill-mode play-state */
.pulsing-badge {
    animation: pulseGlow 1.8s ease-in-out infinite alternate both;
}
```

Отдельные свойства (longhands):
- `animation-name`: имя правила `@keyframes`.
- `animation-duration`: длительность цикла (`1.5s`, `300ms`).
- `animation-timing-function`: кривая плавности (`linear`, `ease`, `ease-in`, `ease-out`, `ease-in-out`, `cubic-bezier(...)`, `spring(...)`).
- `animation-delay`: задержка перед стартом (`0.5s`).
- `animation-iteration-count`: целое или дробное число повторов (например, `1.5`) или `infinite` (бесконечно).
- `animation-direction`: `normal`, `reverse`, `alternate`, `alternate-reverse`.
- `animation-fill-mode`: `none`, `forwards` (сохраняет конечный кадр), `backwards`, `both`.
- `animation-play-state`: `running`, `paused`.

### 10.3. Runtime поведение и соответствие стандарту CSS

1. **Возврат к базовому стилю при завершении и отмене**:
   - По завершении конечной анимации без `fill-mode: forwards` или `both` элемент автоматически возвращается к базовым значениям, вычисленным каскадом стилей.
   - При динамическом удалении класса стилей или отмене анимации базовое состояние восстанавливается немедленно.
   - Рантайм-установки C++/Lua и явные аниматоры `ElementAnimator` имеют более высокий приоритет и не перезаписываются стилями.

2. **Неявные ключевые кадры (Implicit Keyframes)**:
   - Если в `@keyframes` отсутствует шаг `0%` (`from`) или `100%` (`to`), недостающее граничное значение автоматически берется из текущего вычисленного базового стиля элемента.

3. **Независимые треки свойств (Independent Property Tracks)**:
   - Если свойство задано только в некоторых шагах (например, `0%` и `100%`), а в промежуточных (`50%`) отсутствует, интерполяция свойства продолжается гладко между его ближайшими опорными точками, не прерываясь другими свойствами.

4. **Посегментный Easing (Per-Segment Easing)**:
   - Сглаживание применяется строго внутри каждого отрезка между соседними кадрами (`segT`). При наличии у ключевого кадра собственного `animation-timing-function` используется он, иначе действует общий `easing` анимации. Двойное искажение общего прогресса итерации исключено.

5. **Дробные циклы (Fractional Iterations)**:
   - При дробном `animation-iteration-count` (например, `1.5` с `forwards`) анимация корректно останавливается на прогрессе последней незавершённой доли цикла с учётом выбранного направления (`direction`).

6. **Режим доступности (Reduced Motion)**:
   - При флаге `reducedMotion: true` конечные анимации завершаются мгновенно (с сохранением финала при `forwards` либо возвратом к базе), а бесконечные (`infinite`) фиксируются на начальном кадре.

7. **Анимация размеров и восстановление `DimensionConstraint`**:
   - Анимация свойств `width` и `height` в keyframes каждый кадр вызывает пересчет flex-раскладки. После завершения анимации исходный режим размера (`SizeMode::Percent`, `Auto` и т.д.) полностью восстанавливается.
   > [!TIP]
   > Для максимальной производительности в 60+ FPS рекомендуется предпочитать визуальное масштабирование через `transform-scale`, не вызывающее инвалидацию макета.

