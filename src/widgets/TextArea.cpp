#include "PerfUI/TextArea.h"
#include "PerfUI/Theme.h"
#include <sstream>
#include <algorithm>

namespace PerfUI {

TextArea::TextArea(std::string text, std::string name)
    : UIElement(std::move(name))
    , m_text(std::move(text))
{
    setFocusable(true);
}

TextArea& TextArea::text(std::string t) {
    if (m_text != t) {
        m_text = std::move(t);
        markLayoutDirty();
    }
    return *this;
}

void TextArea::measure(Dimensions availableSize) {
    float w = availableSize.width > 0.0f ? availableSize.width : 250.0f;
    float h = availableSize.height > 0.0f ? availableSize.height : 150.0f;

    if (layout().width().mode == SizeMode::Fixed) w = layout().width().value;
    if (layout().height().mode == SizeMode::Fixed) h = layout().height().value;

    m_desiredSize = Dimensions{ w, h };
}

void TextArea::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    WidgetState state = currentState();
    BoxStyle resolvedBox = resolveBoxStyle(state);

    Color bg = resolvedBox.backgroundColor.value_or(Color(18, 22, 28, 235));
    Color border = isFocused() ? Theme::Current().colors.borderFocus : resolvedBox.borderColor.value_or(Theme::Current().colors.borderSubtle);
    float radius = resolvedBox.cornerRadius.value_or(6.0f);
    float borderW = isFocused() ? 1.5f : 1.0f;

    // Draw background
    backend.drawRoundedRect(m_bounds, bg, radius, border, borderW);

    // Render multiline text with clipping
    Rect innerRect = m_bounds.inset(Insets{ 8.0f });
    backend.pushClipRect(innerRect);

    float lineHeight = m_textStyle.fontSize * 1.35f;
    float curY = innerRect.y - m_scrollOffset;

    std::stringstream ss(m_text);
    std::string line;
    float totalH = 0.0f;

    while (std::getline(ss, line)) {
        if (curY + lineHeight >= innerRect.y && curY <= innerRect.y + innerRect.height) {
            backend.drawText(line, Point{ innerRect.x, curY }, m_textStyle);
        }
        curY += lineHeight;
        totalH += lineHeight;
    }

    m_contentHeight = totalH;

    backend.popClipRect();

    // Scrollbar if content exceeds innerRect.height
    if (m_contentHeight > innerRect.height) {
        float scrollbarW = 4.0f;
        float barX = m_bounds.x + m_bounds.width - scrollbarW - 3.0f;
        float ratio = innerRect.height / m_contentHeight;
        float thumbH = (std::max)(16.0f, innerRect.height * ratio);
        float maxScroll = m_contentHeight - innerRect.height;
        float thumbY = innerRect.y + (m_scrollOffset / maxScroll) * (innerRect.height - thumbH);

        Rect trackRect{ barX, innerRect.y, scrollbarW, innerRect.height };
        Rect thumbRect{ barX, thumbY, scrollbarW, thumbH };

        backend.drawRect(trackRect, Color(255, 255, 255, 20));
        backend.drawRoundedRect(thumbRect, Theme::Current().colors.borderFocus, 2.0f);
    }

    UIElement::render(backend);
}

bool TextArea::onMouseWheel(float delta, const Point& localPoint) {
    (void)localPoint;
    float maxScroll = (std::max)(0.0f, m_contentHeight - (m_bounds.height - 16.0f));
    m_scrollOffset -= delta * 24.0f;
    m_scrollOffset = (std::clamp)(m_scrollOffset, 0.0f, maxScroll);
    return true;
}

} // namespace PerfUI
