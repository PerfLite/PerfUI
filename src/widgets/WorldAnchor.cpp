#include "PerfUI/WorldAnchor.h"
#include "PerfUI/UIContext.h"
#include <algorithm>
#include <cmath>

namespace PerfUI {

WorldAnchor::WorldAnchor() {
    setSize(m_baseSize);
}

WorldAnchor::WorldAnchor(uint32_t targetId) : m_targetId(targetId) {
    setSize(m_baseSize);
}

void WorldAnchor::setBaseSize(Dimensions size) {
    m_baseSize = size;
    if (!m_distanceScalingEnabled) {
        setSize(size);
    }
}

void WorldAnchor::setScreenData(const Point& screenPos, float distance, bool inFront, bool inFrustum, bool hasLoS, bool isValid) {
    m_manualData = true;
    m_manualPoint.targetId = m_targetId;
    m_manualPoint.screenPos = screenPos;
    m_manualPoint.distance = distance;
    m_manualPoint.inFrontOfCamera = inFront;
    m_manualPoint.inFrustum = inFrustum;
    m_manualPoint.hasLineOfSight = hasLoS;
    m_manualPoint.isValid = isValid;
}

void WorldAnchor::setDistanceScaling(float minDist, float maxDist, float minScale, float maxScale, bool enabled) {
    m_distanceScalingEnabled = enabled;
    m_minScaleDist = minDist;
    m_maxScaleDist = maxDist;
    m_minScale = minScale;
    m_maxScale = maxScale;
}

void WorldAnchor::setDistanceAlpha(float minDist, float maxDist, float minAlpha, float maxAlpha, bool enabled) {
    m_distanceAlphaEnabled = enabled;
    m_minAlphaDist = minDist;
    m_maxAlphaDist = maxDist;
    m_minAlpha = minAlpha;
    m_maxAlpha = maxAlpha;
}

void WorldAnchor::setEdgeClamping(bool enabled, float screenMargin) {
    m_edgeClampingEnabled = enabled;
    m_screenMargin = screenMargin;
}

void WorldAnchor::update(float deltaTime) {
    if (m_manualData) {
        Dimensions vp = (context() && context()->viewportSize().width > 0.0f) ?
            context()->viewportSize() : Dimensions{ 1920.0f, 1080.0f };
        applyProjection(m_manualPoint, vp);
    } else if (context()) {
        const auto* pt = context()->getWorldAnchorPoint(m_targetId);
        if (pt) {
            applyProjection(*pt, context()->viewportSize());
        } else {
            setVisible(false);
        }
    }

    UIElement::update(deltaTime);
}

void WorldAnchor::applyProjection(const WorldAnchorPoint& pt, const Dimensions& viewport) {
    if (!pt.isValid) {
        setVisible(false);
        return;
    }

    if (m_hideWhenOccluded && !pt.hasLineOfSight) {
        setVisible(false);
        return;
    }

    if (!m_edgeClampingEnabled && m_hideBehindCamera && !pt.inFrontOfCamera) {
        setVisible(false);
        return;
    }

    setVisible(true);

    // Distance Scaling
    if (m_distanceScalingEnabled && m_maxScaleDist > m_minScaleDist) {
        float t = (pt.distance - m_minScaleDist) / (m_maxScaleDist - m_minScaleDist);
        t = std::clamp(t, 0.0f, 1.0f);
        m_currentScale = m_maxScale + t * (m_minScale - m_maxScale);
        setSize({ m_baseSize.width * m_currentScale, m_baseSize.height * m_currentScale });
    } else {
        m_currentScale = 1.0f;
        setSize(m_baseSize);
    }

    // Distance Alpha
    if (m_distanceAlphaEnabled && m_maxAlphaDist > m_minAlphaDist) {
        float t = (pt.distance - m_minAlphaDist) / (m_maxAlphaDist - m_minAlphaDist);
        t = std::clamp(t, 0.0f, 1.0f);
        float a = m_maxAlpha + t * (m_minAlpha - m_maxAlpha);
        setOpacity(std::clamp(a, 0.0f, 1.0f));
    }

    // Viewport bounds
    float vw = viewport.width > 0.0f ? viewport.width : 1920.0f;
    float vh = viewport.height > 0.0f ? viewport.height : 1080.0f;
    float cx = vw * 0.5f;
    float cy = vh * 0.5f;

    float margin = std::max(0.0f, m_screenMargin);
    float minX = margin;
    float maxX = vw - margin;
    float minY = margin;
    float maxY = vh - margin;

    Point targetScreen = pt.screenPos;
    bool isInsideBox = (targetScreen.x >= minX && targetScreen.x <= maxX &&
                        targetScreen.y >= minY && targetScreen.y <= maxY);

    Point anchorScreen = targetScreen;

    if (!pt.inFrontOfCamera || !isInsideBox) {
        if (m_edgeClampingEnabled) {
            m_isClamped = true;

            float dirX = targetScreen.x - cx;
            float dirY = targetScreen.y - cy;

            // Invert projected vector if point is behind camera plane
            if (!pt.inFrontOfCamera) {
                dirX = -dirX;
                dirY = -dirY;
            }

            if (std::abs(dirX) < 1e-5f && std::abs(dirY) < 1e-5f) {
                dirY = 1.0f;
            }

            m_clampedAngle = std::atan2(dirY, dirX);

            float halfW = std::max(1.0f, (maxX - minX) * 0.5f);
            float halfH = std::max(1.0f, (maxY - minY) * 0.5f);

            float tx = (std::abs(dirX) > 1e-5f) ? (halfW / std::abs(dirX)) : 1e9f;
            float ty = (std::abs(dirY) > 1e-5f) ? (halfH / std::abs(dirY)) : 1e9f;
            float t = std::min(tx, ty);

            anchorScreen.x = std::clamp(cx + dirX * t, minX, maxX);
            anchorScreen.y = std::clamp(cy + dirY * t, minY, maxY);
        } else {
            m_isClamped = false;
            m_clampedAngle = 0.0f;
            if (m_hideBehindCamera && !pt.inFrontOfCamera) {
                setVisible(false);
                return;
            }
        }
    } else {
        m_isClamped = false;
        m_clampedAngle = 0.0f;
    }

    Point finalPoint = anchorScreen + m_screenOffset;
    float w = m_bounds.width;
    float h = m_bounds.height;
    Point elemPos = finalPoint;

    switch (m_alignment) {
    case AnchorAlignment::Center:
        elemPos = { finalPoint.x - w * 0.5f, finalPoint.y - h * 0.5f };
        break;
    case AnchorAlignment::BottomCenter:
        elemPos = { finalPoint.x - w * 0.5f, finalPoint.y - h };
        break;
    case AnchorAlignment::TopCenter:
        elemPos = { finalPoint.x - w * 0.5f, finalPoint.y };
        break;
    case AnchorAlignment::TopLeft:
        elemPos = finalPoint;
        break;
    }

    setPosition(elemPos);
}

} // namespace PerfUI
