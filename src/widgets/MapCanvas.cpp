#include "PerfUI/MapCanvas.h"

#include "PerfUI/Theme.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <unordered_map>

namespace PerfUI {

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kDegToRad = kPi / 180.0f;
constexpr std::size_t kMaxLabels = 12;
constexpr std::size_t kLabelCandidates = 64;

float normalizeDeg(float deg) {
    float d = std::fmod(deg, 360.0f);
    if (d < 0.0f) d += 360.0f;
    return d;
}

float bearingTo(float dx, float dy) {
    return normalizeDeg(std::atan2f(dx, dy) / kDegToRad);
}

void formatMeters(char* buf, std::size_t size, float meters) {
    std::snprintf(buf, size, "%d m", static_cast<int>(std::lroundf(meters)));
}

void formatCount(char* buf, std::size_t size, std::size_t count) {
    std::snprintf(buf, size, "%d", static_cast<int>(count));
}

// Grid step label: integer meters without trailing zeros.
void formatAxis(char* buf, std::size_t size, float meters) {
    if (std::fabs(meters) < 0.5f) {
        std::snprintf(buf, size, "0");
        return;
    }
    std::snprintf(buf, size, "%d", static_cast<int>(std::lroundf(meters)));
}

PerfUI::Point along(const PerfUI::Point& origin, float angleDeg, float distance) {
    const float rad = angleDeg * kDegToRad;
    return { origin.x + std::sin(rad) * distance, origin.y - std::cos(rad) * distance };
}

PerfUI::Point perpendicular(float angleDeg) {
    const float rad = angleDeg * kDegToRad;
    return { std::cos(rad), std::sin(rad) };
}

PerfUI::Point directionVector(float angleDeg) {
    const float rad = angleDeg * kDegToRad;
    return { std::sin(rad), -std::cos(rad) };
}

}  // namespace

MapCanvas::MapCanvas(std::string name)
    : UIElement(std::move(name)) {
    // In the focus graph, the map is a single element: stick pans camera rather than traversing markers.
    setFocusable(true);
}

MapCanvas& MapCanvas::setDataSource(IMapDataSource* src) {
    m_source = src;
    return *this;
}

MapCanvas& MapCanvas::mode(MapMode m) {
    m_mode = m;
    return *this;
}

MapCanvas& MapCanvas::radarRange(float meters) {
    m_radarRange = meters > 0.0f ? meters : 0.0f;
    return *this;
}

MapCanvas& MapCanvas::showDistances(bool on) {
    m_showDistances = on;
    return *this;
}

MapCanvas& MapCanvas::showGrid(bool on) {
    m_showGrid = on;
    return *this;
}

MapCanvas& MapCanvas::rotateWithPlayer(bool on) {
    m_rotateWithPlayer = on;
    return *this;
}

MapCanvas& MapCanvas::onMarkerClicked(std::function<void(const MapMarker&)> cb) {
    m_onMarkerClicked = std::move(cb);
    return *this;
}

MapCanvas& MapCanvas::clampToBounds(float minX, float minY, float maxX, float maxY) {
    if (maxX <= minX || maxY <= minY) {
        m_hasBounds = false;
        m_boundsMeters = Rect{};
        return *this;
    }
    m_hasBounds = true;
    m_boundsMeters = Rect{ minX, minY, maxX - minX, maxY - minY };
    clampCamera();
    markLayoutDirty();
    return *this;
}

MapCanvas& MapCanvas::clearBounds() {
    m_hasBounds = false;
    m_boundsMeters = Rect{};
    return *this;
}

MapCanvas& MapCanvas::metersPerPixel(float meters) {
    m_metersPerPixel = std::clamp(meters, m_minMetersPerPixel, m_maxMetersPerPixel);
    return *this;
}

MapCanvas& MapCanvas::zoomLimits(float minMetersPerPixel, float maxMetersPerPixel) {
    if (maxMetersPerPixel <= 0.0f || minMetersPerPixel <= 0.0f || maxMetersPerPixel < minMetersPerPixel) {
        return *this;
    }
    m_minMetersPerPixel = minMetersPerPixel;
    m_maxMetersPerPixel = maxMetersPerPixel;
    return metersPerPixel(m_metersPerPixel);
}

MapCanvas& MapCanvas::cameraCenter(Point meters) {
    m_camera = meters;
    m_followPlayer = false;
    clampCamera();
    return *this;
}

MapCanvas& MapCanvas::followPlayer(bool on) {
    m_followPlayer = on;
    if (on) centerOnPlayer();
    return *this;
}

MapCanvas& MapCanvas::clusterPixelSize(float pixels) {
    m_clusterPx = pixels > 0.0f ? pixels : 0.0f;
    return *this;
}

MapCanvas& MapCanvas::onBackground(std::function<void()> cb) {
    m_onBack = std::move(cb);
    return *this;
}

void MapCanvas::centerOnPlayer() {
    if (!m_source) return;
    const MapPlayerState player = m_source->player();
    m_camera = Point{ player.x, player.y };
    m_followPlayer = true;   // returned to player — follow player again
    clampCamera();
}

void MapCanvas::centerOn(Point meters) {
    m_camera = meters;
    m_followPlayer = false;   // explicit user navigation takes priority over auto-follow
    clampCamera();
}

