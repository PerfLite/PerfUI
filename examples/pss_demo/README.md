# PerfUI Style Sheets (PSS) Showcase Demo

This example demonstrates the PSS subsystem in action:

- **Decoupled Styling**: All element styles, colors, padding, borders, shadows, and hover states are defined strictly in `styles.pss`.
- **Pseudo-Classes**: Hover (`:hover`), active (`:active`), focus (`:focus`), disabled (`:disabled`), and checked (`:checked`) states trigger dynamically without writing custom C++ widget code.
- **Runtime CSS Variables**: Clicking "Switch Color Accent" updates `:root` variables (`--accent`, `--accent-hover`, etc.) via `ui.setVar`, instantly recalculating dependent component styles across the tree without re-instantiating widgets.
- **Zero Layout Overhead on Visual State Changes**: Pure visual changes (colors, opacity, borders) do not trigger flex layout recalculation.

## Files
- `menu.xml` - XML UI markup importing `styles.pss` and binding to `main.lua`.
- `styles.pss` - CSS-like PSS stylesheet.
- `main.lua` - Event handlers manipulating CSS variables and element classes.
