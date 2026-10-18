#pragma once

#include "PerfUI/Layout.h"
#include "PerfUI/MapCanvas.h"
#include "PerfUI/Panel.h"
#include "PerfUI/Text.h"
#include "PerfUI/Theme.h"
#include "PerfUI/Toast.h"
#include "PerfUI/UIContext.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace PerfUI::Examples {

/**
 * @brief MapCanvas demo: player walks in a circle with thirteen markers around.
 *
 * Data is provided via VectorMapDataSource - exactly as a mod would feed from the game,
 * except values are updated by simulation rather than the game engine. No Skyrim dependencies.
 */
class MapDemoHud final : public Panel {
public:
    explicit MapDemoHud(std::string name = "MapDemoHud")
        : Panel(std::move(name))
    {
        setFocusable(true);
        backgroundColor(Color(14, 18, 26, 225));
        borderColor(Color::BorderSubtle());
        borderWidth(1.2f);
        cornerRadius(10.0f);
        shadow(true, Color(0, 0, 0, 180), 18.0f, { 0.0f, 6.0f });
        layout().direction(LayoutDirection::Vertical).padding(14.0f, 12.0f).gap(8.0f);

        m_title = add<Text>("MapCanvas — Radar Mode");
        m_title->color(Color::NordicGold()).fontSize(16.0f).bold(true);
        m_title->layout().width(kBlockWidth);

        m_map = add<MapCanvas>("DemoMap");
        m_map->layout().width(kMapSize).height(kMapSize);
        m_map->setDataSource(&m_source)
              .mode(MapMode::Radar)
              .radarRange(kStartRange)
              .showDistances(true)
              .showGrid(false)
              .rotateWithPlayer(true)
              .metersPerPixel(2.0f)
              .clusterPixelSize(26.0f)
              .zoomLimits(0.25f, 200.0f)
              .clampToBounds(-400.0f, -400.0f, 400.0f, 900.0f)
              .followPlayer(true);
        m_map->onMarkerClicked([this](const MapMarker& marker) { selectMarker(marker.id); });
        m_map->onBackground([this]() {
            if (auto* ctx = context()) {
                ctx->showToast("MapCanvas", "Exit: Back action forwarded to host", ToastType::Info, 1.5f);
            }
        });

        // Fixed height: measure() on wrapped text yields a single line, and without it
        // multiline status and hint would overflow the panel bottom boundary.
        m_status = add<Text>("");
        m_status->color(Color(200, 205, 215, 255)).fontSize(13.0f).wrap(true);
        m_status->layout().width(kBlockWidth).height(36.0f);

        m_hint = add<Text>("M mode · R rotate · G grid · D distances · T target · +/− range/zoom · "
                           "C on player · F follow · arrows - camera · click marker - select");
        m_hint->color(Color(130, 140, 155, 255)).fontSize(11.5f).wrap(true);
        m_hint->layout().width(kBlockWidth).height(54.0f);

        seedMarkers();
        m_source.setPlayer(0.0f, 0.0f, 0.0f);
        refreshStatus();
    }

    void update(float deltaTime) override {
        Panel::update(deltaTime);
        if (!isVisible()) return;

        // Player walks along a circle of radius kWalkRadius around origin.
        m_walkPhase += deltaTime * kWalkSpeedRadPerSec;
        const float cx = kWalkRadius * std::cos(m_walkPhase);
        const float cy = kWalkRadius * std::sin(m_walkPhase);
        // Velocity is tangent: d/dt (R cos t, R sin t) = (-sin t, cos t).
        const float vx = -std::sin(m_walkPhase);
        const float vy = std::cos(m_walkPhase);
        m_source.setPlayer(cx, cy, normalizeDeg(std::atan2(vx, vy) * kRadToDeg));

        m_statusTimer += deltaTime;
        if (m_statusTimer >= 0.25f) {
            m_statusTimer = 0.0f;
            refreshStatus();
        }
    }

