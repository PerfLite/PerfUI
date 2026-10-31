# MapCanvas — Map & Radar Widget

`PerfUI::MapCanvas` (`include/PerfUI/MapCanvas.h`, `src/widgets/MapCanvas.cpp`) is a flexible map widget supporting two primary modes: a circular radar with the player centered (`MapMode::Radar`) or a pannable and zoomable schematic world map (`MapMode::World`). The widget remains completely decoupled from Skyrim internals: all data rendered is provided via the `IMapDataSource` interface.
Mod code populates this data source from game memory, keeping `PerfUI` portable and backend-agnostic.

A complete live sample is located in `examples/map_demo/` (in the test sandbox press `[F9]`, toggle mode with `[M]`).

---

## 1. Data Model

```cpp
enum class MapMarkerKind { Quest, Location, Enemy, Custom };

struct MapMarker {
    std::uint64_t id{ 0 };                 // Unique marker identifier
    std::string   label;                   // Label text; empty renders distance only
    MapMarkerKind kind{ MapMarkerKind::Custom };
    float x{ 0.0f };                       // Meters, +X = East
    float y{ 0.0f };                       // Meters, +Y = North
    bool  active{ false };                 // Currently selected quest objective
    bool  discovered{ true };              // false: translucent marker
    bool  sameSpace{ true };               // false: hidden from map, rendered as status badge
    Color color{ Color::Transparent() };   // alpha == 0: defaults to theme token for kind
};

struct MapPlayerState {
    float x{ 0.0f };
    float y{ 0.0f };
    float headingDeg{ 0.0f };              // 0 = North, clockwise rotation
};

class IMapDataSource {
public:
    virtual MapPlayerState player() const = 0;
    virtual std::span<const MapMarker> markers() const = 0;
    virtual std::uint64_t revision() const = 0;   // Incremented whenever data changes
};
```

Default theme colors by `kind` (when marker color has zero alpha):
- `Quest`: `theme.colors.textAccent`
- `Location`: light blue `{126, 200, 227}`
- `Enemy`: danger red `{214, 86, 78}`
- `Custom`: `theme.colors.borderFocus`

Shapes: `Quest` is a diamond, `Location` is a circle, `Enemy` is a triangle, `Custom` is a compact circle. Active objectives are enlarged, featuring a pulsating ring and a dotted navigation line from the player.

### Built-in In-Memory Source

`VectorMapDataSource` is provided for tests, demos, and single-threaded scenarios:

```cpp
PerfUI::VectorMapDataSource src;
src.setPlayer(0.0f, 0.0f, 90.0f);          // x, y, heading in degrees
src.setMarkers({ markerA, markerB });      // automatically increments revision
map.setDataSource(&src);                   // widget does not own data source
```

### Game Integration (Skyrim Mod)

The data source lives in the mod; the widget lives in PerfUI. Rule: `markers()` must return a snapshot that remains valid for the duration of the current frame render pass.

```cpp
class SkyrimMapDataSource final : public PerfUI::IMapDataSource {
public:
    PerfUI::MapPlayerState player() const override { return m_player; }
    std::span<const PerfUI::MapMarker> markers() const override { return m_markers; }
    std::uint64_t revision() const override { return m_revision; }

    // Invoked from Skyrim gameplay thread, dispatches snapshot into UI queue
    void pushFromGameThread(PerfUI::MapPlayerState p, std::vector<PerfUI::MapMarker> m) {
        PerfUI::Client::RunOnUIThread([this, p, m = std::move(m)]() mutable {
            m_player = p;
            m_markers = std::move(m);
            ++m_revision;
        });
    }

private:
    PerfUI::MapPlayerState m_player;
    std::vector<PerfUI::MapMarker> m_markers;      // Accessed solely on UI thread
    std::uint64_t m_revision{ 1 };
};
```

---

## 2. Coordinate System & Trigonometry

- **X axis increases East, Y axis increases North** (unlike screen Y which points downwards).
- **Screen Y increases downwards.** This accounts for minus signs in projection formulas.
- **`headingDeg`: 0 = North, increases clockwise.** 90 = East, 180 = South, 270 = West.
- **Target azimuth** is calculated clockwise from North: `atan2(dx, dy)`.

```
units → meters:  m = units * 0.01428                 (Skyrim scale: 1 m ≈ 70 units)
bearing(P) [deg] β = deg(atan2(P.x - p.x, P.y - p.y)) → normalized to [0, 360)
player heading:  h = headingDeg
relative angle:  θ = β − h

radar, rotateWithPlayer = true:   screenX = cx + R · sin θ
                                  screenY = cy − R · cos θ
radar, rotateWithPlayer = false:  θ = β (North locked upwards)
world (camera c, m/px = k):       screenX = cx + (P.x − c.x) / k
                                  screenY = cy − (P.y − c.y) / k
```

