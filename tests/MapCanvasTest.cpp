// Headless tests for MapCanvas on MockRenderBackend: no ImGui, D3D, or Skyrim.
// Verifies <world -> screen> transform, range projection, out-of-range arrows,
// sameSpace, geometry cache by revision, click handling, edge cases (phase 1), and camera
// World mode: cursor zoom, bounds clamping, clustering, grid step, and input (phase 2).

#include "PerfUI/MapCanvas.h"

#include <MockRenderBackend.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

#define TEST_ASSERT(cond, msg)                                                             \
    do {                                                                                   \
        if (cond) {                                                                        \
            ++g_passed;                                                                    \
        } else {                                                                           \
            ++g_failed;                                                                    \
            std::printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);                  \
        }                                                                                  \
    } while (0)

using namespace PerfUI;

bool near(float a, float b, float eps = 0.5f) {
    return std::fabs(a - b) <= eps;
}

using CommandType = MockRenderBackend::DrawCommand::Type;

MockRenderBackend::DrawCommand findLast(const MockRenderBackend& mock, CommandType type) {
    const auto& cmds = mock.commands();
    for (auto it = cmds.rbegin(); it != cmds.rend(); ++it) {
        if (it->type == type) return *it;
    }
    return MockRenderBackend::DrawCommand{ type, Rect{}, Color{}, "", 0.0f };
}

std::size_t countOf(const MockRenderBackend& mock, CommandType type) {
    std::size_t n = 0;
    for (const auto& cmd : mock.commands()) {
        if (cmd.type == type) ++n;
    }
    return n;
}

bool hasTextContaining(const MockRenderBackend& mock, const std::string& needle) {
    for (const auto& cmd : mock.commands()) {
        if (cmd.type == CommandType::Text && cmd.text.find(needle) != std::string::npos) return true;
    }
    return false;
}

// Small radius circle (marker shape, not range ring or border) centered at point.
bool hasMarkerCircleAt(const MockRenderBackend& mock, float x, float y, float maxRadius) {
    for (const auto& cmd : mock.commands()) {
        if (cmd.type != CommandType::Circle) continue;
        const float cx = cmd.bounds.x + cmd.radius;
        const float cy = cmd.bounds.y + cmd.radius;
        if (cmd.radius <= maxRadius && near(cx, x, 1.0f) && near(cy, y, 1.0f)) return true;
    }
    return false;
}

// Triangle with vertex (points[0]) at given point - used for boundary arrows.
bool hasTriangleTipAt(const MockRenderBackend& mock, float x, float y, float eps = 3.0f) {
    for (const auto& cmd : mock.commands()) {
        if (cmd.type != CommandType::Triangle || cmd.points.empty()) continue;
        if (near(cmd.points[0].x, x, eps) && near(cmd.points[0].y, y, eps)) return true;
    }
    return false;
}

Point shapeCenter(const MockRenderBackend::DrawCommand& cmd) {
    Point sum{};
    for (const auto& p : cmd.points) {
        sum.x += p.x;
        sum.y += p.y;
    }
    const float n = cmd.points.empty() ? 1.0f : static_cast<float>(cmd.points.size());
    return { sum.x / n, sum.y / n };
}

MapMarker makeMarker(std::uint64_t id, const std::string& label, float x, float y,
                     MapMarkerKind kind = MapMarkerKind::Custom) {
    MapMarker marker;
    marker.id = id;
    marker.label = label;
    marker.kind = kind;
    marker.x = x;
    marker.y = y;
    return marker;
}

void renderOnce(MapCanvas& canvas, MockRenderBackend& mock) {
    mock.beginFrame();
    canvas.render(mock);
    mock.endFrame();
}

