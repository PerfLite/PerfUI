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

    // --- Этап 2: World-режим, камера, кластеры ---
    // Все значения координат — в метрах той же системы, что и MapMarker.

    // Область, в которую можно водить камеру. Выход за неё отрезается,
    // чтобы нельзя было утащить карту в пустоту.
    MapCanvas& clampToBounds(float minX, float minY, float maxX, float maxY);
    MapCanvas& clearBounds();
    bool hasBounds() const { return m_hasBounds; }

    MapCanvas& metersPerPixel(float meters);       // зум: сколько метров в одном пикселе
    float metersPerPixel() const { return m_metersPerPixel; }
    MapCanvas& zoomLimits(float minMetersPerPixel, float maxMetersPerPixel);

    MapCanvas& cameraCenter(Point meters);         // центр окна камеры, метры
    Point cameraCenter() const { return m_camera; }

    MapCanvas& followPlayer(bool on);              // камера ездит за игроком, пока включено
    bool isFollowingPlayer() const { return m_followPlayer; }

    // Маркеры, чьи экранные позиции ближе этого расстояния, рисуются одним значком с числом.
    // 0 — кластеризация выключена. Только World.
    MapCanvas& clusterPixelSize(float pixels);
    float clusterPixelSize() const { return m_clusterPx; }

    MapCanvas& onBackground(std::function<void()> cb);   // «назад» с геймпада / Esc

    void centerOnPlayer();
    void centerOn(Point meters);
    void zoomBy(float factor, const Point* anchorLocalPx = nullptr);   // factor > 1 — отдаляем
    void panPixels(Point deltaPx);
    void panMeters(Point deltaMeters);

    IMapDataSource* dataSource() const { return m_source; }
    MapMode mode() const { return m_mode; }
    float radarRange() const { return m_radarRange; }
    bool showsDistances() const { return m_showDistances; }
    bool showsGrid() const { return m_showGrid; }
    bool rotatesWithPlayer() const { return m_rotateWithPlayer; }

    // Диагност для тестов и профилирования.
    std::size_t recomputeCount() const { return m_recomputeCount; }
    std::size_t projectedCount() const { return m_projected.size(); }
    std::size_t clusterCount() const { return m_clusters.size(); }
    float gridStepMeters() const { return m_gridStep; }   // 0 — не рисуем сетку

    // World: зона кнопки «центрировать на игроке» в координатах вьюпорта.
    // Публично: чтобы хост и тесты могли попасть по кнопке, не угадывая её размеры.
    [[nodiscard]] Rect recenterButtonRect() const;

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;
    bool onPointerDown(const Point& localPoint) override;
    bool onPointerUp(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;
    bool onMouseWheel(float delta, const Point& localPoint) override;
    bool onAction(NavDirection dir) override;
    bool onKeyDown(int keyCode) override;

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
        Point camera{};
        float metersPerPixel{ 1.0f };
        float clusterPx{ 0.0f };
        Rect boundsMeters{};

        // Point/Rect не имеют operator==, поэтому сравнение по полям вручную.
        bool operator==(const CacheKey& o) const {
            return source == o.source && revision == o.revision &&
                   width == o.width && height == o.height && mode == o.mode &&
                   radarRange == o.radarRange && rotateWithPlayer == o.rotateWithPlayer &&
                   showGrid == o.showGrid && camera.x == o.camera.x && camera.y == o.camera.y &&
                   metersPerPixel == o.metersPerPixel && clusterPx == o.clusterPx &&
                   boundsMeters.x == o.boundsMeters.x && boundsMeters.y == o.boundsMeters.y &&
                   boundsMeters.width == o.boundsMeters.width && boundsMeters.height == o.boundsMeters.height;
        }
    };

    struct Cluster {
        Point screen{};
        std::size_t count{ 0 };
        std::size_t marker{ 0 };   // представитель для подписи и клика
    };

    [[nodiscard]] Point center() const;
    [[nodiscard]] float radiusPx() const;
    [[nodiscard]] float scaleFactor() const;
    [[nodiscard]] TextStyle labelStyle() const;
    [[nodiscard]] static Color colorFor(const MapMarker& marker);

    [[nodiscard]] Rect viewRect() const;                  // World: рабочая область внутри рамки
    [[nodiscard]] Point worldToScreen(Point meters) const;
    [[nodiscard]] Point screenToWorld(Point pixels) const;
    [[nodiscard]] float pickGridStep() const;             // метры; 0 — сетку не рисуем
    void clampCamera();
    void moveCameraMeters(Point deltaMeters);
    static Point clampRayToRect(Point from, Point to, const Rect& rect);

    void refreshSnapshot();
    void recompute();
    void recomputeRadar();
    void recomputeWorld();
    void drawRings(UIRenderBackend& backend);
    void drawGrid(UIRenderBackend& backend);
    void drawWorldGrid(UIRenderBackend& backend);
    void drawPlayer(UIRenderBackend& backend);
    void drawMarkers(UIRenderBackend& backend);
    void drawClusters(UIRenderBackend& backend);
    void drawLabels(UIRenderBackend& backend);
    void drawActiveTarget(UIRenderBackend& backend);
    void drawActiveLink(UIRenderBackend& backend);
    void drawOffSpaceBadge(UIRenderBackend& backend);
    void drawRecenterButton(UIRenderBackend& backend);
    // Оба хит-теста ждут точку в координатах вьюпорта (как и Projected::screen),
    // а не локальную точку события — смещение делает вызывающий.
    [[nodiscard]] const Projected* hitMarker(const Point& viewportPoint) const;
    [[nodiscard]] const Cluster* hitCluster(const Point& viewportPoint) const;

    IMapDataSource* m_source{ nullptr };
    MapMode m_mode{ MapMode::Radar };
    float m_radarRange{ 300.0f };
    bool m_showDistances{ true };
    bool m_showGrid{ false };
    bool m_rotateWithPlayer{ true };
    std::function<void(const MapMarker&)> m_onMarkerClicked;
    std::function<void()> m_onBack;

    // Камера World-режима: центр в метрах и «метров на пиксель».
    Point m_camera{};
    float m_metersPerPixel{ 2.0f };
    float m_minMetersPerPixel{ 0.25f };
    float m_maxMetersPerPixel{ 200.0f };
    bool m_followPlayer{ true };
    bool m_hasBounds{ false };
    Rect m_boundsMeters{};                    // x,y = min; width,height = размер области в метрах
    float m_clusterPx{ 26.0f };
    float m_gridStep{ 0.0f };

    bool m_dragging{ false };
    Point m_lastDrag{};
    Point m_dragTravel{};
    static constexpr float kDragClickTolerancePx = 6.0f;

    AnimatedFloat m_pulse{ 0.0f, 3.0f };
    float m_playerAngleDeg{ 0.0f };  // угол стрелки игрока на экране, от «верха» по часовой

    CacheKey m_key;
    const void* m_snapshotSource{ nullptr };
    std::uint64_t m_snapshotRevision{ 0 };
    std::vector<MapMarker> m_snapshot;
    std::vector<Projected> m_projected;
    std::vector<Cluster> m_clusters;
    long m_active{ -1 };          // индекс в m_projected
    long m_offSpaceActive{ -1 };  // индекс в m_snapshot
    std::size_t m_recomputeCount{ 0 };
};

}  // namespace PerfUI
