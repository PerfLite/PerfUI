# PerfUI Style Sheets (PSS) Specification & Modder Guide (English)

[English](PSS_EN.md) | [Русский](pss.md)

**PerfUI Style Sheets** (`.pss`) is a lightweight, declarative styling engine implementing a clean subset of CSS designed specifically for high-performance game user interfaces in *The Elder Scrolls V: Skyrim SE / AE / VR* and standalone DirectX 11 applications.

PSS completely decouples visual presentation, animations, and theme styling from markup structure (`.xml`) and gameplay/UI logic (`.lua` / C++), eliminating the memory and latency overhead of heavy Chromium or Flash/Scaleform runtimes.

---

## 1. Connecting Stylesheets

### From XML Markup
Stylesheets are linked via the `styles` attribute in the root `<UI>` tag or via inner `<Styles>` tags:

```xml
<UI styles="theme.pss custom.pss" script="menu.lua">
    <!-- Or explicit imports within the document -->
    <Styles src="extra.pss" />

    <Panel class="card">
        <Text class="title" text="Header Title" />
    </Panel>
</UI>
```

Multiple stylesheet files are applied in declaration order. When selector specificity is equal, properties declared in later files override earlier ones.

### From Lua API
```lua
-- Dynamically reload all connected stylesheets without reconstructing the widget tree
ui.reloadStyles()

-- Read and modify CSS variables at runtime
ui.setVar("--accent", "#00d2ff")
local accent = ui.getVar("--accent")

-- Element class manipulation
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

### From C++ API
```cpp
// Reload stylesheets across the UI context
context.reloadStyles();

// Mutate CSS variables at runtime
context.setCssVariable("--accent", "#ff5500");

// Class manipulation on any element
element->addClass("active");
element->removeClass("disabled");
element->toggleClass("expanded");
```

---

## 2. Selectors and Specificity

PSS supports the following selector formats:

| Selector | Example | Description |
| :--- | :--- | :--- |
| `*` | `* { opacity: 1; }` | Universal selector |
| Widget Tag | `Button { height: 32; }` | Matches widget type (case-insensitive) |
| Class | `.card { radius: 8; }` | Matches elements with the given class |
| ID | `#submitBtn { width: 200; }` | Matches elements by unique identifier (`id` or `name`) |
| Compound | `Panel.card.big` | Element of type `Panel` with both `.card` and `.big` classes |
| Descendant | `Panel .desc` | Element with class `.desc` nested inside any ancestor `Panel` |
| Direct Child | `Panel > Text.title` | Direct child `Text.title` immediately inside `Panel` |
| Comma List | `Button, .clickable` | Applies the rule block to multiple selector targets |

### State Pseudo-Classes
- `:hover` — pointer is positioned over the element (or its children).
- `:active` — pointer button (or gamepad action) is pressed over the element.
- `:focus` — element currently possesses input focus (keyboard or gamepad).
- `:disabled` — element is disabled (`enabled="false"` or `setEnabled(false)`).
- `:checked` — checkbox or radio button is toggled on (`checked="true"` or `setChecked(true)`).
- `:first-child` — element is the first child among its parent's children.
- `:last-child` — element is the last child among its parent's children.

### Specificity Calculation
Specificity follows the standard CSS tuple `(idCount, classAndPseudoCount, tagCount)`:
1. `#id` carries the highest weight `(1, 0, 0)`.
2. `.class` and `:pseudo-class` carry weight `(0, 1, 0)`.
3. Widget tags (`Panel`, `Button`) carry weight `(0, 0, 1)`.
4. The universal selector `*` carries weight `(0, 0, 0)`.

When two rules have equal specificity, the rule declared **later in source order** prevails.

---

## 3. CSS Variables (`:root`)

CSS custom properties (variables) are defined in the `:root` pseudo-class and referenced using `var(--name)` or `var(--name, fallback)`:

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

Mutating a CSS variable via `ui.setVar("--gold", "#00ffff")` lazily invalidates the style subtree and updates all dependent properties within **single-digit microseconds** without re-instantiating widgets.

---

## 4. Supported PSS Properties