void MapCanvas::zoomBy(float factor, const Point* anchorLocalPx) {
    if (!(factor > 0.0f) || m_metersPerPixel <= 0.0f) return;

    // The point under anchor should remain in place: remember world under cursor before zooming.
    const Point anchorPx = anchorLocalPx ? *anchorLocalPx : center();
    const Point anchorWorld = screenToWorld(anchorPx);
    const Point c = center();
    const float nextPerPixel = std::clamp(m_metersPerPixel * factor, m_minMetersPerPixel, m_maxMetersPerPixel);

    m_camera = Point{ anchorWorld.x - (anchorPx.x - c.x) * nextPerPixel,
                      anchorWorld.y + (anchorPx.y - c.y) * nextPerPixel };
    m_metersPerPixel = nextPerPixel;
    clampCamera();
}

void MapCanvas::moveCameraMeters(Point deltaMeters) {
    m_camera = m_camera + deltaMeters;
    clampCamera();
}

void MapCanvas::panMeters(Point deltaMeters) {
    m_camera = m_camera - deltaMeters;
    clampCamera();
}

void MapCanvas::panPixels(Point deltaPx) {
    // Dragging pulls map along: camera center moves in opposite direction.
    panMeters(Point{ deltaPx.x * m_metersPerPixel, -deltaPx.y * m_metersPerPixel });
}

void MapCanvas::update(float deltaTime) {
    UIElement::update(deltaTime);

    // Ping-pong pulse animation: tick to 1, then back to 0.
    if (m_pulse.isAtTarget()) {
        m_pulse.setTarget(m_pulse.target() > 0.5f ? 0.0f : 1.0f);
    }
    m_pulse.update(deltaTime);
}

void MapCanvas::measure(Dimensions availableSize) {
    const auto& pad = layout().padding();

    float w = 160.0f;
    float h = 160.0f;

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    } else if (layout().width().mode == SizeMode::Percent) {
        w = availableSize.width * (layout().width().value / 100.0f);
    }

    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    } else if (layout().height().mode == SizeMode::Percent) {
        h = availableSize.height * (layout().height().value / 100.0f);
    }

    w += pad.horizontal();
    h += pad.vertical();

    w = (std::clamp)(w, layout().minWidth(), layout().maxWidth());
    h = (std::clamp)(h, layout().minHeight(), layout().maxHeight());

    m_desiredSize = Dimensions{ w, h };
}

Point MapCanvas::center() const {
    return { m_bounds.x + m_bounds.width * 0.5f, m_bounds.y + m_bounds.height * 0.5f };
}

float MapCanvas::radiusPx() const {
    const float half = (std::min)(m_bounds.width, m_bounds.height) * 0.5f;
    return half > 3.0f ? half - 3.0f : 0.0f;
}

float MapCanvas::scaleFactor() const {
    return (std::clamp)(radiusPx() / 80.0f, 0.75f, 1.75f);
}

TextStyle MapCanvas::labelStyle() const {
    const auto& th = Theme::Current();
    TextStyle style;
    style.color = th.colors.textPrimary;
    style.fontSize = (std::clamp)(th.typography.fontSmall * scaleFactor(), 8.0f, 20.0f);
    return style;
}

Color MapCanvas::colorFor(const MapMarker& marker) {
    if (marker.color.a != 0) return marker.color;

    const auto& th = Theme::Current();
    switch (marker.kind) {
    case MapMarkerKind::Quest:
        return th.colors.textAccent;
    case MapMarkerKind::Location:
        return { 126, 200, 227, 255 };
    case MapMarkerKind::Enemy:
        return { 214, 86, 78, 255 };
    case MapMarkerKind::Custom:
        return th.colors.borderFocus;
    }
    return th.colors.textPrimary;
}

Rect MapCanvas::viewRect() const {
    const float inset = 2.0f;
    return Rect{ m_bounds.x + inset, m_bounds.y + inset,
                 (std::max)(0.0f, m_bounds.width - inset * 2.0f),
                 (std::max)(0.0f, m_bounds.height - inset * 2.0f) };
}

Rect MapCanvas::recenterButtonRect() const {
    const float scale = scaleFactor();
    const float size = 28.0f * scale;
    const Rect view = viewRect();
    // Top-right corner: keeps clear of axis labels (bottom/left) and off-space badge.
    return Rect{ view.x + view.width - size - 6.0f * scale, view.y + 6.0f * scale, size, size };
}

Point MapCanvas::worldToScreen(Point meters) const {
    const Point c = center();
    return { c.x + (meters.x - m_camera.x) / m_metersPerPixel,
             c.y - (meters.y - m_camera.y) / m_metersPerPixel };
}

Point MapCanvas::screenToWorld(Point pixels) const {
    const Point c = center();
    return { m_camera.x + (pixels.x - c.x) * m_metersPerPixel,
             m_camera.y - (pixels.y - c.y) * m_metersPerPixel };
}

