#pragma once

#include "Types.h"
#include "Animation.h"
#include "UIElement.h"

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace PerfUI {

enum class MapMarkerKind {
    Quest,
    Location,
    Enemy,
    Custom
};

struct MapMarker {
    std::uint64_t id{ 0 };              // уникальный в пределах карты
    std::string label;
    MapMarkerKind kind{ MapMarkerKind::Custom };
    float x{ 0.0f };                    // метры, +X = восток
    float y{ 0.0f };                    // метры, +Y = север
    bool active{ false };               // текущая цель выбранного задания
    bool discovered{ true };
    bool sameSpace{ true };             // false: цель в другом пространстве — на карте не рисуется
    Color color{ Color::Transparent() }; // alpha == 0: цвет берётся из темы по kind
};

struct MapPlayerState {
    float x{ 0.0f };
    float y{ 0.0f };
    float headingDeg{ 0.0f };           // 0 = север, растёт по часовой стрелке
};

// Виджет не знает про игру: данные приходят через этот интерфейс.
// markers() должен возвращать снимок, валидный на время рендера кадра.
class IMapDataSource {
public:
    virtual ~IMapDataSource() = default;

    virtual MapPlayerState player() const = 0;
    virtual std::span<const MapMarker> markers() const = 0;
    virtual std::uint64_t revision() const = 0;  // меняется при любом изменении данных
};

// Готовый источник для тестов, примеров и одно-потоковых сценариев.
class PERFUI_API VectorMapDataSource final : public IMapDataSource {
public:
    MapPlayerState player() const override { return m_player; }
    std::span<const MapMarker> markers() const override { return m_markers; }
    std::uint64_t revision() const override { return m_revision; }

    void setPlayer(const MapPlayerState& state) {
        if (state.x == m_player.x && state.y == m_player.y && state.headingDeg == m_player.headingDeg) return;
        m_player = state;
        ++m_revision;
    }

    void setPlayer(float x, float y, float headingDeg = 0.0f) { setPlayer(MapPlayerState{ x, y, headingDeg }); }

    void setMarkers(std::vector<MapMarker> markers) {
        m_markers = std::move(markers);
        ++m_revision;
    }

    void addMarker(MapMarker marker) {
        m_markers.push_back(std::move(marker));
        ++m_revision;
    }

    void clearMarkers() {
        m_markers.clear();
        ++m_revision;
    }

private:
    MapPlayerState m_player;
    std::vector<MapMarker> m_markers;
    std::uint64_t m_revision{ 1 };
};

enum class MapMode {
    Radar,
    World
};

// Радар-карта (этап 1) и схематичная карта мира (этап 2).
// MapMode::World до этапа 2 рисуется как радар с rotateWithPlayer = false, то есть север вверх.
class PERFUI_API MapCanvas final : public UIElement {
public:
    explicit MapCanvas(std::string name = "MapCanvas");

    MapCanvas& setDataSource(IMapDataSource* src);   // не владеет; nullptr безопасен
    MapCanvas& mode(MapMode m);
    MapCanvas& radarRange(float meters);             // радиус радара, по умолчанию 300 м
    MapCanvas& showDistances(bool on);
    MapCanvas& showGrid(bool on);
    MapCanvas& rotateWithPlayer(bool on);            // только Radar, по умолчанию true
    MapCanvas& onMarkerClicked(std::function<void(const MapMarker&)> cb);

    IMapDataSource* dataSource() const { return m_source; }
    MapMode mode() const { return m_mode; }
    float radarRange() const { return m_radarRange; }
    bool showsDistances() const { return m_showDistances; }
    bool showsGrid() const { return m_showGrid; }
    bool rotatesWithPlayer() const { return m_rotateWithPlayer; }

    // Диагност для тестов и профилирования.
    std::size_t recomputeCount() const { return m_recomputeCount; }
    std::size_t projectedCount() const { return m_projected.size(); }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;
    bool onPointerUp(const Point& localPoint) override;

private:
    // Ссылок на объекты источника между кадрами не держим: маркеры копируются в m_snapshot,
    // а проекция указывает на них индексом.
    struct Projected {
        std::size_t marker{ 0 };
        Point screen{};
        float distanceMeters{ 0.0f };
        float bearingDeg{ 0.0f };      // азимут от севера, [0, 360)
        float screenAngleDeg{ 0.0f };  // угол от «верха экрана» по часовой
        bool onEdge{ false };          // дальше радиуса — рисуем стрелку на границе
    };

    struct CacheKey {
        const void* source{ nullptr };
        std::uint64_t revision{ 0 };
        float width{ 0.0f };
        float height{ 0.0f };
        MapMode mode{ MapMode::Radar };
        float radarRange{ 0.0f };
        bool rotateWithPlayer{ true };
        bool showGrid{ false };

        bool operator==(const CacheKey&) const = default;
    };

    [[nodiscard]] Point center() const;
    [[nodiscard]] float radiusPx() const;
    [[nodiscard]] float scaleFactor() const;
    [[nodiscard]] TextStyle labelStyle() const;
    [[nodiscard]] static Color colorFor(const MapMarker& marker);

    void recompute();
    void drawRings(UIRenderBackend& backend);
    void drawGrid(UIRenderBackend& backend);
    void drawPlayer(UIRenderBackend& backend);
    void drawMarkers(UIRenderBackend& backend);
    void drawActiveTarget(UIRenderBackend& backend);
    void drawOffSpaceBadge(UIRenderBackend& backend);

    IMapDataSource* m_source{ nullptr };
    MapMode m_mode{ MapMode::Radar };
    float m_radarRange{ 300.0f };
    bool m_showDistances{ true };
    bool m_showGrid{ false };
    bool m_rotateWithPlayer{ true };
    std::function<void(const MapMarker&)> m_onMarkerClicked;

    AnimatedFloat m_pulse{ 0.0f, 3.0f };
    float m_playerAngleDeg{ 0.0f };  // угол стрелки игрока на экране, от «верха» по часовой

    CacheKey m_key;
    std::vector<MapMarker> m_snapshot;
    std::vector<Projected> m_projected;
    long m_active{ -1 };          // индекс в m_projected
    long m_offSpaceActive{ -1 };  // индекс в m_snapshot
    std::size_t m_recomputeCount{ 0 };
};

}  // namespace PerfUI
