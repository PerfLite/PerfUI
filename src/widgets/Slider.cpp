#include "PerfUI/Slider.h"
#include "PerfUI/UIContext.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace PerfUI {

Slider::Slider(float value, float minVal, float maxVal, std::string label, std::string name)
    : UIElement(std::move(name))
    , m_minValue((std::min)(minVal, maxVal))
    , m_maxValue((std::max)(minVal, maxVal))
    , m_value((std::clamp)(value, (std::min)(minVal, maxVal), (std::max)(minVal, maxVal)))
    , m_label(std::move(label))
{
    setFocusable(true);
}

Slider& Slider::value(float v) {
    float lo = (std::min)(m_minValue, m_maxValue);
    float hi = (std::max)(m_minValue, m_maxValue);
    float clamped = (std::clamp)(v, lo, hi);
    if (m_value != clamped) {
        m_value = clamped;
        markLayoutDirty();
        if (m_onValueChanged) {
            m_onValueChanged(m_value);
        }
    }
    return *this;
}

Slider& Slider::range(float minVal, float maxVal) {
    m_minValue = (std::min)(minVal, maxVal);
    m_maxValue = (std::max)(minVal, maxVal);
    m_value = (std::clamp)(m_value, m_minValue, m_maxValue);
    markLayoutDirty();
    return *this;
}

Slider& Slider::label(std::string_view text) {
    if (m_label != text) {
        m_label = text;
        markLayoutDirty();
    }
    return *this;
}

void Slider::measure(Dimensions availableSize) {
    float w = availableSize.width;
    float h = (std::max)(22.0f, m_thumbRadius * 2.0f + 6.0f);

    if (!m_label.empty()) {
        h += 18.0f; // Extra room for label
    }

    const auto& pad = layout().padding();
    h += pad.vertical();

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    }
    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    }

    w = (std::clamp)(w, layout().minWidth(), layout().maxWidth());
    h = (std::clamp)(h, layout().minHeight(), layout().maxHeight());

    m_desiredSize = Dimensions{ w, h };
}

void Slider::updateValueFromX(float localX) {
    float trackPadding = m_thumbRadius + layout().padding().left;
    float trackWidth = m_bounds.width - (m_thumbRadius * 2.0f) - layout().padding().horizontal();

    if (trackWidth <= 0.0f) return;

    float ratio = (localX - trackPadding) / trackWidth;
    ratio = (std::clamp)(ratio, 0.0f, 1.0f);

    float newVal = m_minValue + ratio * (m_maxValue - m_minValue);

    if (m_step > 0.0001f) {
        newVal = std::round((newVal - m_minValue) / m_step) * m_step + m_minValue;
    }

    value(newVal);
}

bool Slider::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    m_dragging = true;
    updateValueFromX(localPoint.x);
    return true;
}

bool Slider::onPointerUp(const Point& localPoint) {
    UIElement::onPointerUp(localPoint);
    m_dragging = false;
    return true;
}

void Slider::onPointerMove(const Point& localPoint) {
    UIElement::onPointerMove(localPoint);
    if (m_dragging) {
        updateValueFromX(localPoint.x);
    }
}

void Slider::update(float deltaTime) {
    UIElement::update(deltaTime);
    // Dragging handled through mouse position if active
}

bool Slider::onAction(NavDirection dir) {
    float delta = (m_step > 0.0f) ? m_step : ((m_maxValue - m_minValue) * 0.05f);
    if (dir == NavDirection::Left) {
        value(m_value - delta);
        return true;
    } else if (dir == NavDirection::Right) {
        value(m_value + delta);
        return true;
    }
    return false;
}

void Slider::render(UIRenderBackend& backend) {
    if (!isVisible() || m_bounds.width <= 0.0f) return;

    const auto& pad = layout().padding();
    float currentY = m_bounds.y + pad.top;

    // Draw label if present
    if (!m_label.empty()) {
        char valBuf[32];
        if (m_step >= 1.0f) {
            std::snprintf(valBuf, sizeof(valBuf), "%d", static_cast<int>(m_value));
        } else {
            std::snprintf(valBuf, sizeof(valBuf), "%.2f", m_value);
        }

        std::string fullText = m_label + ": " + valBuf;
        backend.drawText(fullText, Point{ m_bounds.x + pad.left, currentY }, m_labelStyle);
        currentY += 18.0f;
    }

    float trackX = m_bounds.x + pad.left + m_thumbRadius;
    float trackW = (std::max)(10.0f, m_bounds.width - pad.horizontal() - m_thumbRadius * 2.0f);
    float remainingHeight = (std::max)(m_thumbRadius * 2.0f, (m_bounds.y + m_bounds.height - pad.bottom) - currentY);
    float trackCenterY = currentY + remainingHeight * 0.5f;
    float trackY = trackCenterY - m_trackHeight * 0.5f;

    Rect trackRect{ trackX, trackY, trackW, m_trackHeight };
    backend.drawRoundedRect(trackRect, m_trackColor, m_trackHeight * 0.5f, m_trackBorder, 1.0f);

    // Calculate ratio
    float range = m_maxValue - m_minValue;
    float ratio = (range > 0.0001f) ? (m_value - m_minValue) / range : 0.0f;
    ratio = (std::clamp)(ratio, 0.0f, 1.0f);

    // Filled portion
    if (ratio > 0.001f) {
        Rect fillRect{ trackX, trackY, trackW * ratio, m_trackHeight };
        backend.drawRoundedRect(fillRect, m_fillColor, m_trackHeight * 0.5f);
    }

    // Thumb position
    float thumbCenterX = trackX + trackW * ratio;
    float thumbCenterY = trackCenterY;
    Rect thumbRect{
        thumbCenterX - m_thumbRadius,
        thumbCenterY - m_thumbRadius,
        m_thumbRadius * 2.0f,
        m_thumbRadius * 2.0f
    };

    Color thumbBorder = (m_hovered || m_dragging || isFocused()) ? Color::BorderFocus() : Color::BorderSubtle();
    if (m_hovered || m_dragging) {
        backend.drawShadow(thumbRect, m_thumbRadius, Color::FocusGlow(), 8.0f, { 0.0f, 1.0f });
    }

    backend.drawRoundedRect(thumbRect, m_thumbColor, m_thumbRadius, thumbBorder, 1.5f);

    UIElement::render(backend);
}

} // namespace PerfUI
