#pragma once

#include "Types.h"
#include "UIElement.h"
#include <unordered_map>
#include <memory>
#include <cmath>

namespace PerfUI {

enum class AnchorAlignment {
    Center,
    BottomCenter,
    TopCenter,
    TopLeft
};

struct WorldAnchorPoint {
    uint32_t targetId{ 0 };
    Point screenPos{ 0.0f, 0.0f };   // In screen pixels (0..width, 0..height)
    float depth{ 0.0f };            // Normalized camera depth (0..1)
    float distance{ 0.0f };         // Distance from camera in world units
    bool inFrontOfCamera{ true };   // True if in front of camera plane
    bool inFrustum{ true };         // True if within screen boundaries
    bool hasLineOfSight{ true };    // True if unobstructed (LoS raycast)
    bool isValid{ true };           // True if target entity still exists
};

struct WorldAnchorSnapshot {
    std::unordered_map<uint32_t, WorldAnchorPoint> points;
    float screenWidth{ 1920.0f };
    float screenHeight{ 1080.0f };
    uint64_t frameNumber{ 0 };
};

class PERFUI_API WorldAnchor : public UIElement {
public:
    WorldAnchor();
    explicit WorldAnchor(uint32_t targetId);
    ~WorldAnchor() override = default;

    // Target Binding
    void setTargetId(uint32_t id) { m_targetId = id; }
    uint32_t targetId() const { return m_targetId; }

    // Direct manual override (useful for testing or custom script driving)
    void setScreenData(const Point& screenPos, float distance, bool inFront, bool inFrustum = true, bool hasLoS = true, bool isValid = true);
    void clearManualScreenData() { m_manualData = false; }

    // Alignment
    void setAnchorAlignment(AnchorAlignment alignment) { m_alignment = alignment; }
    AnchorAlignment anchorAlignment() const { return m_alignment; }

    // Screen Offset (in pixels relative to projected anchor point)
    void setScreenOffset(const Point& offset) { m_screenOffset = offset; }
    Point screenOffset() const { return m_screenOffset; }

    // Distance-Based Scaling
    void setDistanceScaling(float minDist, float maxDist, float minScale = 0.5f, float maxScale = 1.0f, bool enabled = true);
    bool isDistanceScalingEnabled() const { return m_distanceScalingEnabled; }
    float currentScale() const { return m_currentScale; }

    // Distance-Based Alpha (Fade)
    void setDistanceAlpha(float minDist, float maxDist, float minAlpha = 0.0f, float maxAlpha = 1.0f, bool enabled = true);
    bool isDistanceAlphaEnabled() const { return m_distanceAlphaEnabled; }

    // Behind Camera Hiding
    void setHideBehindCamera(bool hide) { m_hideBehindCamera = hide; }
    bool hideBehindCamera() const { return m_hideBehindCamera; }

    // Occlusion Hiding (Line-of-Sight)
    void setHideWhenOccluded(bool hide) { m_hideWhenOccluded = hide; }
    bool hideWhenOccluded() const { return m_hideWhenOccluded; }

    // Screen Edge Clamping
    void setEdgeClamping(bool enabled, float screenMargin = 30.0f);
    bool isEdgeClampingEnabled() const { return m_edgeClampingEnabled; }
    float screenMargin() const { return m_screenMargin; }

    // Clamping status & pointer angle (radians, 0 = right, PI/2 = down)
    bool isClamped() const { return m_isClamped; }
    float clampedAngle() const { return m_clampedAngle; }

    // Base dimensions for scaling
    void setBaseSize(Dimensions size);
    Dimensions baseSize() const { return m_baseSize; }

    // UIElement overrides
    void update(float deltaTime) override;

private:
    void applyProjection(const WorldAnchorPoint& pt, const Dimensions& viewport);

    uint32_t m_targetId{ 0 };
    AnchorAlignment m_alignment{ AnchorAlignment::BottomCenter };
    Point m_screenOffset{ 0.0f, 0.0f };

    bool m_manualData{ false };
    WorldAnchorPoint m_manualPoint;

    // Distance Scaling
    bool m_distanceScalingEnabled{ false };
    float m_minScaleDist{ 200.0f };
    float m_maxScaleDist{ 2000.0f };
    float m_minScale{ 0.5f };
    float m_maxScale{ 1.0f };
    float m_currentScale{ 1.0f };

    // Distance Alpha
    bool m_distanceAlphaEnabled{ false };
    float m_minAlphaDist{ 1500.0f };
    float m_maxAlphaDist{ 3000.0f };
    float m_minAlpha{ 0.0f };
    float m_maxAlpha{ 1.0f };

    // Behind Camera & Occlusion
    bool m_hideBehindCamera{ true };
    bool m_hideWhenOccluded{ false };

    // Edge Clamping
    bool m_edgeClampingEnabled{ false };
    float m_screenMargin{ 30.0f };
    bool m_isClamped{ false };
    float m_clampedAngle{ 0.0f };

    Dimensions m_baseSize{ 100.0f, 40.0f };
};

} // namespace PerfUI