void MapCanvas::clampCamera() {
    if (!m_hasBounds || m_boundsMeters.width <= 0.0f || m_boundsMeters.height <= 0.0f) return;

    // Visible window half in meters; cannot exceed bounds, but if bounds are smaller
    // than window, keep centered to prevent sticking to an edge.
    const Rect view = viewRect();
    const float halfW = view.width * 0.5f * m_metersPerPixel;
    const float halfH = view.height * 0.5f * m_metersPerPixel;
    const float spanX = m_boundsMeters.width;
    const float spanY = m_boundsMeters.height;

    const float minX = m_boundsMeters.x + (std::min)(halfW, spanX * 0.5f);
    const float maxX = m_boundsMeters.x + spanX - (std::min)(halfW, spanX * 0.5f);
    const float minY = m_boundsMeters.y + (std::min)(halfH, spanY * 0.5f);
    const float maxY = m_boundsMeters.y + spanY - (std::min)(halfH, spanY * 0.5f);

    m_camera.x = maxX >= minX ? (std::clamp)(m_camera.x, minX, maxX)
                              : m_boundsMeters.x + spanX * 0.5f;
    m_camera.y = maxY >= minY ? (std::clamp)(m_camera.y, minY, maxY)
                              : m_boundsMeters.y + spanY * 0.5f;
}

Point MapCanvas::clampRayToRect(Point from, Point to, const Rect& rect) {
    // Segment from->to intersection with window boundary. If inside, find exit (tMax);
    // if outside, find entry (tMin). Segments missing the window entirely are clamped to edge.
    float tMin = 0.0f;
    float tMax = 1.0f;
    const Point d = to - from;

    const float lo[2] = { rect.x, rect.y };
    const float hi[2] = { rect.x + rect.width, rect.y + rect.height };
    const float o[2] = { from.x, from.y };
    const float v[2] = { d.x, d.y };

    for (int axis = 0; axis < 2; ++axis) {
        if (std::fabs(v[axis]) < 1e-6f) continue;   // parallel to axis — bounded only by lo/hi below

        float t1 = (lo[axis] - o[axis]) / v[axis];
        float t2 = (hi[axis] - o[axis]) / v[axis];
        if (t1 > t2) std::swap(t1, t2);
        tMin = (std::max)(tMin, t1);
        tMax = (std::min)(tMax, t2);
    }

    const bool crosses = tMax >= tMin;
    const float t = rect.contains(from) ? (std::clamp)(tMax, 0.0f, 1.0f)
                                        : (std::clamp)(tMin, 0.0f, 1.0f);

    if (crosses && (tMin <= 1.0f && tMax >= 0.0f)) {
        return { from.x + d.x * t, from.y + d.y * t };
    }

    return Point{ (std::clamp)(to.x, lo[0], hi[0]), (std::clamp)(to.y, lo[1], hi[1]) };
}

float MapCanvas::pickGridStep() const {
    if (!m_showGrid || m_mode != MapMode::World || m_metersPerPixel <= 0.0f) return 0.0f;

    // Select step so grid lines maintain at least ~64 px separation.
    static constexpr float steps[] = { 10.0f, 20.0f, 50.0f, 100.0f, 250.0f,
                                       500.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f };
    const float minSpacingPx = 64.0f;
    for (const float step : steps) {
        if (step / m_metersPerPixel >= minSpacingPx) return step;
    }
    return steps[std::size(steps) - 1];
}

void MapCanvas::refreshSnapshot() {
    const std::uint64_t revision = m_source ? m_source->revision() : 0ULL;
    const void* source = m_source;

    if (source == m_snapshotSource && revision == m_snapshotRevision) return;

    m_snapshotSource = source;
    m_snapshotRevision = revision;
    m_snapshot.clear();

    if (!m_source) return;

    const std::span<const MapMarker> markers = m_source->markers();
    m_snapshot.assign(markers.begin(), markers.end());
}

void MapCanvas::recompute() {
    m_projected.clear();
    m_clusters.clear();
    m_active = -1;
    m_offSpaceActive = -1;
    m_gridStep = pickGridStep();

    refreshSnapshot();
    if (!m_source || m_snapshot.empty()) return;

    if (m_mode == MapMode::World) {
        recomputeWorld();
        return;
    }
    recomputeRadar();
}

void MapCanvas::recomputeRadar() {
    const MapPlayerState player = m_source->player();
    const float radius = radiusPx();
    if (radius <= 0.0f || m_radarRange <= 0.0f) return;

    const bool rotate = m_rotateWithPlayer;
    const float heading = normalizeDeg(player.headingDeg);
    m_playerAngleDeg = rotate ? 0.0f : heading;

    const Point c = center();

    for (std::size_t i = 0; i < m_snapshot.size(); ++i) {
        const MapMarker& marker = m_snapshot[i];

        if (!marker.sameSpace) {
            if (marker.active && m_offSpaceActive < 0) m_offSpaceActive = static_cast<long>(i);
            continue;
        }

        const float dx = marker.x - player.x;
        const float dy = marker.y - player.y;
        const float distance = std::sqrtf(dx * dx + dy * dy);

        Projected pr;
        pr.marker = i;
        pr.distanceMeters = distance;
        pr.bearingDeg = bearingTo(dx, dy);
        pr.screenAngleDeg = normalizeDeg(pr.bearingDeg - (rotate ? heading : 0.0f));
        pr.onEdge = distance > m_radarRange;

        const float px = pr.onEdge ? radius : (distance / m_radarRange) * radius;
        pr.screen = along(c, pr.screenAngleDeg, px);

        m_projected.push_back(pr);

        if (marker.active && m_active < 0) m_active = static_cast<long>(m_projected.size() - 1);
    }
}

