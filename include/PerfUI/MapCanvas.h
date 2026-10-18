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
    std::uint64_t id{ 0 };              // Unique within the map
    std::string label;
    MapMarkerKind kind{ MapMarkerKind::Custom };
    float x{ 0.0f };                    // Meters, +X = East
    float y{ 0.0f };                    // Meters, +Y = North
    bool active{ false };               // Active objective of selected quest
    bool discovered{ true };
    bool sameSpace{ true };             // false: target is in another space - not rendered on map
    Color color{ Color::Transparent() }; // alpha == 0: color is taken from theme by kind
};

struct MapPlayerState {
    float x{ 0.0f };
    float y{ 0.0f };
    float headingDeg{ 0.0f };           // 0 = north, clockwise
};

// The widget is game-agnostic: data arrives through this interface.
// markers() must return a snapshot valid during the frame render.
class IMapDataSource {
public:
    virtual ~IMapDataSource() = default;

    virtual MapPlayerState player() const = 0;
    virtual std::span<const MapMarker> markers() const = 0;
    virtual std::uint64_t revision() const = 0;  // increments on any data change
};

// Ready-to-use data source for tests, samples, and single-threaded scenarios.
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

// Radar map and world map canvas.
// MapMode::World renders north-up with full camera pan and zoom.
class PERFUI_API MapCanvas final : public UIElement {
public:
    explicit MapCanvas(std::string name = "MapCanvas");

    MapCanvas& setDataSource(IMapDataSource* src);   // non-owning; nullptr safe
    MapCanvas& mode(MapMode m);
    MapCanvas& radarRange(float meters);             // radar range, default 300m
    MapCanvas& showDistances(bool on);
    MapCanvas& showGrid(bool on);
    MapCanvas& rotateWithPlayer(bool on);            // Radar only, default true
    MapCanvas& onMarkerClicked(std::function<void(const MapMarker&)> cb);

    // --- World mode, camera, clusters ---
    // All coordinate values are in meters matching MapMarker coordinates.

    // Camera bounding box. Clamps panning to prevent moving into void.
    MapCanvas& clampToBounds(float minX, float minY, float maxX, float maxY);
    MapCanvas& clearBounds();
    bool hasBounds() const { return m_hasBounds; }

    MapCanvas& metersPerPixel(float meters);       // zoom: meters per pixel
    float metersPerPixel() const { return m_metersPerPixel; }
    MapCanvas& zoomLimits(float minMetersPerPixel, float maxMetersPerPixel);

    MapCanvas& cameraCenter(Point meters);         // camera center in meters
    Point cameraCenter() const { return m_camera; }

    MapCanvas& followPlayer(bool on);              // camera follows player while enabled
    bool isFollowingPlayer() const { return m_followPlayer; }

    // Markers closer than this pixel distance are grouped into clusters. 0 = disabled. World only.
    MapCanvas& clusterPixelSize(float pixels);
    float clusterPixelSize() const { return m_clusterPx; }

    MapCanvas& onBackground(std::function<void()> cb);   // gamepad back / Esc callback

    void centerOnPlayer();
    void centerOn(Point meters);
    void zoomBy(float factor, const Point* anchorLocalPx = nullptr);   // factor > 1 zooms out
    void panPixels(Point deltaPx);
    void panMeters(Point deltaMeters);

    IMapDataSource* dataSource() const { return m_source; }
    MapMode mode() const { return m_mode; }
    float radarRange() const { return m_radarRange; }
    bool showsDistances() const { return m_showDistances; }
    bool showsGrid() const { return m_showGrid; }
    bool rotatesWithPlayer() const { return m_rotateWithPlayer; }

    // Diagnostics for tests and profiling.
    std::size_t recomputeCount() const { return m_recomputeCount; }
    std::size_t projectedCount() const { return m_projected.size(); }
    std::size_t clusterCount() const { return m_clusters.size(); }
    float gridStepMeters() const { return m_gridStep; }   // 0 = no grid

    // World: rect of "center on player" button in viewport coordinates.
    // Public for host and tests to hit-test accurately.
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
    // No external source pointers retained across frames: markers copied to m_snapshot,
    // projections reference them by index.
    struct Projected {
        std::size_t marker{ 0 };
        Point screen{};
        float distanceMeters{ 0.0f };
        float bearingDeg{ 0.0f };      // bearing from north, [0, 360)
        float screenAngleDeg{ 0.0f };  // angle clockwise from screen top
        bool onEdge{ false };          // beyond radius: drawn as arrow on edge
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

        // Point/Rect lack operator==; compare fields manually.
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
        std::size_t marker{ 0 };   // representative for label and click
    };

    [[nodiscard]] Point center() const;
    [[nodiscard]] float radiusPx() const;
    [[nodiscard]] float scaleFactor() const;
    [[nodiscard]] TextStyle labelStyle() const;
    [[nodiscard]] static Color colorFor(const MapMarker& marker);

    [[nodiscard]] Rect viewRect() const;                  // World: active viewport within border
    [[nodiscard]] Point worldToScreen(Point meters) const;
    [[nodiscard]] Point screenToWorld(Point pixels) const;
    [[nodiscard]] float pickGridStep() const;             // meters; 0 = no grid
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
    // Both hit-tests expect viewport coordinates (matching Projected::screen).
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

    // World mode camera: center in meters and meters-per-pixel.
    Point m_camera{};
    float m_metersPerPixel{ 2.0f };
    float m_minMetersPerPixel{ 0.25f };
    float m_maxMetersPerPixel{ 200.0f };
    bool m_followPlayer{ true };
    bool m_hasBounds{ false };
    Rect m_boundsMeters{};                    // x,y = min; width,height = area size in meters
    float m_clusterPx{ 26.0f };
    float m_gridStep{ 0.0f };

    bool m_dragging{ false };
    Point m_lastDrag{};
    Point m_dragTravel{};
    static constexpr float kDragClickTolerancePx = 6.0f;

    AnimatedFloat m_pulse{ 0.0f, 3.0f };
    float m_playerAngleDeg{ 0.0f };  // player screen arrow angle, clockwise from top

    CacheKey m_key;
    const void* m_snapshotSource{ nullptr };
    std::uint64_t m_snapshotRevision{ 0 };
    std::vector<MapMarker> m_snapshot;
    std::vector<Projected> m_projected;
    std::vector<Cluster> m_clusters;
    long m_active{ -1 };          // index in m_projected
    long m_offSpaceActive{ -1 };  // index in m_snapshot
    std::size_t m_recomputeCount{ 0 };
};

}  // namespace PerfUI