| PSS Property | XML Attribute | Description |
| :--- | :--- | :--- |
| `width`, `height` | `width`, `height` | Dimensions (`px`, `%`, `auto`) |
| `min-width`, `min-height` | `min-width`, `min-height` | Minimum dimension constraints |
| `max-width`, `max-height` | `max-width`, `max-height` | Maximum dimension constraints |
| `flex` | `flex` | Flexbox expansion weight |
| `flex-direction` / `direction` | `direction` | Layout orientation (`row`, `column`) |
| `align-items` | `align-items` | Cross-axis alignment (`start`, `center`, `end`, `stretch`) |
| `justify-content` | `justify-content` | Main-axis distribution (`start`, `center`, `end`, `space-between`, `space-around`, `space-evenly`) |
| `gap` | `gap` | Spacing between sibling elements |
| `padding` | `padding` | Internal padding (1, 2, or 4 numbers) |
| `margin` | `margin` | External margin (1, 2, or 4 numbers) |
| `background`, `background-color`, `bg` | `bg`, `background` | Background fill (Hex, RGBA, Named) or `linear-gradient(...)` |
| `border`, `border-color` | `border`, `border-color` | Composite border (`[width] [color]` or `[color]`) |
| `border-width` | `border-width` | Border line thickness |
| `border-radius`, `radius` | `radius` | Corner radius |
| `outline` | `outline` | Shorthand outline definition (`outline: 1.5px solid #c8a45a`) |
| `outline-color` | `outline-color` | Outline stroke color (Hex, RGBA, Named, var) |
| `outline-width` | `outline-width` | Outline stroke thickness (`px`) |
| `outline-offset` | `outline-offset` | Inset (`-3.5px`) or outset (`4px`) distance |
| `border-image` | `border-image` | Nine-slice texture frame (`"textures/frame.png" 16px`) |
| `border-image-slice` | `border-image-slice` | Nine-slice pixel metrics (1, 2, or 4 numbers) |
| `shadow` | `shadow` | Drop shadow (`true`, `false`) |
| `color` | `color` | Text color |
| `font-size` | `font-size` | Text font size in pixels |
| `bold`, `font-weight` | `bold` | Bold font weight |
| `italic`, `font-style` | `italic` | Italic font style |
| `wrap` | `wrap` | Text wrapping width |
| `opacity` | `opacity` | Element opacity (`0.0` .. `1.0`) |
| `blend-mode` | `blend-mode` | Rasterization blending mode (`normal`, `additive`, `multiply`, `screen`) |
| `transform-scale` | `transform-scale` | Visual scale without modifying layout bounds |
| `transform-offset` | `transform-offset` | Visual translation (`px` or `%`) |
| `transform-pivot` | `transform-pivot` | Transformation anchor (`0.5 0.5`, `center`, `top left`, etc.) |
| `transition` | `transition` | Shorthand declaration for smooth transitions |
| `transition-property` | `transition-property` | Target properties for transition |
| `transition-duration` | `transition-duration` | Duration of transition (`0.2s`, `150ms`) |
| `transition-timing-function` | `transition-timing-function` | Easing curve function |
| `transition-delay` | `transition-delay` | Delay before transition interpolation begins |

---

## 5. Cascading Priority Hierarchy

Element property values are resolved strictly according to the following precedence order (from lowest to highest):

1. **Widget Defaults & Theme** (`Theme::Current()`)
2. **PSS Stylesheets** (matching rules sorted by specificity and source order)
3. **XML Inline Attributes** (`<Panel width="300" bg="#ff0000">` overrides PSS rules)
4. **Runtime C++ and Lua API Calls** (direct setter calls like `element->setWidth(400)` or active `ElementAnimator` tracks have absolute priority and are never overridden by PSS cascading)

---

## 6. Smooth Transitions (CSS Transitions)

PSS supports fully declarative CSS transitions (CSS Transitions Level 1) when elements change states (`:hover`, `:active`, `:focus`, class alterations, or `:root` variable changes).

### `transition` Syntax