void MapCanvas::recomputeWorld() {
    const MapPlayerState player = m_source->player();
    const Rect view = viewRect();
    if (view.width <= 0.0f || view.height <= 0.0f || m_metersPerPixel <= 0.0f) return;

    if (m_followPlayer) centerOnPlayer();
    m_playerAngleDeg = normalizeDeg(player.headingDeg);

    const Point playerScreen = worldToScreen(Point{ player.x, player.y });

    for (std::size_t i = 0; i < m_snapshot.size(); ++i) {
        const MapMarker& marker = m_snapshot[i];

        if (!marker.sameSpace) {
            if (marker.active && m_offSpaceActive < 0) m_offSpaceActive = static_cast<long>(i);
            continue;
        }

        const float dx = marker.x - player.x;
        const float dy = marker.y - player.y;

        Projected pr;
        pr.marker = i;
        pr.distanceMeters = std::sqrtf(dx * dx + dy * dy);
        pr.bearingDeg = bearingTo(dx, dy);
        pr.screenAngleDeg = pr.bearingDeg;   // World mode is always north-up
        pr.screen = worldToScreen(Point{ marker.x, marker.y });
        pr.onEdge = !view.contains(pr.screen);

        if (marker.active) {
            // Always keep target on map: outside window, show indicator at edge.
            if (pr.onEdge) pr.screen = clampRayToRect(playerScreen, pr.screen, view);
            m_projected.push_back(pr);
            if (m_active < 0) m_active = static_cast<long>(m_projected.size() - 1);
            continue;
        }

        if (pr.onEdge) continue;   // skip drawing/clustering markers outside window
        m_projected.push_back(pr);
    }

    if (m_clusterPx <= 0.0f || m_projected.size() < 2) return;

    // Active target is excluded from clustering to stay visible and clickable.
    const long activeIdx = m_active;

    // Spatial hash: cell matches cluster radius; neighbours searched in 3x3 neighbourhood.
    // Prevents clustering flickering when camera moves across grid cell edges.
    const auto cellOf = [this](const Point& p) {
        return Point{ std::floor(p.x / m_clusterPx), std::floor(p.y / m_clusterPx) };
    };
    const auto cellKey = [](const Point& cell) {
        return (static_cast<std::int64_t>(static_cast<std::uint32_t>(static_cast<std::int32_t>(cell.x))) << 32) |
               static_cast<std::uint32_t>(static_cast<std::int32_t>(cell.y));
    };

    std::unordered_map<std::int64_t, std::vector<std::size_t>> cells;
    cells.reserve(m_projected.size() * 2);
    for (std::size_t idx = 0; idx < m_projected.size(); ++idx) {
        if (static_cast<long>(idx) == activeIdx) continue;
        cells[cellKey(cellOf(m_projected[idx].screen))].push_back(idx);
    }

    const float radiusSq = m_clusterPx * m_clusterPx;
    std::vector<bool> consumed(m_projected.size(), false);

    // Greedy: first unassigned marker becomes cluster seed and absorbs neighbours
    // within radius. Order is deterministic.
    for (std::size_t seed = 0; seed < m_projected.size(); ++seed) {
        if (consumed[seed] || static_cast<long>(seed) == activeIdx) continue;

        const Point origin = m_projected[seed].screen;
        const Point cell = cellOf(origin);
        std::vector<std::size_t> members;

        for (std::int32_t gx = -1; gx <= 1; ++gx) {
            for (std::int32_t gy = -1; gy <= 1; ++gy) {
                const Point neighbour{ cell.x + static_cast<float>(gx), cell.y + static_cast<float>(gy) };
                const auto found = cells.find(cellKey(neighbour));
                if (found == cells.end()) continue;

                for (const std::size_t idx : found->second) {
                    if (consumed[idx] || idx == seed) continue;
                    const Point delta = m_projected[idx].screen - origin;
                    if (delta.x * delta.x + delta.y * delta.y > radiusSq) continue;
                    members.push_back(idx);
                }
            }
        }

        if (members.empty()) continue;   // single marker: render normally

        consumed[seed] = true;
        Point sum = origin;
        for (const std::size_t idx : members) {
            consumed[idx] = true;
            sum = sum + m_projected[idx].screen;
        }

        Cluster cluster;
        cluster.screen = { sum.x / static_cast<float>(members.size() + 1),
                           sum.y / static_cast<float>(members.size() + 1) };
        cluster.count = members.size() + 1;
        cluster.marker = seed;
        m_clusters.push_back(cluster);
    }

    if (m_clusters.empty()) return;

    std::sort(m_clusters.begin(), m_clusters.end(),
              [](const Cluster& a, const Cluster& b) { return a.marker < b.marker; });

    std::vector<Projected> remaining;
    remaining.reserve(m_projected.size());
    for (std::size_t idx = 0; idx < m_projected.size(); ++idx) {
        if (!consumed[idx]) remaining.push_back(m_projected[idx]);
    }

    // Active target index may have shifted — locate by marker index.
    const long wantedMarker = activeIdx >= 0
                                  ? static_cast<long>(m_projected[static_cast<std::size_t>(activeIdx)].marker)
                                  : -1L;
    m_projected = std::move(remaining);
    m_active = -1L;
    if (wantedMarker >= 0) {
        for (std::size_t idx = 0; idx < m_projected.size(); ++idx) {
            if (m_projected[idx].marker == static_cast<std::size_t>(wantedMarker)) {
                m_active = static_cast<long>(idx);
                break;
            }
        }
    }
}