    bool onKeyDown(int keyCode) override {        if (!isVisible()) return false;

        switch (keyCode) {
        case 'M':
            m_map->mode(m_map->mode() == MapMode::Radar ? MapMode::World : MapMode::Radar);
            break;
        case 'R':
            m_map->rotateWithPlayer(!m_map->rotatesWithPlayer());
            break;
        case 'G':
            m_map->showGrid(!m_map->showsGrid());
            break;
        case 'D':
            m_map->showDistances(!m_map->showsDistances());
            break;
        case 'T':
            cycleActive();
            break;
        // Codes are virtual keycodes (WM_KEYDOWN wParam) to avoid pulling windows.h into demo.
        case 0xBB:  // VK_OEM_PLUS: "=" and "+"
        case 0x6B:  // VK_ADD
            plusKey();
            break;
        case 0xBD:  // VK_OEM_MINUS: "-"
        case 0x6D:  // VK_SUBTRACT
            minusKey();
            break;
        case 'C':
            m_map->centerOnPlayer();
            break;
        case 'F':
            m_map->followPlayer(!m_map->isFollowingPlayer());
            break;
        // 37..40 - arrows: in World mode they pan camera like right stick.
        case 37: m_map->onAction(NavDirection::Left); break;
        case 38: m_map->onAction(NavDirection::Up); break;
        case 39: m_map->onAction(NavDirection::Right); break;
        case 40: m_map->onAction(NavDirection::Down); break;
        default:
            return false;
        }
        refreshStatus();
        return true;
    }

    MapCanvas* map() const { return m_map; }

    // Self-contained screen position: docked to top-right viewport corner.
    void arrange(const Rect& finalRect) override {
        const float w = desiredSize().width;
        const float h = desiredSize().height;
        float x = 24.0f;
        float y = 24.0f;
        if (finalRect.width > 0.0f) {
            x = (std::max)(24.0f, finalRect.width - w - 24.0f);
            y = (std::max)(24.0f, (finalRect.height - h) * 0.5f);
        }
        Panel::arrange(Rect{ x, y, w, h });
    }

private:
    static constexpr float kMapSize = 300.0f;
    static constexpr float kBlockWidth = kMapSize;
    static constexpr float kStartRange = 300.0f;
    static constexpr float kWalkRadius = 140.0f;
    static constexpr float kWalkSpeedRadPerSec = 0.45f;
    static constexpr float kRadToDeg = 57.29577951308232f;

    static float normalizeDeg(float deg) {
        float result = std::fmod(deg, 360.0f);
        if (result < 0.0f) result += 360.0f;
        return result;
    }

    void seedMarkers() {
        struct Seed {
            std::uint64_t id;
            const char* label;
            MapMarkerKind kind;
            float x, y;
        };

        static constexpr Seed seeds[] = {
            { 1, "River Crossing", MapMarkerKind::Location, 90.0f, 60.0f },
            { 2, "Bandit Camp", MapMarkerKind::Enemy, -130.0f, 40.0f },
            { 3, "Old Mine", MapMarkerKind::Location, 40.0f, -170.0f },
            { 4, "Dragon Mound", MapMarkerKind::Enemy, 200.0f, -120.0f },
            { 5, "Windy Farm", MapMarkerKind::Location, -210.0f, -90.0f },
            { 6, "Temple of Meridia", MapMarkerKind::Location, -60.0f, 220.0f },
            { 7, "Sheltered House", MapMarkerKind::Custom, 150.0f, 180.0f },
            { 8, "Hunter Camp", MapMarkerKind::Custom, -250.0f, 150.0f },
            { 9, "Towerbloom", MapMarkerKind::Location, 260.0f, 20.0f },
            { 10, "Heart Temple", MapMarkerKind::Quest, 40.0f, 780.0f },
            // Dense cluster around 'River Crossing': used to test clustering at distance.
            { 11, "Campfire", MapMarkerKind::Enemy, 96.0f, 66.0f },
            { 12, "Bear Cave", MapMarkerKind::Location, 104.0f, 58.0f },
            { 13, "Hunter Rest", MapMarkerKind::Custom, 98.0f, 52.0f },
        };

        m_markers.clear();
        m_markers.reserve(std::size(seeds));
        for (const auto& seed : seeds) {
            MapMarker marker;
            marker.id = seed.id;
            marker.label = seed.label;
            marker.kind = seed.kind;
            marker.x = seed.x;
            marker.y = seed.y;
            marker.active = seed.id == kStartActiveId;
            m_markers.push_back(std::move(marker));
        }
        m_source.setMarkers(m_markers);
    }