// 200x200 square => center (100,100), radius 97.
MapCanvas makeCanvas(VectorMapDataSource& source, float range) {
    MapCanvas canvas;
    canvas.setDataSource(&source).radarRange(range).showDistances(false);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    return canvas;
}
// 1. World -> screen transform: North above center at heading = 0 and to the left at heading = 90.
void testWorldToScreen() {
    std::printf("testWorldToScreen\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "North", 0.0f, 50.0f, MapMarkerKind::Quest) });

    MapCanvas canvas = makeCanvas(source, 100.0f);

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    // Target at 50 m with 100 m range => 48.5 px up from center
    const Point center = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(center.x, 100.0f), "north target stays centered horizontally at heading 0");
    TEST_ASSERT(near(center.y, 51.5f), "north target is above the player at heading 0");

    source.setPlayer(0.0f, 0.0f, 90.0f);  // looking East - North moves to the left
    renderOnce(canvas, mock);

    const Point rotated = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(rotated.x, 51.5f), "north target moves left at heading 90");
    TEST_ASSERT(near(rotated.y, 100.0f), "north target is vertically centered at heading 90");

    // East target at heading = 90 moves up: map rotates with the player.
    source.setMarkers({ makeMarker(2, "East", 50.0f, 0.0f, MapMarkerKind::Quest) });
    renderOnce(canvas, mock);

    const Point east = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(east.x, 100.0f), "east target is centered horizontally when facing east");
    TEST_ASSERT(near(east.y, 51.5f), "east target is ahead of the player when facing east");

    // rotateWithPlayer = false: North is always up, regardless of heading.
    canvas.rotateWithPlayer(false);
    source.setMarkers({ makeMarker(3, "North", 0.0f, 50.0f, MapMarkerKind::Quest) });
    renderOnce(canvas, mock);

    const Point northUp = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(northUp.x, 100.0f) && near(northUp.y, 51.5f), "north stays up when not rotating with player");
}

// 2. Marker within range is rendered proportionally to meters.
void testProportionalDistance() {
    std::printf("testProportionalDistance\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);

    MapCanvas canvas = makeCanvas(source, 200.0f);

    MockRenderBackend mock;
    float previousOffset = -1.0f;
    bool monotonic = true;
    bool linear = true;

    for (const float meters : { 25.0f, 50.0f, 100.0f, 150.0f }) {
        source.setMarkers({ makeMarker(1, "Target", 0.0f, meters, MapMarkerKind::Quest) });
        renderOnce(canvas, mock);

        const float offset = 100.0f - shapeCenter(findLast(mock, CommandType::Quad)).y;
        if (previousOffset >= 0.0f && offset <= previousOffset) monotonic = false;
        previousOffset = offset;

        if (!near(offset, meters / 200.0f * 97.0f, 1.0f)) linear = false;
    }

    TEST_ASSERT(monotonic, "further targets are drawn further from the center");
    TEST_ASSERT(linear, "screen offset is proportional to meters");
}

// 3. Marker out of range: boundary arrow and distance label.
void testFarTargetEdgeArrow() {
    std::printf("testFarTargetEdgeArrow\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "Far Keep", 0.0f, 420.0f, MapMarkerKind::Location) });

    MapCanvas canvas;
    canvas.setDataSource(&source).radarRange(300.0f).showDistances(true);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    // First triangle is player arrow, second is distant target arrow.
    TEST_ASSERT(countOf(mock, CommandType::Triangle) >= 2, "far target draws an edge arrow");
    TEST_ASSERT(hasTextContaining(mock, "420"), "far target label shows the distance");

    bool arrowAtEdge = false;
    for (const auto& cmd : mock.commands()) {
        if (cmd.type != CommandType::Triangle || cmd.points.size() != 3) continue;
        const Point tip = cmd.points[0];
        const float fromCenter = std::hypotf(tip.x - 100.0f, tip.y - 100.0f);
        if (near(fromCenter, 95.0f, 2.0f) && tip.y < 100.0f) arrowAtEdge = true;
    }
    TEST_ASSERT(arrowAtEdge, "edge arrow tip sits on the radar border toward the target");

    // Approaching target converts it from boundary arrow to normal point.
    source.setMarkers({ makeMarker(1, "Far Keep", 0.0f, 100.0f, MapMarkerKind::Location) });
    renderOnce(canvas, mock);

    TEST_ASSERT(canvas.projectedCount() == 1, "inside-range target is still projected");
    TEST_ASSERT(countOf(mock, CommandType::Triangle) == 1, "inside-range target no longer draws an edge arrow");
    TEST_ASSERT(hasMarkerCircleAt(mock, 100.0f, 67.7f, 12.0f),
                "inside-range target is drawn at its proportional spot (100 m out of 300 m radius)");
}

// 4. Edge cases: zero radius, empty list, nullptr source, destroyed source.
void testDegenerateInputs() {
    std::printf("testDegenerateInputs\n");

    MockRenderBackend mock;

    MapCanvas canvas;
    canvas.setBounds(Rect{ 0.0f, 0.0f, 120.0f, 120.0f });

    renderOnce(canvas, mock);
    TEST_ASSERT(mock.clipStackDepth() == 0, "null data source leaves a balanced clip stack");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);

    canvas.setDataSource(&source).radarRange(0.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.projectedCount() == 0, "zero radar range projects nothing");
    TEST_ASSERT(mock.clipStackDepth() == 0, "zero radar range leaves a balanced clip stack");

    canvas.radarRange(100.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.projectedCount() == 0, "empty marker list projects nothing");

    canvas.setBounds(Rect{ 0.0f, 0.0f, 2.0f, 2.0f });
    renderOnce(canvas, mock);
    TEST_ASSERT(mock.clipStackDepth() == 0, "tiny widget bounds are handled");

    source.setMarkers({ makeMarker(1, "Keep", 0.0f, 10.0f), makeMarker(2, "Camp", 10.0f, 0.0f) });
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.projectedCount() == 2, "markers project while the source is set");

    canvas.setDataSource(nullptr);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.projectedCount() == 0, "source pointer cleared before destruction");

    {
        VectorMapDataSource scoped;
        scoped.setPlayer(0.0f, 0.0f, 0.0f);
        scoped.setMarkers({ makeMarker(3, "Scoped", 0.0f, 20.0f) });
        canvas.setDataSource(&scoped);
        renderOnce(canvas, mock);
        TEST_ASSERT(canvas.projectedCount() == 1, "scoped source is used while alive");
    }

    canvas.setDataSource(nullptr);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.projectedCount() == 0, "destroyed source is not read");
    TEST_ASSERT(mock.clipStackDepth() == 0, "all cases leave clip stack empty");
}