When `rotateWithPlayer = true`, player heading is subtracted from the azimuth, ensuring the player arrow always points straight up while the map rotates underneath.

---

## 3. Modes

| Mode | Description |
| :--- | :--- |
| `MapMode::Radar` | Circular disc: player centered, scale determined by radar radius |
| `MapMode::World` | Rectangular world map: pan, zoom, grid, clustering, bounds clamping |

### Radar Mode

A circular disc sized `min(width, height)/2 − 3` px. Range rings at 1/3, 2/3, and 3/3 of the radius with distance labels in meters. Objectives exceeding `radarRange` are clamped to the border with directional arrows and exact distance tags.

### World Mode

Rectangular viewport (`viewRect`, 2 px inset). North locked upwards:
- **Camera:** Centered in world meters (`cameraCenter`) with scale defined in `metersPerPixel`. Configurable via `zoomLimits(min, max)` (default 0.25 … 200).
- **Player Following:** (`followPlayer`, enabled by default). Manual panning or `centerOn` disables following; pressing the recenter target icon in the top-right corner restores camera lock.
- **Frustum Culling:** Off-screen markers are culled. Active objectives are an exception: clamped to the viewport border as directional arrows.
- **Adaptive Grid:** Grid step dynamically selects from {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000} meters to maintain ≥64 px line separation with coordinate axis labels.
- **Marker Clustering:** Markers collapsing within `clusterPixelSize` (default 26 px) are rendered as badge icons displaying item count. Active objectives never cluster. Tapping a cluster centers the camera and zooms in (`zoomBy(0.5)`). Set `clusterPixelSize(0)` to disable clustering.
- **Navigation Line:** Direct line connecting player to active objective with distance badge.
- **Bounding Box Clamping:** `clampToBounds(minX, minY, maxX, maxY)` prevents camera from panning outside world boundaries.

---

## 4. API Reference

| Method | Default | Description |
| :--- | :--- | :--- |
| `setDataSource(IMapDataSource*)` | `nullptr` | Marker data source (safe to pass `nullptr` for empty map) |
| `mode(MapMode)` | `Radar` | Operating mode (`Radar` or `World`) |
| `radarRange(float meters)` | `300` | Radar range in meters (`<= 0` disables projection) |
| `rotateWithPlayer(bool)` | `true` | Radar only: rotate map with player heading vs lock North |
| `showGrid(bool)` | `false` | Cardinal ray markers in Radar; adaptive grid in World |
| `showDistances(bool)` | `true` | Display distance labels for rings, markers, and path lines |
| `onMarkerClicked(cb)` | empty | Callback invoked when marker or target arrow is clicked |
| `metersPerPixel(float)` | `2` | World only: current camera zoom level |
| `zoomLimits(min, max)` | `0.25 / 200` | World only: minimum and maximum allowed zoom |
| `cameraCenter(Point meters)` | `(0,0)` | World only: manual camera position |
| `followPlayer(bool)` | `true` | World only: camera tracks player movement |
| `clusterPixelSize(float px)` | `26` | World only: cluster cell size in pixels (`0` = disabled) |
| `clampToBounds(minX, minY, maxX, maxY)` | — | World only: clamp camera center within boundary |
| `clearBounds()` | — | Remove bounding constraints |
| `onBackground(cb)` | empty | Back navigation callback (`VK_BACK` / Gamepad B button) |

---

## 5. Input Controls

| Event | Radar | World |
| :--- | :--- | :--- |
| Left Click on marker / objective arrow | Invokes `onMarkerClicked` | Invokes `onMarkerClicked` |
| Left Click on cluster badge | — | Centers and zooms into cluster |
| Left Click on recenter icon | — | Centers on player and resumes following |
| Left Drag | Ignored | Pans map camera |
| Mouse Wheel | Ignored | Zooms map around mouse cursor |
| Gamepad Right Stick / Arrow Keys | Ignored | Pans camera (48 px step) |
| Gamepad B / Backspace (`0x08`) | Ignored | Invokes `onBackground` callback |

---

## 6. Performance & Caching

Projection calculations are skipped unless cache keys are invalidated:
- Marker snapshot copying occurs only when the data source `revision()` changes.
- Camera moves (pan/zoom) recompute projected screen positions without recopying data collections.
- Testing benchmark: 1,000 markers across 100 frames takes ~4–5 ms per frame during active recalculation (`PerfUI_Test_MapCanvas`), and zero recompute cost when stationary.