    void selectMarker(std::uint64_t id) {
        bool changed = false;
        for (auto& marker : m_markers) {
            const bool wantActive = marker.id == id;
            if (marker.active != wantActive) {
                marker.active = wantActive;
                changed = true;
            }
        }
        if (!changed) return;

        m_source.setMarkers(m_markers);
        const auto it = std::find_if(m_markers.begin(), m_markers.end(),
                                     [id](const MapMarker& m) { return m.id == id; });
        if (it != m_markers.end()) {
            const auto player = m_source.player();
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s · %.0f m", it->label.c_str(),
                          std::sqrt(square(it->x - player.x) + square(it->y - player.y)));
            m_selection = buf;
        }
        if (auto* ctx = context()) {
            ctx->showToast("Target selected", m_selection, ToastType::Info, 2.0f);
        }
        refreshStatus();
    }

    void cycleActive() {
        std::size_t next = 0;
        for (std::size_t i = 0; i < m_markers.size(); ++i) {
            if (m_markers[i].active) {
                next = (i + 1) % m_markers.size();
                break;
            }
        }
        selectMarker(m_markers[next].id);
    }

    // "+": in World - zoom in (fewer meters per pixel), in Radar - larger view range.
    void plusKey() {
        if (m_map->mode() == MapMode::World) {
            m_map->zoomBy(0.75f);
            return;
        }
        m_map->radarRange(std::clamp(m_map->radarRange() * 1.5f, 40.0f, 1500.0f));
    }

    void minusKey() {
        if (m_map->mode() == MapMode::World) {
            m_map->zoomBy(1.0f / 0.75f);
            return;
        }
        m_map->radarRange(std::clamp(m_map->radarRange() / 1.5f, 40.0f, 1500.0f));
    }

    void refreshStatus() {
        if (!m_map || !m_status) return;
        const auto player = m_source.player();

        char detail[96];
        if (m_map->mode() == MapMode::World) {
            std::snprintf(detail, sizeof(detail), "scale %.2f m/px · grid %.0f m · clusters %zu · %s",
                          m_map->metersPerPixel(),
                          m_map->gridStepMeters(),
                          m_map->clusterCount(),
                          m_map->isFollowingPlayer() ? "following" : "free camera");
        } else {
            std::snprintf(detail, sizeof(detail), "range %.0f m · heading %.0f°",
                          m_map->radarRange(), player.headingDeg);
        }

        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "%s · %s · targets %zu/%zu · recalculations %llu%s",
                      m_map->mode() == MapMode::Radar
                          ? (m_map->rotatesWithPlayer() ? "Radar" : "Radar, North up")
                          : "World",
                      detail,
                      m_map->projectedCount(),
                      m_markers.size(),
                      static_cast<unsigned long long>(m_map->recomputeCount()),
                      m_selection.empty() ? "" : (" · " + m_selection).c_str());
        m_status->text(buf);

        if (m_title) {
            m_title->text(m_map->mode() == MapMode::World
                              ? "MapCanvas — World Map"
                              : "MapCanvas — Radar Mode");
        }
    }

    static float square(float v) { return v * v; }

    VectorMapDataSource m_source;
    std::vector<MapMarker> m_markers;
    MapCanvas* m_map{ nullptr };
    Text* m_title{ nullptr };
    Text* m_status{ nullptr };
    Text* m_hint{ nullptr };
    float m_walkPhase{ 0.0f };
    float m_statusTimer{ 0.0f };
    std::string m_selection;
    static constexpr std::uint64_t kStartActiveId = 10;
};

}  // namespace PerfUI::Examples
