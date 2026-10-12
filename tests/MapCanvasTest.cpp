// Headless-тесты MapCanvas на MockRenderBackend: без ImGui, D3D и Skyrim.
// Проверяют преобразование «мир → экран», проекцию по дальности, стрелку за радиусом,
// sameSpace, кеш геометрии по revision, клик и крайние случаи.

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

// Круг небольшого радиуса (фигура маркера, не кольцо дальности и не рамка) с центром в точке.
bool hasMarkerCircleAt(const MockRenderBackend& mock, float x, float y, float maxRadius) {
    for (const auto& cmd : mock.commands()) {
        if (cmd.type != CommandType::Circle) continue;
        const float cx = cmd.bounds.x + cmd.radius;
        const float cy = cmd.bounds.y + cmd.radius;
        if (cmd.radius <= maxRadius && near(cx, x, 1.0f) && near(cy, y, 1.0f)) return true;
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

// Квадрат 200x200 => центр (100,100), радиус 97.
MapCanvas makeCanvas(VectorMapDataSource& source, float range) {
    MapCanvas canvas;
    canvas.setDataSource(&source).radarRange(range).showDistances(false);
    canvas.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 200.0f });
    return canvas;
}
// 1. Преобразование мир → экран: север над центром при heading = 0 и слева при heading = 90.
void testWorldToScreen() {
    std::printf("testWorldToScreen\n");

    VectorMapDataSource source;
    source.setPlayer(0.0f, 0.0f, 0.0f);
    source.setMarkers({ makeMarker(1, "North", 0.0f, 50.0f, MapMarkerKind::Quest) });

    MapCanvas canvas = makeCanvas(source, 100.0f);

    MockRenderBackend mock;
    renderOnce(canvas, mock);

    // цель на 50 м из радиуса 100 м => 48.5 px вверх от центра
    const Point center = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(center.x, 100.0f), "north target stays centered horizontally at heading 0");
    TEST_ASSERT(near(center.y, 51.5f), "north target is above the player at heading 0");

    source.setPlayer(0.0f, 0.0f, 90.0f);  // смотрим на восток — север уходит влево
    renderOnce(canvas, mock);

    const Point rotated = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(rotated.x, 51.5f), "north target moves left at heading 90");
    TEST_ASSERT(near(rotated.y, 100.0f), "north target is vertically centered at heading 90");

    // Восточная цель при heading = 90 уходит вверх: карта вращается вместе с игроком.
    source.setMarkers({ makeMarker(2, "East", 50.0f, 0.0f, MapMarkerKind::Quest) });
    renderOnce(canvas, mock);

    const Point east = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(east.x, 100.0f), "east target is centered horizontally when facing east");
    TEST_ASSERT(near(east.y, 51.5f), "east target is ahead of the player when facing east");

    // rotateWithPlayer = false: север всегда вверху, независимо от поворота.
    canvas.rotateWithPlayer(false);
    source.setMarkers({ makeMarker(3, "North", 0.0f, 50.0f, MapMarkerKind::Quest) });
    renderOnce(canvas, mock);

    const Point northUp = shapeCenter(findLast(mock, CommandType::Quad));
    TEST_ASSERT(near(northUp.x, 100.0f) && near(northUp.y, 51.5f), "north stays up when not rotating with player");
}

// 2. Маркер внутри радиуса рисуется пропорционально метрам.
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

// 3. Маркер вне радиуса: стрелка на границе и подпись с расстоянием.
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

    // Первый треугольник — стрелка игрока, второй — стрелка дальней цели.
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

    // Приближение цели переводит её из стрелки на границе в обычную точку.
    source.setMarkers({ makeMarker(1, "Far Keep", 0.0f, 100.0f, MapMarkerKind::Location) });
    renderOnce(canvas, mock);

    TEST_ASSERT(canvas.projectedCount() == 1, "inside-range target is still projected");
    TEST_ASSERT(countOf(mock, CommandType::Triangle) == 1, "inside-range target no longer draws an edge arrow");
    TEST_ASSERT(hasMarkerCircleAt(mock, 100.0f, 67.7f, 12.0f),
                "inside-range target is drawn at its proportional spot (100 м из 300 м радиуса)");
}

// 4. Крайние случаи: нулевой радиус, пустой список, nullptr источник, уничтоженный источник.
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

// 5. sameSpace = false: цели нет на карте, есть подпись «в другом месте».
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
    TEST_ASSERT(hasTextContaining(mock, "другом месте"), "off-space marker shows a place badge");
}

// 6. Не изменился revision — геометрия не пересчитывается.
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

// 7. Клик по маркеру.
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

    // Дальняя цель: кликабельна стрелка на границе.
    source.setMarkers({ makeMarker(8, "Far", 0.0f, 900.0f, MapMarkerKind::Quest) });
    renderOnce(canvas, mock);
    clicked = 0;
    TEST_ASSERT(canvas.onPointerUp(Point{ 100.0f, 8.0f }), "click on edge arrow is consumed");
    TEST_ASSERT(clicked == 8, "edge arrow delivers its marker");
}

// 8. Бенчмарк: 1000 маркеров, время одного кадра на mock. Жёсткого порога нет.
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
    renderOnce(canvas, mock);  // прогрев: пересчёт геометрии

    const auto begin = std::chrono::steady_clock::now();
    for (int frame = 0; frame < 100; ++frame) {
        renderOnce(canvas, mock);
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - begin);

    std::printf("  100 кадров по 1000 маркеров: %.1f мкс на кадр, %zu вызовов отрисовки за кадр\n",
                static_cast<double>(elapsed.count()) / 100.0, mock.drawCallCount());

    TEST_ASSERT(canvas.projectedCount() == 1000, "all 1000 markers projected");
    TEST_ASSERT(canvas.recomputeCount() == 1, "steady data does not recompute per frame");
}

}  // namespace

int main() {
    std::printf("=== MapCanvas tests ===\n");

    testWorldToScreen();
    testProportionalDistance();
    testFarTargetEdgeArrow();
    testDegenerateInputs();
    testOffSpaceMarker();
    testRevisionCache();
    testMarkerClick();
    benchmarkThousandMarkers();

    std::printf("--- passed: %d, failed: %d ---\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
