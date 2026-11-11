#include "PerfUI/Text.h"
#include "PerfUI/UIContext.h"
#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace {
std::mutex g_textBindingMutex;
std::unordered_map<const PerfUI::Text*, std::string> g_textBindings;
}

namespace PerfUI {

Text::Text(std::string text, std::string name)
    : UIElement(std::move(name))
    , m_text(std::move(text))
{
}

Text::~Text() {
    std::lock_guard lock(g_textBindingMutex);
    g_textBindings.erase(this);
}

std::string Text::bindingKey() const {
    std::lock_guard lock(g_textBindingMutex);
    auto it = g_textBindings.find(this);
    return it == g_textBindings.end() ? std::string{} : it->second;
}

Text& Text::bindingKey(std::string key) {
    std::lock_guard lock(g_textBindingMutex);
    g_textBindings[this] = std::move(key);
    return *this;
}

Text& Text::text(std::string_view str) {
    if (m_text != str) {
        m_text = str;
        markLayoutDirty();
    }
    return *this;
}

void Text::measure(Dimensions availableSize) {
    Dimensions measured{ 0.0f, 0.0f };

    TextStyle measureStyle = m_style;
    if (m_wrap && availableSize.width > 0.0f && availableSize.width < 50000.0f) {
        measureStyle.wrapWidth = availableSize.width;
    }

    if (m_context && m_context->renderBackend() && !m_text.empty()) {
        measured = m_context->renderBackend()->measureText(m_text, measureStyle);
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

    float effAlpha = effectiveOpacity();
    if (effAlpha <= 0.001f) return;

    TextStyle renderStyle = m_style;
    renderStyle.color = Color(renderStyle.color.r, renderStyle.color.g, renderStyle.color.b,
                              static_cast<uint8_t>(std::clamp(static_cast<float>(renderStyle.color.a) * effAlpha, 0.0f, 255.0f)));
    if (m_wrap && m_bounds.width > 0.0f) {
        renderStyle.wrapWidth = m_bounds.width;
    }

    // In case measure was done before backend was attached
    if (m_desiredSize.width <= 0.0f || m_desiredSize.height <= 0.0f) {
        m_desiredSize = backend.measureText(m_text, renderStyle);
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
    if (m_bounds.height > m_desiredSize.height && !m_wrap) {
        posY = m_bounds.y + (m_bounds.height - m_desiredSize.height) * 0.5f;
    }

    backend.drawText(m_text, Point{ posX, posY }, renderStyle);
}

void Text::onStyleApplied(const ComputedStyle& style) {
    if (style.color.has_value()) m_style.color = *style.color;
    if (style.fontSize.has_value()) {
        m_style.fontSize = *style.fontSize;
        markLayoutDirty();
    }
    if (style.bold.has_value()) {
        m_style.bold = *style.bold;
        markLayoutDirty();
    }
    if (style.italic.has_value()) {
        m_style.italic = *style.italic;
        markLayoutDirty();
    }
    if (style.wrap.has_value()) {
        m_wrap = *style.wrap;
        markLayoutDirty();
    }
}

} // namespace PerfUI