// 5. sameSpace = false: target not on map, shows 'in another location' badge.
void testOffSpaceMarker() {
    std::printf("testOffSpaceMarker\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);

    MapMarker outside = makeMarker(1, "Coldbrook", 0.0f, 40.0f, MapMarkerKind::Quest);
    outside.active = true;
    outside.sameSpace = false;
    source.setMarkers({ outside, makeMarker(2, "Near", 0.0f, 60.0f) });

    MapCanvas canvas;
    canvas.setDataSource(&source).radarRange(100.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    TEST_ASSERT(canvas.projectedCount() == 1, "off-space marker is not projected");
    TEST_ASSERT(hasTextContaining(mock, "another location"), "off-space marker shows a place badge");
}

// 6. Unchanged revision - geometry is not recomputed.
void testRevisionCache() {
    std::printf("testRevisionCache\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "Keep", 0.0f, 30.0f) });

    MapCanvas canvas;
    canvas.setDataSource(&source).radarRange(100.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);
    const std::size_t afterFirst = canvas.recomputeCount();

    renderOnce(canvas, mock);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.recomputeCount() == afterFirst, "unchanged revision does not recompute geometry");

    source.setPlayer(0.0f, 5.0f, 0.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.recomputeCount() == afterFirst + 1, "moved player recomputes once");

    canvas.radarRange(150.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.recomputeCount() == afterFirst + 2, "changed radar range recomputes once");

    canvas.showGrid(true);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.recomputeCount() == afterFirst + 3, "toggled grid recomputes once");
}

// 7. Marker click.
void testMarkerClick() {
    std::printf("testMarkerClick\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(7, "Click Me", 0.0f, 50.0f, MapMarkerKind::Quest) });

    MapCanvas canvas;
    canvas.setDataSource(&source).radarRange(100.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    std::uint64_t clicked = 0;
    canvas.onMarkerClicked([&](const MapMarker& marker) { clicked = marker.id; });

    TEST_ASSERT(!canvas.onPointerUp(Point{ 10.0f, 190.0f }), "click on empty area is not consumed");
    TEST_ASSERT(clicked == 0, "empty-area click fires no callback");
    TEST_ASSERT(canvas.onPointerUp(Point{ 100.0f, 51.5f }), "click on marker is consumed");
    TEST_ASSERT(clicked == 7, "clicked marker id is delivered");

    // Distant target: boundary arrow is clickable.
    source.setMarkers({ makeMarker(8, "Far", 0.0f, 900.0f, MapMarkerKind::Quest) });
    renderOnce(canvas, mock);
    clicked = 0;
    TEST_ASSERT(canvas.onPointerUp(Point{ 100.0f, 8.0f }), "click on edge arrow is consumed");
    TEST_ASSERT(clicked == 8, "edge arrow delivers its marker");
}

// 8. Benchmark: 1000 markers, single frame render time on mock. No hard threshold.
void benchmarkThousandMarkers() {
    std::printf("benchmarkThousandMarkers\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);

    std::vector<MapMarker> markers;
    markers.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        markers.push_back(makeMarker(static_cast<std::uint64_t>(i), "M" + std::to_string(i),
                                     static_cast<float>(i % 97) * 4.0f, static_cast<float>(i / 97) * 4.0f));
    }
    source.setMarkers(std::move(markers));

    MapCanvas canvas;
    canvas.setDataSource(&source).radarRange(300.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 400.0f, 400.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);  // warmup: geometry recalculation

    const auto begin = std::chrono::steady_clock::now();
    for (int frame = 0; frame < 100; ++frame) {
        renderOnce(canvas, mock);
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - begin);

    std::printf("  100 frames with 1000 markers: %.1f us per frame, %zu draw calls per frame\n",
                static_cast<double>(elapsed.count()) / 100.0, mock.drawCallCount());

    TEST_ASSERT(canvas.projectedCount() == 1000, "all 1000 markers projected");
    TEST_ASSERT(canvas.recomputeCount() == 1, "steady data does not recompute per frame");
}

}  // namespace

// 9. World: camera projects world meters, North is up, target offscreen -> boundary arrow.
void testWorldCameraProjection() {
    std::printf("testWorldCameraProjection\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({
        makeMarker(1, "East", 40.0f, 0.0f, MapMarkerKind::Location),
        makeMarker(2, "North", 0.0f, 40.0f, MapMarkerKind::Location),
    });

    MapCanvas canvas;
    canvas.setDataSource(&source)
          .mode(MapMode::World)
          .metersPerPixel(1.0f)
          .showDistances(false)
          .clusterPixelSize(0.0f)
          .followPlayer(false);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    canvas.cameraCenter(Point{ 0.0f, 0.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    // 40 m at 1 m/px => 40 px from center (100,100): East is right, North is up.
    TEST_ASSERT(hasMarkerCircleAt(mock, 140.0f, 100.0f, 12.0f), "world east marker projects right of center");
    TEST_ASSERT(hasMarkerCircleAt(mock, 100.0f, 60.0f, 12.0f), "world north marker projects above center");
    TEST_ASSERT(canvas.projectedCount() == 2, "both in-view markers are projected");
    TEST_ASSERT(canvas.clusterCount() == 0, "clustering off means no clusters");

    // Offscreen target remains visible as a boundary arrow.
    MapMarker far = makeMarker(3, "Far", 1000.0f, 0.0f, MapMarkerKind::Quest);
    far.active = true;
    source.setMarkers({ far, makeMarker(4, "Hidden", 800.0f, 0.0f) });
    renderOnce(canvas, mock);

    TEST_ASSERT(canvas.projectedCount() == 1, "off-view non-active markers are culled");
    // 196 px window with offset 2 => right edge at x = 198, East target => center y = 100.
    TEST_ASSERT(hasTriangleTipAt(mock, 198.0f, 100.0f), "off-view active target draws an arrow on the border");
    TEST_ASSERT(canvas.clusterCount() == 0, "single visible marker cannot cluster");

    // Camera moved away from player: both player and target offscreen. Arrow must clamp
    // to window boundary rather than fly off to coordinates "beyond horizon".
    canvas.cameraCenter(Point{ 500.0f, 0.0f });
    renderOnce(canvas, mock);
    TEST_ASSERT(hasTriangleTipAt(mock, 2.0f, 100.0f), "arrow enters the view from the opposite border");
}

// 9b. World: line "player -> active target" with distance in the middle.
void testWorldActiveLink() {
    std::printf("testWorldActiveLink\n");

    MapMarker target = makeMarker(1, "Target", 0.0f, 120.0f, MapMarkerKind::Quest);
    target.active = true;

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ target });

    MapCanvas canvas;
    canvas.setDataSource(&source).mode(MapMode::World).showDistances(true).clusterPixelSize(0.0f)
          .showGrid(false).metersPerPixel(1.0f).followPlayer(false);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    canvas.cameraCenter(Point{ 0.0f, 60.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    // Player (0,0) -> screen (100,160), target (0,120) -> screen (100,40): both inside window.
    TEST_ASSERT(canvas.projectedCount() == 1, "active target is projected");
    const auto link = findLast(mock, CommandType::Line);
    TEST_ASSERT(near(link.bounds.x, 100.0f, 1.0f) && near(link.bounds.y, 160.0f, 1.0f),
                "link starts at the player");
    TEST_ASSERT(near(link.bounds.x + link.bounds.width, 100.0f, 1.0f) &&
                near(link.bounds.y + link.bounds.height, 40.0f, 1.0f),
                "link ends at the active target");
    TEST_ASSERT(hasTextContaining(mock, "120"), "link label shows the distance in meters");
}

// 10. Cursor zoom: point under cursor stays invariant.
void testZoomAroundCursor() {
    std::printf("testZoomAroundCursor\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "Anchor", 50.0f, 0.0f, MapMarkerKind::Location) });

    MapCanvas canvas;
    canvas.setDataSource(&source).mode(MapMode::World).showDistances(false).clusterPixelSize(0.0f)
          .followPlayer(false).metersPerPixel(1.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    canvas.cameraCenter(Point{ 0.0f, 0.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    // 50 m at 1 m/px => x = 150. Cursor at (150,100) - directly over marker.
    TEST_ASSERT(hasMarkerCircleAt(mock, 150.0f, 100.0f, 12.0f), "marker sits under the cursor before zoom");

    const Point cursor{ 150.0f, 100.0f };
    canvas.zoomBy(0.5f, &cursor);            // 1 m/px -> 0.5 m/px, zoom in towards cursor
    renderOnce(canvas, mock);

    TEST_ASSERT(canvas.metersPerPixel() == 0.5f, "zoomBy scales meters-per-pixel");
    TEST_ASSERT(hasMarkerCircleAt(mock, 150.0f, 100.0f, 12.0f), "point under cursor stays put after zoom");

    // Zoom boundary values prevent camera from entering invalid scale.
    canvas.zoomBy(1000.0f);
    TEST_ASSERT(canvas.metersPerPixel() == 200.0f, "zoom-in limit clamps meters-per-pixel");
    canvas.zoomBy(0.0001f);
    TEST_ASSERT(canvas.metersPerPixel() == 0.25f, "zoom-out limit clamps meters-per-pixel");
}

// 11. Camera panning and bounding area clamping.
void testPanAndBoundsClamp() {
    std::printf("testPanAndBoundsClamp\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "Here", 10.0f, 10.0f) });

    MapCanvas canvas;
    canvas.setDataSource(&source).mode(MapMode::World).clusterPixelSize(0.0f).followPlayer(false)
          .metersPerPixel(1.0f)
          .clampToBounds(-500.0f, -500.0f, 500.0f, 500.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });

    // 196 px window at 1 m/px => center can move up to 98 m from area edge.
    // Dragging map with cursor: down on screen means camera moves North.
    canvas.panPixels(Point{ 100000.0f, 0.0f });
    TEST_ASSERT(near(canvas.cameraCenter().x, -402.0f, 1.0f), "pan west stops at the bound");
    canvas.panPixels(Point{ -100000.0f, 0.0f });
    TEST_ASSERT(near(canvas.cameraCenter().x, 402.0f, 1.0f), "pan east stops at the bound");
    canvas.panPixels(Point{ 0.0f, 100000.0f });
    TEST_ASSERT(near(canvas.cameraCenter().y, 402.0f, 1.0f), "dragging down stops at the northern bound");
    canvas.panPixels(Point{ 0.0f, -100000.0f });
    TEST_ASSERT(near(canvas.cameraCenter().y, -402.0f, 1.0f), "dragging up stops at the southern bound");

    // Area smaller than window - camera keeps it centered rather than clamping to edge.
    MapCanvas tiny;
    tiny.setDataSource(&source).mode(MapMode::World).followPlayer(false).metersPerPixel(10.0f)
        .clampToBounds(0.0f, 0.0f, 10.0f, 10.0f);
    tiny.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    tiny.cameraCenter(Point{ 500.0f, -500.0f });
    TEST_ASSERT(near(tiny.cameraCenter().x, 5.0f, 0.01f), "small map centers horizontally");
    TEST_ASSERT(near(tiny.cameraCenter().y, 5.0f, 0.01f), "small map centers vertically");

    // Invalid area (max <= min) does not clamp anything.
    MapCanvas broken;
    broken.setDataSource(&source).mode(MapMode::World).followPlayer(false)
          .clampToBounds(100.0f, 100.0f, 0.0f, 0.0f);
    broken.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    broken.panMeters(Point{ 1000.0f, 0.0f });
    TEST_ASSERT(!broken.hasBounds(), "degenerate bounds are rejected");
    TEST_ASSERT(near(broken.cameraCenter().x, -1000.0f, 0.5f), "rejected bounds do not clamp");

    // centerOn disables auto-follow, otherwise camera would return to player.
    MapCanvas follow;
    follow.setDataSource(&source).mode(MapMode::World).clusterPixelSize(0.0f).metersPerPixel(1.0f);
    follow.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    follow.centerOn(Point{ 300.0f, 0.0f });
    MockRenderBackend mock;
    renderOnce(follow, mock);
    TEST_ASSERT(!follow.isFollowingPlayer(), "centerOn turns off follow-player");
    TEST_ASSERT(near(follow.cameraCenter().x, 300.0f, 0.5f), "camera stays where the user aimed");

    follow.centerOnPlayer();
    renderOnce(follow, mock);
    TEST_ASSERT(near(follow.cameraCenter().x, 0.0f, 0.5f), "centerOnPlayer returns to the player");
}

// 12. Clustering nearby markers at distance.
void testClustering() {
    std::printf("testClustering\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({
        makeMarker(1, "A", 10.0f, 10.0f, MapMarkerKind::Location),
        makeMarker(2, "B", 12.0f, 10.0f, MapMarkerKind::Location),
        makeMarker(3, "C", 14.0f, 11.0f, MapMarkerKind::Location),
        makeMarker(4, "Distant", 60.0f, -40.0f, MapMarkerKind::Enemy),
    });

    MapCanvas canvas;
    canvas.setDataSource(&source).mode(MapMode::World).showDistances(false).metersPerPixel(1.0f)
          .followPlayer(false).clusterPixelSize(26.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    TEST_ASSERT(canvas.clusterCount() == 1, "three nearby markers form one cluster");
    TEST_ASSERT(canvas.projectedCount() == 1, "the lonely marker stays a marker");
    TEST_ASSERT(!hasTextContaining(mock, "A"), "clustered labels are not drawn");
    TEST_ASSERT(hasTextContaining(mock, "3"), "cluster badge shows the count");

    // Clustering disabled - all four markers rendered separately.
    canvas.clusterPixelSize(0.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.clusterCount() == 0, "zero cluster size disables clustering");
    TEST_ASSERT(canvas.projectedCount() == 4, "all markers project when clustering is off");

    // Active target from cluster is not hidden.
    MapMarker active = makeMarker(2, "B", 12.0f, 10.0f, MapMarkerKind::Quest);
    active.active = true;
    auto markers = source.markers();
    std::vector<MapMarker> next;
    next.push_back(makeMarker(1, "A", 10.0f, 10.0f, MapMarkerKind::Location));
    next.push_back(active);
    next.push_back(makeMarker(3, "C", 14.0f, 11.0f, MapMarkerKind::Location));
    source.setMarkers(std::move(next));
    canvas.clusterPixelSize(26.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.clusterCount() == 1, "remaining pair still clusters");
    TEST_ASSERT(canvas.projectedCount() == 1, "active target is never hidden in a cluster");

    // Clicking cluster focuses on it and zooms in.
    // Centroid of triplet: x = (110+112+114)/3 = 112, y = (90+90+89)/3 ~= 89.7.
    const Point clusterBefore = canvas.cameraCenter();
    TEST_ASSERT(canvas.onPointerUp(Point{ 112.0f, 89.7f }), "cluster click is consumed");
    TEST_ASSERT(canvas.metersPerPixel() < 1.0f, "cluster click zooms in");
    TEST_ASSERT(canvas.cameraCenter().x != clusterBefore.x || canvas.cameraCenter().y != clusterBefore.y,
                "cluster click moves the camera to the cluster");

    // Neighbors on opposite sides of cell boundary (20 px apart with 26 px cell) should
    // also group: otherwise icon appears and disappears on single-pixel camera shifts.
    canvas.metersPerPixel(1.0f);
    canvas.cameraCenter(Point{ 0.0f, 0.0f });
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({
        makeMarker(1, "Left", 12.0f, 0.0f, MapMarkerKind::Location),    // screen x = 112
        makeMarker(2, "Right", 32.0f, 0.0f, MapMarkerKind::Location),   // screen x = 132, cell boundary at 130
    });
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.clusterCount() == 1, "markers across a cell boundary still cluster");
    TEST_ASSERT(canvas.projectedCount() == 0, "both markers hide inside the badge");
}

// 13. Adaptive grid step and axis labels.
void testAdaptiveGridStep() {
    std::printf("testAdaptiveGridStep\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "Here", 30.0f, 30.0f) });

    MapCanvas canvas;
    canvas.setDataSource(&source).mode(MapMode::World).showGrid(true).showDistances(false)
          .clusterPixelSize(0.0f).followPlayer(false);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 600.0f, 600.0f });

    MockRenderBackend mock;

    canvas.metersPerPixel(1.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.gridStepMeters() == 100.0f, "at 1 m/px the grid step is 100 m");

    canvas.metersPerPixel(5.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.gridStepMeters() == 500.0f, "at 5 m/px the grid step is 500 m");

    canvas.metersPerPixel(0.5f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.gridStepMeters() == 50.0f, "at 0.5 m/px the grid step is 50 m");

    canvas.metersPerPixel(50.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.gridStepMeters() == 5000.0f, "at 50 m/px the grid step is 5000 m");

    // At 5 m/px 596 px window = ~1490 m: lines every 500 m, coordinate labels at borders.
    canvas.metersPerPixel(5.0f);
    renderOnce(canvas, mock);
    TEST_ASSERT(countOf(mock, CommandType::Line) >= 8, "grid draws lines in both directions");
    TEST_ASSERT(hasTextContaining(mock, "-500"), "axis label shows the grid coordinate");
    TEST_ASSERT(hasTextContaining(mock, "500"), "axis label shows the positive coordinate");

    // Grid disabled - no step lines.
    canvas.showGrid(false);
    renderOnce(canvas, mock);
    TEST_ASSERT(canvas.gridStepMeters() == 0.0f, "grid off means no step");
}

// 14. Camera input: mouse wheel, dragging, stick.
void testCameraInput() {
    std::printf("testCameraInput\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "Here", 20.0f, 0.0f, MapMarkerKind::Location) });

    MapCanvas canvas;
    std::uint64_t clicked = 0;
    canvas.setDataSource(&source).mode(MapMode::World).showDistances(false).clusterPixelSize(0.0f)
          .metersPerPixel(1.0f);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    canvas.cameraCenter(Point{ 0.0f, 0.0f });
    canvas.onMarkerClicked([&](const MapMarker& marker) { clicked = marker.id; });

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    // Wheel: zoom into point under cursor, follow mode stays in place.
    const float before = canvas.metersPerPixel();
    TEST_ASSERT(canvas.onMouseWheel(1.0f, Point{ 120.0f, 100.0f }), "wheel is consumed in World");
    TEST_ASSERT(canvas.metersPerPixel() < before, "wheel up zooms in");
    TEST_ASSERT(canvas.onMouseWheel(0.0f, Point{ 120.0f, 100.0f }) == false, "zero wheel delta is ignored");

    // Dragging: map moves with cursor, click does not trigger.
    canvas.followPlayer(false);
    const Point cameraBefore = canvas.cameraCenter();
    canvas.onPointerDown(Point{ 100.0f, 100.0f });
    canvas.onPointerMove(Point{ 120.0f, 100.0f });
    canvas.onPointerMove(Point{ 140.0f, 100.0f });
    TEST_ASSERT(canvas.onPointerUp(Point{ 140.0f, 100.0f }), "drag release is consumed");
    TEST_ASSERT(clicked == 0, "drag does not fire the marker callback");
    TEST_ASSERT(canvas.cameraCenter().x < cameraBefore.x, "dragging right moves the camera west");

    // Tapping marker after dragging is a normal click: settles camera into known state.
    canvas.cameraCenter(Point{ 0.0f, 0.0f });
    canvas.metersPerPixel(1.0f);
    renderOnce(canvas, mock);
    // Marker (20, 0) at 1 m/px and camera (0, 0) => screen (120, 100).
    TEST_ASSERT(canvas.onPointerUp(Point{ 120.0f, 100.0f }), "tap on a marker is consumed");
    TEST_ASSERT(clicked == 1, "tap delivers the marker id");

    // 'Center on player' button: moved camera away - tapping icon returns to player.
    canvas.cameraCenter(Point{ 400.0f, 0.0f });
    renderOnce(canvas, mock);
    const Rect button = canvas.recenterButtonRect();
    TEST_ASSERT(canvas.onPointerUp(Point{ button.x + button.width * 0.5f,
                                          button.y + button.height * 0.5f }),
                "tap on the recenter button is consumed");
    TEST_ASSERT(near(canvas.cameraCenter().x, 0.0f, 0.5f), "recenter button returns the camera to the player");
    TEST_ASSERT(canvas.isFollowingPlayer(), "recenter resumes following the player");
    TEST_ASSERT(clicked == 1, "recenter tap does not select a marker");

    // Right stick: direction = where camera travels in world; not passed to host.
    canvas.cameraCenter(Point{ 0.0f, 0.0f });
    const Point beforeNav = canvas.cameraCenter();
    TEST_ASSERT(canvas.onAction(NavDirection::Right), "nav action is handled in World");
    TEST_ASSERT(canvas.cameraCenter().x > beforeNav.x, "nav right moves the camera east");
    TEST_ASSERT(canvas.onAction(NavDirection::Up), "nav up is handled too");
    TEST_ASSERT(canvas.cameraCenter().y > beforeNav.y, "nav up moves the camera north");
    const Point afterUp = canvas.cameraCenter();
    TEST_ASSERT(canvas.onAction(NavDirection::Left), "nav left is handled too");
    TEST_ASSERT(canvas.cameraCenter().x < afterUp.x, "nav left moves the camera west");

    // In Radar mode map does not intercept camera input.
    canvas.mode(MapMode::Radar);
    TEST_ASSERT(!canvas.onMouseWheel(1.0f, Point{ 100.0f, 100.0f }), "wheel ignored in Radar");
    TEST_ASSERT(!canvas.onAction(NavDirection::Left), "nav ignored in Radar");

    // 'Back' from gamepad: VK_BACK (0x08) forwarded to host, not swallowed.
    bool backed = false;
    canvas.mode(MapMode::World);
    canvas.onBackground([&]() { backed = true; });
    TEST_ASSERT(canvas.onKeyDown(0x08), "back key is handled when a callback is set");
    TEST_ASSERT(backed, "back callback fires");
    TEST_ASSERT(!canvas.onKeyDown('M'), "unrelated keys pass through");
}

int main() {
    std::printf("=== MapCanvas tests ===\n");

    testWorldToScreen();
    testProportionalDistance();
    testFarTargetEdgeArrow();
    testDegenerateInputs();
    testOffSpaceMarker();
    testRevisionCache();
    testMarkerClick();
    testWorldCameraProjection();
    testWorldActiveLink();
    testZoomAroundCursor();
    testPanAndBoundsClamp();
    testClustering();
    testAdaptiveGridStep();
    testCameraInput();
    benchmarkThousandMarkers();

    std::printf("--- passed: %d, failed: %d ---\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}

