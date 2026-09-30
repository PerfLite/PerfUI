# PerfUI Architecture Specification

## 1. Vision & Architecture Overview

**PerfUI** is an independent, high-performance retained-mode UI framework specifically tailored for *The Elder Scrolls V: Skyrim Special Edition / Anniversary Edition*. Its mission is to enable fluid, beautiful, controller-friendly in-game menus (Journal, Main Menu, Inventory, Character, Map, HUD) and provide third-party modders with an intuitive modern C++ API.

### 1.1 The Golden Principle: Strict Backend Decoupling
Dear ImGui is used **solely** as an initial rendering backend foundation (`ImDrawList`). The public API and Core of PerfUI **never** expose or include `<imgui.h>`. All visual primitives are submitted through an abstract `UIRenderBackend` interface. This ensures that ImGui can later be replaced with a native D3D11/Vulkan/D3D12 renderer without modifying a single line of user or widget code.

```text
┌────────────────────────────────────────────────────────────────────────┐
│                          Game / Mod Logic                              │
│         Journal Menu, Custom Mod Interfaces, Player Inventory          │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        PerfUI Public API Layer                         │
│     UIContext, UIWindow, Panel, Button, Text, ScrollView, etc.         │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                              PerfUI Core                               │
│  ├── Retained UI Tree (UIElement, Widget, Container)                   │
│  ├── Layout Engine (Flex/Stack, Box Model, Anchor, Auto-Size)          │
│  ├── Styling & Theme Engine (States, Brushes, Fonts, Metrics)          │
│  ├── Animation Engine (Interpolation, Easing, Timelines)               │
│  ├── Input & Navigation (Focus Graph, Gamepad D-Pad, Input Capture)   │
│  └── Event Dispatcher (Bubble, Tunnel, Direct Callbacks)               │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       UIRenderBackend Interface                        │
│   beginFrame(), drawRect(), drawRoundedRect(), drawText(), etc.        │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        Backend Implementations                         │
│  ┌───────────────────────────────────┐  ┌───────────────────────────┐  │
│  │   ImGuiBackend (D3D11 / ImDrawList)│  │ Standalone / Native D3D11 │  │
│  └───────────────────────────────────┘  └───────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Layer Definitions

### 2.1 Core Subsystem (`src/core/`)
Responsible for object lifecycles, memory, hierarchy, and per-frame orchestration.
- **`UIContext`**: The root manager for a UI instance. Owns the root node, input dispatcher, style manager, and render bridge.
- **`UIElement` / `Node`**: Abstract base class for everything in the retained tree.
  - Maintains `parent` (observer pointer) and `children` (`std::vector<std::unique_ptr<UIElement>>`).
  - Manages dirty flags: `LayoutDirty`, `StyleDirty`, `RenderDirty`.
  - Holds unique ID (`ElementId`), visibility, and enabled state.
- **`UIWindow`**: Top-level root container representing an interactive window or full-screen menu overlay.
- **`EventSystem`**: Type-safe event dispatching (mouse clicks, key down/up, gamepad actions, focus change) with event bubbling up the hierarchy and event consumption.

### 2.2 Layout Engine (`src/layout/`)
Performs 2-pass layout calculation independent of rendering:
1. **Measure Pass**: Bottom-up traversal where each element determines its desired size given parent constraints (`availableWidth`, `availableHeight`).
2. **Arrange Pass**: Top-down traversal where parents assign final `Rect` (position and size) to children based on:
   - Direction: `Horizontal` or `Vertical` stack (flex-like flow).
   - Sizing: `Auto` (shrink-to-content), `Fixed` (exact pixels), `Percent` (% of parent), `Flex` (weighted share of remaining space).
   - Padding & Margin: Box model edge insets.
   - Alignment: `Start`, `Center`, `End`, `Stretch`.
   - Gap: Constant spacing between adjacent items in container.
   - Clipping & Scrolling: Containers with overflow assign clipping regions and compute scroll offsets.

### 2.3 Styling & Theme System (`src/style/`)
Decouples visual appearance from logic.
- **`Style`**: Collection of visual attributes:
  - `backgroundColor`, `borderColor`, `borderWidth`, `borderRadius`
  - `textColor`, `fontFamily`, `fontSize`
  - `opacity`, `shadowColor`, `shadowBlur`, `shadowOffset`
  - `padding`, `margin`
- **`StyleState`**: Dynamic states (`Normal`, `Hovered`, `Pressed`, `Focused`, `Disabled`, `Selected`). Styles are resolved per-frame based on element state.
- **`Theme`**: Central palette and default component styles (e.g. `SkyrimDefault`, `NordicDark`, `MinimalModern`).

### 2.4 Input & Gamepad Navigation (`src/input/`, `src/navigation/`)
Skyrim is played heavily with controllers. The input engine is designed **Gamepad-First**:
- **Focus Graph**: Retained elements marked `Focusable` form a 2D spatial graph.
- **Directional Navigation**: When the user presses D-Pad (Up/Down/Left/Right) or moves the left stick, the navigation system finds the geometrically nearest focusable candidate in that direction.
- **Action Mapping**:
  - `Submit`: Gamepad `A` / Keyboard `Enter` / `Space`
  - `Cancel` / `Back`: Gamepad `B` / Keyboard `Escape`
  - `TabPrev` / `TabNext`: Gamepad `LB` / `RB` / Keyboard `Q` / `E`
- **Modal Windows**: Windows can capture focus exclusively, suspending background navigation until dismissed.
- **Input Capture**: When PerfUI menu is active, it requests Skyrim's `BSInputEventQueue` or Windows message loop to swallow game controls to prevent weapon swinging or camera moving while navigating menus.

### 2.5 Animation System (`src/animation/`)
Independent interpolation engine driven by delta time:
- Supports numeric properties: `Opacity`, `PositionX/Y`, `Scale`, `Size`, `Color`.
- Easing functions: `Linear`, `InQuad`, `OutQuad`, `InOutQuad`, `InCubic`, `OutCubic`, `OutBack`, `OutBounce`.
- Fluent builder syntax:
  ```cpp
  element->animate()
      .opacity(0.0f, 1.0f)
      .duration(200ms)
      .ease(Easing::OutCubic);
  ```

### 2.6 Rendering Abstraction (`src/backends/`)
The interface between PerfUI Core and the low-level graphics API:
```cpp
class UIRenderBackend {
public:
    virtual ~UIRenderBackend() = default;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void pushClipRect(const Rect& rect) = 0;
    virtual void popClipRect() = 0;