void MapCanvas::render(UIRenderBackend& backend) {
    if (!isVisible() || m_bounds.width <= 1.0f || m_bounds.height <= 1.0f) return;

    const CacheKey key{
        m_source,
        m_source ? m_source->revision() : 0ULL,
        m_bounds.width,
        m_bounds.height,
        m_mode,
        m_radarRange,
        m_rotateWithPlayer,
        m_showGrid,
        m_camera,
        m_metersPerPixel,
        m_clusterPx,
        m_hasBounds ? m_boundsMeters : Rect{},
    };

    if (!(key == m_key)) {
        m_key = key;
        recompute();
        ++m_recomputeCount;
    }

    const auto& th = Theme::Current();
    backend.pushClipRect(m_bounds);

    if (m_mode == MapMode::World) {
        const Rect view = viewRect();
        backend.drawRect(m_bounds, th.colors.surfaceBase);

        if (m_showGrid) drawWorldGrid(backend);
        drawActiveLink(backend);
        drawMarkers(backend);
        drawClusters(backend);
        drawLabels(backend);
        drawPlayer(backend);
        drawOffSpaceBadge(backend);

        backend.drawRectOutline(view, th.colors.borderStrong, 1.5f * scaleFactor());
        drawRecenterButton(backend);
    } else {
        const float radius = radiusPx();
        if (radius > 0.0f) {
            backend.drawRoundedRect(m_bounds, th.colors.surfaceBase, radius);

            if (m_showGrid) drawGrid(backend);
            drawRings(backend);
            drawActiveTarget(backend);
            drawMarkers(backend);
            drawLabels(backend);
            drawPlayer(backend);
            drawOffSpaceBadge(backend);

            backend.drawCircle(center(), radius, Color::Transparent(), th.colors.borderStrong, 1.5f * scaleFactor());
        }
    }

    backend.popClipRect();

    UIElement::render(backend);
}

void MapCanvas::drawRings(UIRenderBackend& backend) {
    if (m_radarRange <= 0.0f) return;

    const auto& th = Theme::Current();
    const Point c = center();
    const float radius = radiusPx();
    const float scale = scaleFactor();

    for (int ring = 1; ring <= 3; ++ring) {
        const float fraction = static_cast<float>(ring) / 3.0f;
        const float r = radius * fraction;
        backend.drawCircle(c, r, Color::Transparent(), th.colors.borderSubtle, 1.0f * scale);

        if (!m_showDistances) continue;

        char buf[32];
        formatMeters(buf, sizeof(buf), m_radarRange * fraction);
        const TextStyle style = labelStyle();
        backend.drawText(buf, Point{ c.x + 3.0f * scale, c.y - r - style.fontSize * 0.65f }, style);
    }
}

void MapCanvas::drawGrid(UIRenderBackend& backend) {
    const auto& th = Theme::Current();
    const Point c = center();
    const float radius = radiusPx();
    const float thickness = 1.0f * scaleFactor();

    for (const float angle : { 0.0f, 90.0f, 180.0f, 270.0f }) {
        backend.drawLine(c, along(c, angle, radius), th.colors.borderSubtle, thickness);
    }
}

void MapCanvas::drawWorldGrid(UIRenderBackend& backend) {
    const float step = m_gridStep;
    if (step <= 0.0f || m_metersPerPixel <= 0.0f) return;

    const auto& th = Theme::Current();
    const Rect view = viewRect();
    const float thickness = 1.0f * scaleFactor();
    const TextStyle style = labelStyle();
    char buf[32];

    // World coordinates of top-left and bottom-right viewport bounds.
    const Point leftTop = screenToWorld(view.topLeft());
    const Point rightBottom = screenToWorld(Point{ view.x + view.width, view.y + view.height });

    const float firstX = std::floor(leftTop.x / step) * step;
    for (float worldX = firstX; worldX <= rightBottom.x + step; worldX += step) {
        const float px = worldToScreen(Point{ worldX, 0.0f }).x;
        if (px < view.x || px > view.x + view.width) continue;

        backend.drawLine(Point{ px, view.y }, Point{ px, view.y + view.height }, th.colors.borderSubtle, thickness);
        formatAxis(buf, sizeof(buf), worldX);
        backend.drawText(buf, Point{ px + 2.0f, view.y + view.height - style.fontSize * 1.3f }, style);
    }

    // Screen Y grows downward, so top of window corresponds to maximum world Y.
    const float firstY = std::floor(rightBottom.y / step) * step;
    for (float worldY = firstY; worldY <= leftTop.y + step; worldY += step) {
        const float py = worldToScreen(Point{ 0.0f, worldY }).y;
        if (py < view.y || py > view.y + view.height) continue;

        backend.drawLine(Point{ view.x, py }, Point{ view.x + view.width, py }, th.colors.borderSubtle, thickness);
        formatAxis(buf, sizeof(buf), worldY);
        backend.drawText(buf, Point{ view.x + 3.0f, py - style.fontSize * 0.9f }, style);
    }
}

