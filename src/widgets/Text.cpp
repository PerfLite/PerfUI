#include "PerfUI/Text.h"
#include "PerfUI/UIContext.h"
#include <algorithm>

namespace PerfUI {

Text::Text(std::string text, std::string name)
    : UIElement(std::move(name))
    , m_text(std::move(text))
{
}

Text& Text::text(std::string_view str) {
    if (m_text != str) {
        m_text = str;
        markLayoutDirty();
    }
    return *this;
}

void Text::measure(Dimensions availableSize) {
    (void)availableSize;

    Dimensions measured{ 0.0f, 0.0f };

    if (m_context && m_context->renderBackend() && !m_text.empty()) {
        measured = m_context->renderBackend()->measureText(m_text, m_style);
    } else if (!m_text.empty()) {
        // Fallback heuristic if not yet attached to renderer
        measured.width = static_cast<float>(m_text.length()) * (m_style.fontSize * 0.58f);
        measured.height = m_style.fontSize * 1.25f;
    }

    // Apply explicit dimension constraints if specified
    float w = measured.width;
    float h = measured.height;

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

void Text::render(UIRenderBackend& backend) {
    if (!isVisible() || m_text.empty()) return;

    // In case measure was done before backend was attached
    if (m_desiredSize.width <= 0.0f || m_desiredSize.height <= 0.0f) {
        m_desiredSize = backend.measureText(m_text, m_style);
    }

    float posX = m_bounds.x;
    if (m_bounds.width > m_desiredSize.width) {
        if (m_textAlign == Alignment::Center) {
            posX = m_bounds.x + (m_bounds.width - m_desiredSize.width) * 0.5f;
        } else if (m_textAlign == Alignment::End) {
            posX = m_bounds.x + m_bounds.width - m_desiredSize.width;
        }
    }

    float posY = m_bounds.y;
    if (m_bounds.height > m_desiredSize.height) {
        posY = m_bounds.y + (m_bounds.height - m_desiredSize.height) * 0.5f;
    }

    backend.drawText(m_text, Point{ posX, posY }, m_style);
}

} // namespace PerfUI