    virtual void drawRect(const Rect& rect, Color color) = 0;
    virtual void drawRoundedRect(const Rect& rect, Color color, float radius, Color borderColor = {}, float borderWidth = 0.0f) = 0;
    virtual void drawText(const std::string_view text, const Point& position, const TextStyle& style) = 0;
    virtual void drawImage(TextureHandle texture, const Rect& rect, Color tint = Color::White) = 0;
    virtual void drawShadow(const Rect& rect, float radius, Color shadowColor, float blurRadius, const Point& offset) = 0;
};
```
- **`ImGuiBackend`**: Implements `UIRenderBackend` using `ImDrawList`. Translates PerfUI commands into `ImDrawList::AddRectFilled`, `AddText`, etc.
- Core never references `ImDrawList` directly.

### 2.7 Skyrim Integration Boundary (`src/skyrim/`)
Isolates game engine dependencies from UI logic:
- Communicates with SKSE and CommonLibSSE-NG.
- Hooks D3D11 `IDXGISwapChain::Present` to invoke PerfUI frame rendering.
- Hooks input pipelines to toggle menu mode and capture game inputs.
- Wraps Skyrim game data into pure C++ structures (e.g. `Skyrim::Quest`, `Skyrim::InventoryItem`) without leaking Bethesda game types into UI widgets.

---

## 3. Ownership & Memory Management Model

1. **Hierarchy Ownership**:
   - `UIContext` owns the root `UIWindow` objects via `std::vector<std::unique_ptr<UIWindow>>`.
   - Containers own child `UIElement` instances via `std::vector<std::unique_ptr<UIElement>>`.
   - Raw pointers (`UIElement*`) are used strictly as non-owning observer handles for convenience.
2. **Safe Referencing**:
   - Every element has an immutable `ElementId` (64-bit integer or hashed string).
   - If an element is destroyed, any focus or hover references to its `ElementId` are automatically invalidated in `UIContext`.
3. **Zero Dynamic Allocations in Hot Paths**:
   - Layout calculation reuses scratch buffers across frames.
   - Draw command generation flushes into pre-allocated vertex/command lists.

---

## 4. Threading & Execution Pipeline

Skyrim executes logic across multiple threads (Havok physics, AI, TaskInterface) while rendering occurs on the primary render thread in `Present()`.
- **Render Thread**:
  1. `D3D11 Present Hook` fires.
  2. Preserve D3D11 render state (shaders, blend states, rasterizer state, viewports).
  3. `UIContext::update(deltaTime)`: process animations, handle input events from the queue.
  4. `UIContext::layout()`: recompute layout if `LayoutDirty`.
  5. `UIContext::render(backend)`: traverse tree and emit drawing primitives to `UIRenderBackend`.
  6. Restore D3D11 render state.
- **Logic / Game Thread**:
  - SKSE tasks and game events post messages to a thread-safe MPSC (Multi-Producer Single-Consumer) queue in `UIContext` to avoid race conditions with rendering.