// Line from player to active target with midpoint distance (World).
void MapCanvas::drawActiveLink(UIRenderBackend& backend) {
    if (m_active < 0 || !m_source) return;

    const auto& th = Theme::Current();
    const MapPlayerState player = m_source->player();
    const Projected& pr = m_projected[static_cast<std::size_t>(m_active)];
    const Point from = worldToScreen(Point{ player.x, player.y });

    backend.drawLine(from, pr.screen, th.colors.textAccent, 1.5f * scaleFactor());

    if (!m_showDistances) return;

    const TextStyle style = labelStyle();
    char buf[32];
    formatMeters(buf, sizeof(buf), pr.distanceMeters);
    const Dimensions size = backend.measureText(buf, style);
    const Point mid{ (from.x + pr.screen.x) * 0.5f, (from.y + pr.screen.y) * 0.5f };
    backend.drawText(buf, Point{ mid.x - size.width * 0.5f, mid.y - size.height * 0.5f }, style);
}

void MapCanvas::drawPlayer(UIRenderBackend& backend) {
    const auto& th = Theme::Current();
    const float scale = scaleFactor();
    const float size = 9.0f * scale;

    // Radar: player centered. World: projected according to world position, camera anywhere.
    Point c = center();
    if (m_mode == MapMode::World) {
        if (!m_source) return;
        const MapPlayerState player = m_source->player();
        c = worldToScreen(Point{ player.x, player.y });
    }

    backend.drawCircle(c, size * 1.2f, th.colors.surfaceActive, th.colors.borderStrong, 1.0f * scale);

    const Point dir = directionVector(m_playerAngleDeg);
    const Point perp = perpendicular(m_playerAngleDeg);

    const Point tip{ c.x + dir.x * size, c.y + dir.y * size };
    const Point back{ c.x - dir.x * size * 0.7f, c.y - dir.y * size * 0.7f };

    backend.drawTriangle(
        tip,
        Point{ back.x + perp.x * size * 0.6f, back.y + perp.y * size * 0.6f },
        Point{ back.x - perp.x * size * 0.6f, back.y - perp.y * size * 0.6f },
        th.colors.textAccent);
}

void MapCanvas::drawMarkers(UIRenderBackend& backend) {
    const float scale = scaleFactor();
    const Point c = center();
    const float radius = radiusPx();

    for (const Projected& pr : m_projected) {
        const MapMarker& marker = m_snapshot[pr.marker];
        Color color = colorFor(marker);
        if (!marker.discovered) color = Color{ color.r, color.g, color.b, 90 };

        const float size = (marker.active ? 7.0f : 5.0f) * scale;

        if (pr.onEdge) {
            const float arrowLen = (marker.active ? 15.0f : 11.0f) * scale;
            const float arrowWidth = (marker.active ? 6.5f : 4.5f) * scale;
            const Point dir = directionVector(pr.screenAngleDeg);
            const Point perp = perpendicular(pr.screenAngleDeg);
            // Radar: arrow on circumference. World: pr.screen is already clamped to window edge.
            const Point tip = m_mode == MapMode::World
                ? pr.screen
                : Point{ c.x + dir.x * (radius - 2.0f * scale), c.y + dir.y * (radius - 2.0f * scale) };
            const Point base{ tip.x - dir.x * arrowLen, tip.y - dir.y * arrowLen };

            backend.drawTriangle(
                tip,
                Point{ base.x + perp.x * arrowWidth, base.y + perp.y * arrowWidth },
                Point{ base.x - perp.x * arrowWidth, base.y - perp.y * arrowWidth },
                color);
            continue;
        }

        const Point p = pr.screen;

        switch (marker.kind) {
        case MapMarkerKind::Quest:
            backend.drawQuad(
                Point{ p.x, p.y - size },
                Point{ p.x + size, p.y },
                Point{ p.x, p.y + size },
                Point{ p.x - size, p.y },
                color);
            break;
        case MapMarkerKind::Location:
            backend.drawCircle(p, size * 0.9f, color);
            break;
        case MapMarkerKind::Enemy:
            backend.drawTriangle(
                Point{ p.x, p.y - size },
                Point{ p.x + size * 0.9f, p.y + size * 0.7f },
                Point{ p.x - size * 0.9f, p.y + size * 0.7f },
                color);
            break;
        case MapMarkerKind::Custom:
            backend.drawCircle(p, size * 0.65f, color);
            break;
        }
    }
}

void MapCanvas::drawClusters(UIRenderBackend& backend) {
    if (m_clusters.empty()) return;

    const auto& th = Theme::Current();
    const float scale = scaleFactor();
    const TextStyle style = labelStyle();

    for (const Cluster& cluster : m_clusters) {
        const float half = (std::max)(8.0f, m_clusterPx * 0.5f);
        backend.drawCircle(cluster.screen, half, th.colors.surfaceActive, th.colors.borderFocus, 1.5f * scale);

        char buf[16];
        formatCount(buf, sizeof(buf), cluster.count);
        const Dimensions size = backend.measureText(buf, style);
        backend.drawText(buf, Point{ cluster.screen.x - size.width * 0.5f,
                                     cluster.screen.y - size.height * 0.5f }, style);
    }
}

