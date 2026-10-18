#include "PerfUI/Accordion.h"
#include "PerfUI/Panel.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

AccordionSection::AccordionSection(std::string title, std::unique_ptr<UIElement> content, bool expanded, std::string name)
    : UIElement(std::move(name))
    , m_title(std::move(title))
    , m_expanded(expanded)
{
    setFocusable(true);
    m_animProgress.snapTo(expanded ? 1.0f : 0.0f);
    if (content) {
        setContent(std::move(content));
    } else {
        setContent(std::make_unique<Panel>());
    }
}

AccordionSection& AccordionSection::title(std::string t) {
    m_title = std::move(t);
    markLayoutDirty();
    return *this;
}

void AccordionSection::setExpanded(bool expanded) {
    if (m_expanded != expanded) {
        m_expanded = expanded;
        m_animProgress.setTarget(expanded ? 1.0f : 0.0f);
        markLayoutDirty();

        if (expanded) {
            if (auto* acc = dynamic_cast<Accordion*>(m_parent)) {
                acc->notifySectionToggled(this);
            }
        }
    }
}

void AccordionSection::setContent(std::unique_ptr<UIElement> content) {
    if (m_content) {
        removeChild(m_content);
        m_content = nullptr;
    }
    if (content) {
        m_content = addChild(std::move(content));
    }
    markLayoutDirty();
}

void AccordionSection::update(float deltaTime) {
    UIElement::update(deltaTime);
    float oldVal = m_animProgress.value();
    m_animProgress.update(deltaTime);
    if (std::abs(oldVal - m_animProgress.value()) > 0.001f) {
        markLayoutDirty();
    }
}

void AccordionSection::measure(Dimensions availableSize) {
    m_contentDesiredHeight = 0.0f;
    float w = availableSize.width > 0.0f ? availableSize.width : 250.0f;

    if (m_content && m_content->isVisible()) {
        m_content->measure(Dimensions{ w, 0.0f });
        m_contentDesiredHeight = m_content->desiredSize().height;
    }

    float totalH = m_headerHeight + m_contentDesiredHeight * m_animProgress.value();
    m_desiredSize = Dimensions{ w, totalH };
}

void AccordionSection::arrange(const Rect& finalRect) {
    UIElement::arrange(finalRect);

    if (m_content) {
        Rect contentRect{
            finalRect.x,
            finalRect.y + m_headerHeight,
            finalRect.width,
            m_contentDesiredHeight
        };
        m_content->arrange(contentRect);
    }
}

void AccordionSection::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    WidgetState state = currentState();
    Rect headerRect{ m_bounds.x, m_bounds.y, m_bounds.width, m_headerHeight };

    Color headerBg = state == WidgetState::Hovered ? Color(35, 42, 54, 240) : Color(22, 27, 34, 230);
    Color border = isFocused() ? Theme::Current().colors.borderFocus : Theme::Current().colors.borderSubtle;
    float borderW = isFocused() ? 1.5f : 1.0f;

    // Header background
    backend.drawRoundedRect(headerRect, headerBg, 4.0f, border, borderW);

    // Indicator (Chevron / Arrow)
    std::string arrow = m_animProgress.value() > 0.5f ? "v" : ">";
    TextStyle arrowStyle;
    arrowStyle.color = isFocused() ? Theme::Current().colors.borderFocus : Theme::Current().colors.textSecondary;
    arrowStyle.fontSize = 12.0f;
    arrowStyle.bold = true;
    backend.drawText(arrow, Point{ headerRect.x + 10.0f, headerRect.y + (headerRect.height - 12.0f * 1.25f) * 0.5f }, arrowStyle);

    // Header title
    if (!m_title.empty()) {
        TextStyle titleStyle;
        titleStyle.color = isFocused() ? Theme::Current().colors.borderFocus : Theme::Current().colors.textPrimary;
        titleStyle.fontSize = 14.0f;
        titleStyle.bold = true;
        backend.drawText(m_title, Point{ headerRect.x + 28.0f, headerRect.y + (headerRect.height - 14.0f * 1.25f) * 0.5f }, titleStyle);
    }

    // Render content with scissor clipping if expanding/expanded
    if (m_content && m_animProgress.value() > 0.01f) {
        Rect clipRect{
            m_bounds.x,
            m_bounds.y + m_headerHeight,
            m_bounds.width,
            m_contentDesiredHeight * m_animProgress.value()
        };

        backend.pushClipRect(clipRect);
        m_content->render(backend);
        backend.popClipRect();
    }
}

bool AccordionSection::onPointerDown(const Point& localPoint) {
    if (localPoint.y <= m_headerHeight) {
        UIElement::onPointerDown(localPoint);
        setExpanded(!m_expanded);
        return true;
    }
    return UIElement::onPointerDown(localPoint);
}

bool AccordionSection::onAction(NavDirection dir) {
    (void)dir;
    setExpanded(!m_expanded);
    return true;
}

// ---------------------------------------------------------------------------
// Accordion
// ---------------------------------------------------------------------------
Accordion::Accordion(std::string name)
    : UIElement(std::move(name))
{
    layout().direction(LayoutDirection::Vertical);
    layout().gap(4.0f);
}

AccordionSection* Accordion::addSection(const std::string& title, std::unique_ptr<UIElement> content, bool expanded) {
    return add<AccordionSection>(title, std::move(content), expanded);
}

void Accordion::notifySectionToggled(AccordionSection* section) {
    if (!m_exclusive || !section || !section->isExpanded()) return;

    for (const auto& child : children()) {
        if (auto* s = dynamic_cast<AccordionSection*>(child.get())) {
            if (s != section && s->isExpanded()) {
                s->setExpanded(false);
            }
        }
    }
}

} // namespace PerfUI
