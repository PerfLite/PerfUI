#include "PerfUI/SegmentedControl.h"
#include "PerfUI/Theme.h"
#include "PerfUI/UIRenderBackend.h"
#include <algorithm>

namespace PerfUI {

SegmentedControl::SegmentedControl(std::string name)
    : UIElement(std::move(name)) {
    m_layout.height(32.0f);
}

SegmentedControl& SegmentedControl::addSegment(std::string text, std::string value) {
    if (value.empty()) {
        value = text;
    }
    m_segments.push_back({ std::move(text), std::move(value), {} });
    calculateThumbTarget();
    return *this;
}

SegmentedControl& SegmentedControl::clearSegments() {
    m_segments.clear();
    m_selectedIndex = 0;
    m_hoveredIndex = -1;
    m_initializedThumb = false;
    return *this;
}

void SegmentedControl::setSelectedIndex(size_t index) {
    if (m_segments.empty()) return;
    if (index >= m_segments.size()) index = m_segments.size() - 1;

    if (m_selectedIndex != index) {
        m_selectedIndex = index;
        calculateThumbTarget();
        if (m_onSelectionChanged) {
            m_onSelectionChanged(m_selectedIndex, m_segments[m_selectedIndex].text);
        }
    }
}

std::string SegmentedControl::selectedText() const {
    if (m_selectedIndex < m_segments.size()) {
        return m_segments[m_selectedIndex].text;
    }
    return {};
}

std::string SegmentedControl::selectedValue() const {
    if (m_selectedIndex < m_segments.size()) {
        return m_segments[m_selectedIndex].value;
    }
    return {};
}

void SegmentedControl::calculateThumbTarget() {
    if (m_segments.empty() || m_selectedIndex >= m_segments.size()) return;

    const auto& seg = m_segments[m_selectedIndex];
    float targetX = seg.bounds.x - m_bounds.x + 2.0f;
    float targetWidth = std::max(4.0f, seg.bounds.width - 4.0f);

    if (!m_initializedThumb && seg.bounds.width > 0.0f) {
        m_thumbX.snapTo(targetX);
        m_thumbWidth.snapTo(targetWidth);
        m_initializedThumb = true;
    } else {
        m_thumbX.setTarget(targetX);
        m_thumbWidth.setTarget(targetWidth);
    }
}

void SegmentedControl::update(float deltaTime) {
    UIElement::update(deltaTime);
    m_thumbX.update(deltaTime);
    m_thumbWidth.update(deltaTime);
}

void SegmentedControl::measure(Dimensions availableSize) {
    (void)availableSize;
    float minWidth = 40.0f;
    if (!m_segments.empty()) {
        minWidth = 0.0f;
        for (const auto& s : m_segments) {
            float textW = static_cast<float>(s.text.length()) * 8.0f + 24.0f;
            minWidth += std::max(textW, 48.0f);
        }
    }

    float finalW = (m_layout.width().mode == SizeMode::Fixed && m_layout.width().value > 0.0f)
        ? m_layout.width().value : minWidth;
    float finalH = (m_layout.height().mode == SizeMode::Fixed && m_layout.height().value > 0.0f)
        ? m_layout.height().value : 32.0f;

    m_desiredSize = { finalW, finalH };
}

void SegmentedControl::arrange(const Rect& finalRect) {
    UIElement::arrange(finalRect);

    if (m_segments.empty()) return;

    float segW = finalRect.width / static_cast<float>(m_segments.size());
    for (size_t i = 0; i < m_segments.size(); ++i) {
        m_segments[i].bounds = Rect{
            finalRect.x + static_cast<float>(i) * segW,
            finalRect.y,
            segW,
            finalRect.height
        };
    }

    calculateThumbTarget();
}

void SegmentedControl::render(UIRenderBackend& backend) {
    if (!m_visible || m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) return;

    const auto& theme = Theme::Current();
    float radius = theme.metrics.radiusSmall > 0.0f ? theme.metrics.radiusSmall : 4.0f;

    // Track background
    Color bg = (m_style.box.normal && m_style.box.normal->backgroundColor.has_value())
        ? m_style.box.normal->backgroundColor.value()
        : theme.colors.surfaceBase;
    Color border = theme.colors.borderSubtle;
    backend.drawRoundedRect(m_bounds, bg, radius, border, 1.0f);

    if (m_segments.empty()) return;

    // Sliding thumb
    if (m_thumbWidth.value() > 0.0f) {
        Rect thumbRect{
            m_bounds.x + m_thumbX.value(),
            m_bounds.y + 2.0f,
            m_thumbWidth.value(),
            m_bounds.height - 4.0f
        };
        Color thumbColor = theme.colors.surfaceElevated;
        backend.drawRoundedRect(thumbRect, thumbColor, std::max(1.0f, radius - 1.0f), theme.colors.borderFocus, 1.0f);
    }

    // Dividers and segment texts
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const auto& seg = m_segments[i];

        // Draw divider before segment (except first and except adjacent to selected)
        if (i > 0 && i != m_selectedIndex && (i - 1) != m_selectedIndex) {
            backend.drawLine(
                Point{ seg.bounds.x, seg.bounds.y + 6.0f },
                Point{ seg.bounds.x, seg.bounds.y + seg.bounds.height - 6.0f },
                theme.colors.borderSubtle,
                1.0f
            );
        }

        // Hover highlight if not selected
        if (static_cast<int>(i) == m_hoveredIndex && i != m_selectedIndex) {
            Rect hoverRect{ seg.bounds.x + 2.0f, seg.bounds.y + 2.0f, seg.bounds.width - 4.0f, seg.bounds.height - 4.0f };
            backend.drawRoundedRect(hoverRect, theme.colors.surfaceActive, radius - 1.0f);
        }

        // Segment text
        Color textColor = (i == m_selectedIndex) ? theme.colors.textPrimary : theme.colors.textSecondary;
        if (static_cast<int>(i) == m_hoveredIndex && i != m_selectedIndex) {
            textColor = theme.colors.textPrimary;
        }

        TextStyle tstyle;
        tstyle.fontSize = 13.0f;
        tstyle.color = textColor;
        tstyle.bold = (i == m_selectedIndex);

        float textW = static_cast<float>(seg.text.length()) * 7.5f;
        float textX = seg.bounds.x + (seg.bounds.width - textW) * 0.5f;
        float textY = seg.bounds.y + (seg.bounds.height - 14.0f) * 0.5f;

        backend.drawText(seg.text, Point{ textX, textY }, tstyle);
    }
}

bool SegmentedControl::onPointerDown(const Point& localPoint) {
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const auto& seg = m_segments[i];
        if (localPoint.x >= (seg.bounds.x - m_bounds.x) &&
            localPoint.x <= (seg.bounds.x - m_bounds.x + seg.bounds.width) &&
            localPoint.y >= 0.0f && localPoint.y <= m_bounds.height) {
            setSelectedIndex(i);
            return true;
        }
    }
    return false;
}

void SegmentedControl::onPointerMove(const Point& localPoint) {
    m_hoveredIndex = -1;
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const auto& seg = m_segments[i];
        if (localPoint.x >= (seg.bounds.x - m_bounds.x) &&
            localPoint.x <= (seg.bounds.x - m_bounds.x + seg.bounds.width) &&
            localPoint.y >= 0.0f && localPoint.y <= m_bounds.height) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }
}

void SegmentedControl::onPointerLeave() {
    m_hoveredIndex = -1;
}

} // namespace PerfUI
