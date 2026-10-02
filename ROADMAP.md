# PerfUI Development Roadmap

This roadmap defines the step-by-step development process of **PerfUI**. Every phase must culminate in a working, verifiable state before proceeding to the next.

---

## Phase Overview

```text
Phase 0  ──► Architecture & Research (Clean foundations, documents, CMake structure)
Phase 1  ──► Minimal SKSE + ImGui D3D11 Hook (First stable in-game window & hotkey)
Phase 2  ──► Backend Abstraction (UIRenderBackend decoupling)
Phase 3  ──► Retained UI Tree (UIContext, Element, Window, Lifecycle)
Phase 4  ──► Layout Engine (Flex/Stack Box Model, Auto-sizing, Scroll)
Phase 5  ──► Core Widgets (Panel, Text, Button, ScrollView, etc.)
Phase 6  ──► Styling & Themes (States, Brushes, Border, Fonts, Default Theme)
Phase 7  ──► Input & Navigation (Gamepad-First 2D Focus Graph, Actions)
Phase 8  ──► Animation System (Timelines, Tweens, Easing curves)
Phase 9  ──► Rendering Improvements (Shadows, Rounded corners, Masks, Icons)
Phase 10 ──► Skyrim Integration (SKSE/CommonLibSSE Quest & Event APIs)
Phase 11 ──► Journal Prototype (The First Big Test)
Phase 12 ──► Stress & Performance Testing (100+ widgets, Alt-Tab, Memory leaks)
Phase 13 ──► Main Menu Prototype
Phase 14 ──► Developer API & Documentation
Phase 15 ──► Backend Independence Test (Mock/Second Backend)
Phase 16 ──► PerfUI SDK Packaging
Phase 17 ──► Advanced Features (Live Reload, In-game UI Inspector, Profiler)
```

---

## Phase Details & Exit Criteria

### Phase 0: Research & Architecture (COMPLETED)
- [x] Project directory layout (`include/PerfUI`, `src/core`, `src/backends`, etc.).
- [x] Version control initialized (`.git`, `.gitignore`).
- [x] Move ImGui to `third_party/imgui/`.
- [x] Document core architecture (`ARCHITECTURE.md`).
- [x] Document roadmap and milestones (`ROADMAP.md`).
- [x] Document design system and API ergonomics (`DESIGN.md`).
- [x] Configure root `CMakeLists.txt` and standalone sandbox project (`PerfUI_Sandbox`).
*Exit Criteria:* All design specs completed; CMake and Sandbox build cleanly.

### Phase 1: Minimal SKSE + ImGui Prototype (COMPLETED & DEPLOYED)
- [x] Setup CommonLibSSE-NG dependency (via xmake & local cache).
- [x] Implement DXGI Present & ResizeBuffers hooks for Skyrim D3D11.
- [x] State preservation: save and restore DirectX 11 pipeline state before/after rendering.
- [x] Hotkey toggle (`F11`) to open/close test window with `ESC` to close.
- [x] Input capture: swallow mouse and keyboard from Skyrim camera/controls when UI is open.
- [x] Auto-deploy script (`deploy.ps1`) to Mod Organizer 2 mods and profiles.
*Exit Criteria:* `PerfUI.dll` compiles and deploys cleanly to MO2, ready for in-game verification.

### Phase 2: Backend Abstraction (COMPLETED)
- [x] Define pure abstract interface: `UIRenderBackend`.
- [x] Implement `ImGuiRenderBackend` in `src/backends/imgui/` using `ImDrawList`.
- [x] Zero `<imgui.h>` includes in Core headers and public includes.
- [x] Validate that Core compiles with zero awareness of ImGui.
*Exit Criteria:* Core contains 0 ImGui references; rendering flows entirely through `UIRenderBackend`.

### Phase 3: Retained UI Tree
- Implement `UIContext`, `UIElement`, `Container`, `UIWindow`.
- Parent/child lifecycle management (`std::unique_ptr` ownership, observer handles).
- Dirty flags system (`LayoutDirty`, `RenderDirty`, `StyleDirty`).
- Tree traversal (measure, arrange, hit test, render).
*Exit Criteria:* UI is constructed via retained hierarchy (`window->add<Container>()`), not immediate frame-by-frame procedural calls.

### Phase 4: Layout Engine
- Box model: `Rect`, `Point`, `Size`, `Padding`, `Margin`.
- Directional layout: `Horizontal` and `Vertical` stacks.
- Sizing modes: `Fixed`, `Auto` (content-driven), `Percent`, `Flex`.
- Gap spacing, alignment (`Start`, `Center`, `End`, `Stretch`).
- Clipping rects and scroll view offsets.
*Exit Criteria:* Nested containers arrange their children automatically without manual `SetCursorPos` coordinates.

### Phase 5: Core Widgets
- Implement foundation components:
  - `Panel` (Background container)
  - `Text` (Typography with wrap and auto-size)
  - `Image` (Texture rendering with aspect ratio)
  - `Button` & `IconButton` (Interactive with click callback)
  - `List` & `ScrollView` (Scrollable item containers)
  - `Checkbox`, `Slider`, `ProgressBar`
