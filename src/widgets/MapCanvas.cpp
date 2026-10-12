#include "PerfUI/MapCanvas.h"

#include "PerfUI/Theme.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

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
    std::snprintf(buf, size, "%d м", static_cast<int>(std::lroundf(meters)));
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

void MapCanvas::update(float deltaTime) {
    UIElement::update(deltaTime);

    // Пульсация «пинг-понг»: тик к 1, по достижении — обратно к 0.
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

void MapCanvas::recompute() {
    m_snapshot.clear();
    m_projected.clear();
    m_active = -1;
    m_offSpaceActive = -1;

    if (!m_source) return;

    const std::span<const MapMarker> markers = m_source->markers();
    m_snapshot.assign(markers.begin(), markers.end());

    const MapPlayerState player = m_source->player();
    const float radius = radiusPx();
    if (radius <= 0.0f || m_radarRange <= 0.0f) return;

    const bool rotate = (m_mode == MapMode::Radar) && m_rotateWithPlayer;
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
    };

    if (!(key == m_key)) {
        m_key = key;
        recompute();
        ++m_recomputeCount;
    }

    const auto& th = Theme::Current();
    const float radius = radiusPx();

    backend.pushClipRect(m_bounds);

    if (radius > 0.0f) {
        backend.drawRoundedRect(m_bounds, th.colors.surfaceBase, radius);

        if (m_showGrid) drawGrid(backend);
        drawRings(backend);
        drawActiveTarget(backend);
        drawMarkers(backend);
        drawPlayer(backend);
        drawOffSpaceBadge(backend);

        backend.drawCircle(center(), radius, Color::Transparent(), th.colors.borderStrong, 1.5f * scaleFactor());
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

void MapCanvas::drawPlayer(UIRenderBackend& backend) {
    const auto& th = Theme::Current();
    const Point c = center();
    const float scale = scaleFactor();
    const float size = 9.0f * scale;

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
            const Point tip{ c.x + dir.x * (radius - 2.0f * scale), c.y + dir.y * (radius - 2.0f * scale) };
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

    // Подписи: сначала ближайшие к игроку; перекрывающиеся пропускаем, чтобы не выводить кашу.
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
    char distBuf[32];

    for (const std::size_t i : order) {
        if (placed.size() >= kMaxLabels) break;

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
        ? std::string("Цель в другом месте")
        : marker.label + " — в другом месте";

    const TextStyle style = labelStyle();
    const Dimensions size = backend.measureText(text, style);
    const Point pos{
        m_bounds.x + (std::max)(4.0f * scale, (m_bounds.width - size.width) * 0.5f),
        m_bounds.y + m_bounds.height - size.height - 4.0f * scale,
    };

    backend.drawText(text, pos, TextStyle{ th.colors.textSecondary, style.fontSize, false, false });
}

bool MapCanvas::onPointerUp(const Point& localPoint) {
    if (!m_onMarkerClicked || m_projected.empty()) return false;

    const Point c = center();
    const float clickRadius = radiusPx() - 8.0f * scaleFactor();
    const float threshold = 14.0f * scaleFactor();
    const float thresholdSq = threshold * threshold;

    const Projected* best = nullptr;
    float bestDistSq = 0.0f;

    for (const Projected& pr : m_projected) {
        // У дальней цели кликабельна стрелка на границе, а не её реальная позиция «за горизонтом».
        const Point target = pr.onEdge ? along(c, pr.screenAngleDeg, clickRadius) : pr.screen;
        const Point delta = target - localPoint;
        const float distSq = delta.x * delta.x + delta.y * delta.y;

        if (distSq <= thresholdSq && (!best || distSq < bestDistSq)) {
            best = &pr;
            bestDistSq = distSq;
        }
    }

    if (!best) return false;

    m_onMarkerClicked(m_snapshot[best->marker]);
    return true;
}

}  // namespace PerfUI