// Labels: nearest to player first; overlapping labels omitted to prevent clutter.
void MapCanvas::drawLabels(UIRenderBackend& backend) {
    if (m_projected.empty()) return;

    const float scale = scaleFactor();

    std::vector<std::size_t> order;
    order.reserve(m_projected.size());
    for (std::size_t i = 0; i < m_projected.size(); ++i) order.push_back(i);

    const std::size_t candidates = (std::min)(kLabelCandidates, order.size());
    std::partial_sort(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(candidates), order.end(),
        [this](std::size_t a, std::size_t b) {
            return m_projected[a].distanceMeters < m_projected[b].distanceMeters;
        });
    order.resize(candidates);

    const TextStyle style = labelStyle();
    const float offset = 8.0f * scale;
    std::vector<Rect> placed;
    placed.reserve(kMaxLabels);

    // Cluster badges occupy space — labels should not overlap them.
    for (const Cluster& cluster : m_clusters) {
        const float half = (std::max)(8.0f, m_clusterPx * 0.5f);
        placed.push_back(Rect{ cluster.screen.x - half, cluster.screen.y - half, half * 2.0f, half * 2.0f });
    }

    char distBuf[32];

    for (const std::size_t i : order) {
        if (placed.size() >= kMaxLabels + m_clusters.size()) break;

        const Projected& pr = m_projected[i];
        const MapMarker& marker = m_snapshot[pr.marker];

        std::string text = marker.label;
        formatMeters(distBuf, sizeof(distBuf), pr.distanceMeters);

        if (text.empty()) {
            if (!m_showDistances) continue;
            text = distBuf;
        } else if (m_showDistances) {
            text += " · ";
            text += distBuf;
        }

        const Dimensions size = backend.measureText(text, style);
        const Rect box{ pr.screen.x + offset, pr.screen.y - size.height * 0.5f, size.width, size.height };

        bool overlaps = false;
        for (const Rect& other : placed) {
            const bool intersects = box.x < other.x + other.width && box.x + box.width > other.x &&
                                    box.y < other.y + other.height && box.y + box.height > other.y;
            if (intersects) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) continue;

        placed.push_back(box);
        backend.drawText(text, Point{ box.x, box.y }, style);
    }
}

void MapCanvas::drawActiveTarget(UIRenderBackend& backend) {
    if (m_active < 0) return;

    const Projected& pr = m_projected[static_cast<std::size_t>(m_active)];
    const auto& th = Theme::Current();
    const Point c = center();
    const float radius = radiusPx();
    const float scale = scaleFactor();

    // Dashed line from player to target for radar; drawActiveLink serves this role in World mode.
    if (m_mode != MapMode::World && m_radarRange > 0.0f) {
        const float reach = pr.onEdge ? radius - 13.0f * scale : (pr.distanceMeters / m_radarRange) * radius;
        const float dash = 5.0f * scale;
        const float gap = 4.0f * scale;
        const Point dir = directionVector(pr.screenAngleDeg);

        for (float t = dash + gap; t < reach; t += dash + gap) {
            const float end = (std::min)(t + dash, reach);
            backend.drawLine(
                Point{ c.x + dir.x * t, c.y + dir.y * t },
                Point{ c.x + dir.x * end, c.y + dir.y * end },
                th.colors.textAccent,
                1.5f * scale);
        }
    }

    const Color accent = th.colors.textAccent;
    const float pulse = (std::clamp)(m_pulse.value(), 0.0f, 1.0f);
    backend.drawCircle(
        pr.screen,
        (8.0f + 8.0f * pulse) * scale,
        Color::Transparent(),
        Color{ accent.r, accent.g, accent.b, static_cast<std::uint8_t>(220.0f * (1.0f - pulse)) },
        2.0f * scale);
}

void MapCanvas::drawOffSpaceBadge(UIRenderBackend& backend) {
    if (m_offSpaceActive < 0) return;

    const auto& th = Theme::Current();
    const MapMarker& marker = m_snapshot[static_cast<std::size_t>(m_offSpaceActive)];
    const float scale = scaleFactor();

    std::string text = marker.label.empty()
        ? std::string("Target in another location")
        : marker.label + " — in another location";

    const TextStyle style = labelStyle();
    const Dimensions size = backend.measureText(text, style);
    const Point pos{
        m_bounds.x + (std::max)(4.0f * scale, (m_bounds.width - size.width) * 0.5f),
        m_bounds.y + m_bounds.height - size.height - 4.0f * scale,
    };

    backend.drawText(text, pos, TextStyle{ th.colors.textSecondary, style.fontSize, false, false });
}

// "Center on player" button: visible when camera is panned away from player.
void MapCanvas::drawRecenterButton(UIRenderBackend& backend) {
    if (m_followPlayer || !m_source) return;

    const auto& th = Theme::Current();
    const float scale = scaleFactor();
    const Rect b = recenterButtonRect();

    backend.drawRoundedRect(b, th.colors.surfaceActive, 6.0f * scale, th.colors.borderFocus, 1.2f * scale);

    // Target icon (ring + dot). Kept clean to avoid collision with edge arrows.
    const Point c{ b.x + b.width * 0.5f, b.y + b.height * 0.5f };
    backend.drawCircle(c, 6.5f * scale, Color::Transparent(), th.colors.textPrimary, 1.5f * scale);
    backend.drawCircle(c, 1.8f * scale, th.colors.textPrimary);
}

