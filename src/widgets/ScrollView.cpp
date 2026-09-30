#include "PerfUI/ScrollView.h"
#include "PerfUI/LayoutEngine.h"
#include <algorithm>

namespace PerfUI {

ScrollView::ScrollView(std::string name)
    : UIElement(std::move(name))
{
}

ScrollView& ScrollView::scrollTo(float offset) {
    float maxScroll = (std::max)(0.0f, m_contentHeight - m_bounds.height);
    m_scrollOffset = (std::clamp)(offset, 0.0f, maxScroll);
    markLayoutDirty();
    return *this;
}

ScrollView& ScrollView::scrollBy(float delta) {
    return scrollTo(m_scrollOffset + delta);
}

void ScrollView::measure(Dimensions availableSize) {
    // Measure content with infinite vertical headroom so children get full desired height
    Dimensions unconstrained{ availableSize.width, 100000.0f };
    LayoutEngine::Measure(this, unconstrained);

    // Save total content height
    m_contentHeight = m_desiredSize.height;

    // Determine ScrollView's own desired height based on parent constraint or available space
    float desiredH = availableSize.height;
    if (layout().height().mode == SizeMode::Fixed) {
        desiredH = layout().height().value;
    } else if (layout().height().mode == SizeMode::Percent) {
        desiredH = availableSize.height * (layout().height().value / 100.0f);
    }

    float desiredW = availableSize.width;
    if (layout().width().mode == SizeMode::Fixed) {
        desiredW = layout().width().value;
    } else if (layout().width().mode == SizeMode::Percent) {
        desiredW = availableSize.width * (layout().width().value / 100.0f);
    }

    desiredW = (std::clamp)(desiredW, layout().minWidth(), layout().maxWidth());
    desiredH = (std::clamp)(desiredH, layout().minHeight(), layout().maxHeight());

    m_desiredSize = Dimensions{ desiredW, desiredH };
}

void ScrollView::arrange(const Rect& finalRect) {
    m_bounds = finalRect;
    clearLayoutDirty();

    float maxScroll = (std::max)(0.0f, m_contentHeight - finalRect.height);
    m_scrollOffset = (std::clamp)(m_scrollOffset, 0.0f, maxScroll);

    Rect contentRect = finalRect.inset(layout().padding());
    contentRect.y -= m_scrollOffset;
    contentRect.height = (std::max)(contentRect.height, m_contentHeight);

    LayoutEngine::ArrangeContent(this, contentRect);
}

UIElement* ScrollView::hitTest(const Point& point) {
    if (!isVisible() || !m_bounds.contains(point)) {
        return nullptr;
    }

    // Children bounds are offset by m_scrollOffset and positioned in screen space
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (auto* target = (*it)->hitTest(point)) {
            return target;
        }
    }

    return this;
}

bool ScrollView::onMouseWheel(float delta, const Point& localPoint) {
    (void)localPoint;
    float maxScroll = (std::max)(0.0f, m_contentHeight - m_bounds.height);
    if (maxScroll <= 0.0f) {
        return false; // Let parent handle it if no scrolling needed
    }

    m_scrollOffset -= delta * m_scrollSpeed;
    m_scrollOffset = (std::clamp)(m_scrollOffset, 0.0f, maxScroll);
    markLayoutDirty();
    return true;
}

void ScrollView::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    backend.pushClipRect(m_bounds);
    UIElement::render(backend);
    backend.popClipRect();

    // Render scrollbar if content exceeds viewport
    if (m_contentHeight > m_bounds.height && m_showScrollbar) {
        float trackX = m_bounds.x + m_bounds.width - m_scrollbarWidth - 2.0f;
        float trackY = m_bounds.y + 4.0f;
        float trackH = m_bounds.height - 8.0f;

        if (trackH > 0.0f) {
            backend.drawRoundedRect(
                Rect{ trackX, trackY, m_scrollbarWidth, trackH },
                m_trackColor,
                m_scrollbarWidth * 0.5f
            );

            float thumbH = (std::max)(16.0f, trackH * (m_bounds.height / m_contentHeight));
            float maxScroll = m_contentHeight - m_bounds.height;
            float scrollRatio = (maxScroll > 0.0f) ? (m_scrollOffset / maxScroll) : 0.0f;
            float thumbY = trackY + (trackH - thumbH) * scrollRatio;

            backend.drawRoundedRect(
                Rect{ trackX, thumbY, m_scrollbarWidth, thumbH },
                m_thumbColor,
                m_scrollbarWidth * 0.5f
            );
        }
    }
}

} // namespace PerfUI