#### Shorthand
```css
/* Single property transition */
Button {
    transition: background 0.2s ease;
}

/* Multiple comma-separated properties */
Button {
    transition: background 0.2s ease, border-color 0.2s ease, transform-scale 0.15s ease-out;
}

/* Transition all animatable properties with custom cubic-bezier */
Panel.card {
    transition: all 0.3s cubic-bezier(0.25, 0.1, 0.25, 1.0);
}

/* Disable transitions */
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

### Animatable Properties
- `background` / `background-color` (Fill color, `Color`)
- `border-color` (Border stroke color, `Color`)
- `border-width` (Border thickness, `float`)
- `corner-radius` / `radius` (Corner rounding, `float`)
- `outline-color` (Outline color, `Color`)
- `outline-width` (Outline thickness, `float`)
- `outline-offset` (Outline offset, `float`)
- `color` (Text foreground color, `Color`)
- `opacity` (Alpha transparency, `float`)
- `transform-scale` (Visual scaling, `Point`)
- `transform-offset` / `transform-offset-x` / `transform-offset-y` (Visual offset, `Point` / `float`)
- `width` / `height` (Dimensions, `float`, with non-blocking layout invalidation)
- `all` (Applies to all animatable properties above)

### Timing Functions
- `linear` — constant velocity
- `ease` — standard CSS easing curve (`cubic-bezier(0.25, 0.1, 0.25, 1.0)`)
- `ease-in` — slow start
- `ease-out` — smooth deceleration towards the end
- `ease-in-out` — smooth acceleration and deceleration
- `cubic-bezier(x1, y1, x2, y2)` — arbitrary cubic Bézier spline parameters
- Game easing curves: `ease-out-bounce`, `ease-out-elastic`, `ease-in-quad`, `ease-out-quad`, `ease-in-cubic`, `ease-out-cubic`.
- **Physical Spring Dynamics `spring(...)`**:
  - Named parameters: `spring(stiffness: 190, damping: 18, mass: 1)`
  - Positional syntax: `spring(200, 15)`
  - Physics presets: `spring(snappy)`, `spring(gentle)`, `spring(wobbly)`
  - Driven by second-order damped harmonic oscillator (`PerfUI::SpringFloat`) with natural overshoot and continuous velocity preservation during reversals.

### Seamless Mid-Transition Reversal
If an element's state changes while a transition is actively interpolating (e.g., the pointer leaves a button at 40% animation progress), the interpolation reverses **from the current visual value** back towards the resting state. There are no visual jumps, snaps, or flickering.

### Priority Protection and Accessibility
1. **Explicit C++ Animation Priority**: If an element is actively being animated via C++ `ElementAnimator` (e.g., `btn->animate().alpha(0.0f, 0.3f)`), PSS CSS transitions for that property are automatically suppressed.
2. **Reduced Motion**: When `isReducedMotion()` accessibility mode is active on `AnimationScheduler`, all transitions complete instantly without time interpolation.
3. **Cold Mount**: On initial widget tree mounting, resting styles apply instantly without transitions from zero or black.

---

## 7. Performance and Invalidation Architecture

1. **Dirty Subtree Marking**: State changes (hover, press, class mutation) mark `m_styleDirty = true` only on the target element, setting `m_childStyleDirty = true` along the ancestor path to the root.
2. **Indexed Rule Lookups**: Rules are partitioned into hash indexes by ID, class, and tag name. Cascade evaluation tests only candidate rules matching the target element.
3. **Separation of Visual and Layout Passes**: When computed styles only alter visual traits (colors, borders, shadows, opacity) without altering dimension or flex properties (`!hasLayoutDifferences`), flex layout reflow is skipped.
4. **Zero-Overhead Transition Controller**: `m_transitions` is allocated lazily as `std::unique_ptr<StyleTransitionController>` only for elements with declared transitions. Static elements incur a single 8-byte pointer check in the frame update loop.
5. **Benchmarks**:
   - 5,000 elements: cold cascade computation of the full tree takes **~4.5 ms**.
   - 5,000 elements: single element state/class transition cascade takes **~0.12 ms**.
   - 100 simultaneous active button transitions: per-frame update cost is **< 0.02 ms**.

---

## 8. Advanced Styling (Outline, Gradients, 9-Slice)

### 8.1. Inset and Outset Contours (`outline`, `outline-offset`)
Unlike `border`, `outline` does not take up room in flexbox layout and supports negative `outline-offset`, creating inner frames over the element background:

```css
.card {
    border: 1.5px solid rgba(48, 58, 74, 0.85); /* Outer steel */
    outline: 1px solid rgba(218, 175, 90, 0.25); /* Inner gold filigree */
    outline-offset: -3.5px; /* Inset frame position */
}
```

### 8.2. Linear Gradients (`linear-gradient`)
The `background` property supports 2-stop and 3-stop linear gradients without custom shader passes:

```css
/* Smooth fade to darkness */
.art-fade {
    background: linear-gradient(to bottom, transparent, rgba(11, 14, 20, 0.95));
}

/* 3-stop gradient divider (transparent -> gold -> transparent) */
.gold-separator {
    background: linear-gradient(to right, transparent, #daaf5a, transparent);
}
```

Supported directions: `to bottom`, `to top`, `to right`, `to left`.

### 8.3. Nine-Slice Texture Frames (`border-image`)
Enables AAA texture frame borders without C++ code:

```css
.scroll-panel {
    border-image: "textures/frame.png" 16px;
}

/* Distinct slices (top, right, bottom, left) */
.custom-panel {
    border-image: "textures/frame.png" 10px 12px 14px 16px;
}
```

### 8.4. AAA Menu Card Example

```css
/* Card resting state */
.menu-card {
    width: 235px;
    height: 350px;
    border-radius: 8px;
    background-color: rgba(14, 18, 26, 0.9);
    
    /* Double frame: outer steel + inset gold */
    border: 1.5px solid rgba(48, 58, 74, 0.85);
    outline: 1px solid rgba(218, 175, 90, 0.25);
    outline-offset: -3.5px;
    
    /* Drop shadow */
    box-shadow: true;
    
    /* Spring physics for lift */
    transform-offset-y: 0px;
    transition: transform-offset-y 0.35s spring(stiffness: 190, damping: 18),
                transform-scale 0.35s spring(stiffness: 190, damping: 18),
                outline-color 0.2s ease;
}

/* Hover state — physical lift and micro-zoom */
.menu-card:hover {
    transform-offset-y: -22px;
    transform-scale: 1.03;
    outline: 1.5px solid rgba(218, 175, 90, 0.65);
}

/* Soft gradient fade for banner */
.card-image-fade {
    height: 68px;
    background: linear-gradient(to bottom, transparent, rgba(11, 14, 20, 0.95));
}
```