const MapCanvas::Projected* MapCanvas::hitMarker(const Point& localPoint) const {
    if (m_projected.empty()) return nullptr;

    const Point c = center();
    const float clickRadius = radiusPx() - 8.0f * scaleFactor();
    const float threshold = 14.0f * scaleFactor();
    const float thresholdSq = threshold * threshold;

    const Projected* best = nullptr;
    float bestDistSq = 0.0f;

    for (const Projected& pr : m_projected) {
        // Distant target edge indicator is clickable rather than its off-screen position.
        const Point target = pr.onEdge
            ? (m_mode == MapMode::World ? pr.screen : along(c, pr.screenAngleDeg, clickRadius))
            : pr.screen;
        const Point delta = target - localPoint;
        const float distSq = delta.x * delta.x + delta.y * delta.y;

        if (distSq <= thresholdSq && (!best || distSq < bestDistSq)) {
            best = &pr;
            bestDistSq = distSq;
        }
    }

    return best;
}

const MapCanvas::Cluster* MapCanvas::hitCluster(const Point& localPoint) const {
    const float threshold = (std::max)(8.0f, m_clusterPx * 0.5f) + 2.0f;
    const float thresholdSq = threshold * threshold;

    const Cluster* best = nullptr;
    float bestDistSq = 0.0f;

    for (const Cluster& cluster : m_clusters) {
        const Point delta = cluster.screen - localPoint;
        const float distSq = delta.x * delta.x + delta.y * delta.y;
        if (distSq <= thresholdSq && (!best || distSq < bestDistSq)) {
            best = &cluster;
            bestDistSq = distSq;
        }
    }

    return best;
}

bool MapCanvas::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    if (localPoint.x < 0.0f || localPoint.y < 0.0f ||
        localPoint.x > m_bounds.width || localPoint.y > m_bounds.height) {
        return false;
    }

    // Return true in Radar mode as well so UIContext routes onPointerUp to this element.
    if (m_mode == MapMode::World) {
        m_dragging = true;
        m_lastDrag = localPoint;
        m_dragTravel = Point{};
    }
    return true;
}

void MapCanvas::onPointerMove(const Point& localPoint) {
    if (!m_dragging) return;

    const Point delta = localPoint - m_lastDrag;
    m_lastDrag = localPoint;
    m_dragTravel = m_dragTravel + delta;

    // Manual pan disables auto-follow until "recenter" is pressed.
    if (m_followPlayer && std::fabs(m_dragTravel.x) + std::fabs(m_dragTravel.y) > kDragClickTolerancePx) {
        m_followPlayer = false;
    }

    if (m_followPlayer) return;
    panPixels(delta);
}

bool MapCanvas::onPointerUp(const Point& localPoint) {
    const bool wasDragging = m_dragging;
    m_dragging = false;
    UIElement::onPointerUp(localPoint);

    // Tap without movement is a marker click; dragging does not trigger click.
    if (wasDragging) {
        const float travel = std::fabs(m_dragTravel.x) + std::fabs(m_dragTravel.y);
        m_dragTravel = Point{};
        if (travel > kDragClickTolerancePx) return true;
    }

    // Projected positions are in viewport coords; adjust by bounds() for correct hit detection.
    const Point absolute = localPoint + m_bounds.topLeft();

    if (m_mode == MapMode::World) {
        if (!m_followPlayer && recenterButtonRect().contains(absolute)) {
            centerOnPlayer();
            return true;
        }
        if (const Cluster* cluster = hitCluster(absolute)) {
            // Clicking a cluster centers and zooms in for clarity.
            centerOn(screenToWorld(cluster->screen));
            zoomBy(0.5f);
            return true;
        }
    }

    const Projected* hit = hitMarker(absolute);
    if (!hit || !m_onMarkerClicked) return false;

    m_onMarkerClicked(m_snapshot[hit->marker]);
    return true;
}

bool MapCanvas::onMouseWheel(float delta, const Point& localPoint) {
    if (m_mode != MapMode::World || delta == 0.0f) return false;

    const float factor = delta > 0.0f ? 0.8f : 1.25f;   // zoom in / out
    const Point absolute = localPoint + m_bounds.topLeft();
    zoomBy(factor, &absolute);
    if (m_followPlayer) centerOnPlayer();
    return true;
}

bool MapCanvas::onAction(NavDirection dir) {
    if (m_mode != MapMode::World || !isVisible()) return false;

    // Stick navigation: camera pans in world space (up = north, right = east).
    const float stepMeters = 48.0f * m_metersPerPixel;
    switch (dir) {
    case NavDirection::Up:    moveCameraMeters(Point{ 0.0f, stepMeters }); break;
    case NavDirection::Down:  moveCameraMeters(Point{ 0.0f, -stepMeters }); break;
    case NavDirection::Left:  moveCameraMeters(Point{ -stepMeters, 0.0f }); break;
    case NavDirection::Right: moveCameraMeters(Point{ stepMeters, 0.0f }); break;
    }
    return true;
}

bool MapCanvas::onKeyDown(int keyCode) {
    // 0x08 = VK_BACK: gamepad back button maps to this code.
    if (keyCode != 0x08 || !m_onBack || m_mode != MapMode::World) return false;

    m_onBack();
    return true;
}

}  // namespace PerfUI