- Modder-extensible widget base class (`class MyWidget : public PerfUI::Widget`).
*Exit Criteria:* Core widgets can be combined in test sandbox to construct rich static layouts.

### Phase 6: Styling & Themes
- Visual properties: colors, borders, border radii, shadows, fonts.
- State-driven styling: `Normal`, `Hovered`, `Pressed`, `Focused`, `Disabled`.
- Theme manager with `PerfUIDefault` theme.
*Exit Criteria:* Changing theme or widget state automatically updates visuals smoothly without modifying widget code.

### Phase 7: Input & Controller Navigation
- Gamepad-first focus manager.
- 2D spatial navigation algorithm (nearest neighbor in Up/Down/Left/Right directions).
- Action bindings for XInput: D-Pad, Left Stick, `A` (Submit), `B` (Cancel), Bumpers (Tabs).
- Keyboard parity (Arrow keys, Enter, Esc, Tab).
- Modal focus isolation.
*Exit Criteria:* Full navigation through complex forms and menus solely using an Xbox/PlayStation controller.

### Phase 8: Animation System
- Property tweener: numeric interpolation over time.
- Easing library (Linear, OutCubic, InOutQuad, OutBack, OutBounce, etc.).
- Animated transitions for widget states (e.g. hover glow, panel slide-in, fade-in).
*Exit Criteria:* Smooth opening and closing animations of windows and buttons driven by `deltaTime`.

### Phase 9: Rendering Refinements
- High-quality rounded corners and multi-stop gradients.
- Box shadows with soft blur.
- Font atlas integration and glyph caching.
- Vector icons / SVG or icon font support.
*Exit Criteria:* UI visuals match modern AAA game standards.

### Phase 10: Skyrim Integration Layer
- Pure C++ wrapper layer `PerfUI::Skyrim` isolated from UI Core.
- Hook into Skyrim menu lifecycle (`MenuOpen`, `MenuClose`).
- Game event listeners (`QuestStatusChanged`, etc.).
- Safe game-thread to render-thread data marshalling.
- Quest API (`Skyrim::Quests::active()`, objectives, stages).
*Exit Criteria:* Reading Skyrim quests from the engine into plain C++ data structures safely on the render thread.

### Phase 11: The Journal Prototype
- Full implementation of Skyrim Quest Journal:
  - Left sidebar: Categories (Main, Side, Misc, Completed).
  - Center: Quest list with selection state and controller scroll.
  - Right: Quest details, stage summary, animated objective checkboxes.
- 100% built on PerfUI public API (zero direct ImGui or Skyrim engine calls in Journal code).
*Exit Criteria:* Fully functional, beautiful, controller-navigable Journal in Skyrim.

### Phase 12: Stress & Performance Testing
- 100+ active widgets in view.
- Frame time profiling (< 0.2ms target for UI update and render).
- Stress test: rapid window toggle, 1000 alt-tabs, resolution switches, memory leak check.
*Exit Criteria:* Zero memory leaks, zero D3D11 device loss crashes, zero frame drops.

### Phase 13: Main Menu Integration (COMPLETED)
- [x] Full-screen Main Menu prototype (`MainMenuWindow`: Continue, New Game, Load, Settings, Credits, Quit).
- [x] Hotkey toggle in sandbox (`F10`) and seamless switching between Quest Journal and Main Menu.
- [x] Integration with Skyrim D3D11 rendering hook.
*Exit Criteria:* PerfUI successfully powers full-screen primary game menus.

### Phase 14: Developer API & Examples (COMPLETED)
- [x] Clear modder documentation and tutorials (`docs/MODDER_GUIDE.md`).
- [x] Minimal example project (`examples/modder_custom_hud/`: `CustomHealthBar.h`, `ModEntry.cpp`, `CMakeLists.txt`).
- [x] Dynamic SDK API loader (`PerfUI/PerfUIApi.h`) and `RequestPluginAPI` export in `PerfUI.dll`.
- [x] Interactive sandbox integration (`F8` toggle, `H` heal, `J` damage simulator).
*Exit Criteria:* A third-party modder can create a custom window or HUD in under 30 lines of code.

### Phase 15: Backend Independence Test (COMPLETED)
- [x] Create headless mock rendering backend (`src/backends/mock/MockRenderBackend.h`).
- [x] Create automated test runner (`tests/BackendIndependenceTest.cpp` -> `PerfUI_Test_Independence.exe`).
- [x] Verified 100% Core build and execution completely detached from ImGui and DirectX (16/16 tests passed).
*Exit Criteria:* Proof of complete backend neutrality.

### Phase 16: PerfUI SDK Packaging
- Modular distribution: `PerfUI_Core.lib`, `PerfUI_Backend_ImGui.lib`, `PerfUI_Skyrim.lib`, public headers.
*Exit Criteria:* Ready-to-consume SDK release for the Skyrim modding community.

### Phase 17: Long-Term Innovations
- In-game UI Inspector (live element picker and property editor).
- Live reload of styles and layouts.
- Real-time in-game UI profiler (draw calls, layout time, memory).
