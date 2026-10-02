#include "PerfUI/ComboBox.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

ComboBox::ComboBox(std::vector<std::string> options, size_t initialIndex, std::string name)
    : UIElement(std::move(name))
    , m_options(std::move(options))
    , m_selectedIndex(initialIndex)
{
    setFocusable(true);
}

ComboBox::~ComboBox() {
    if (m_context && m_isOpen) {
        m_context->clearActiveComboBox(this);
    }
}

ComboBox& ComboBox::options(std::vector<std::string> opts) {
    m_options = std::move(opts);
    if (m_selectedIndex >= m_options.size()) {
        m_selectedIndex = 0;
    }
    markLayoutDirty();
    return *this;
}

const std::string& ComboBox::selectedText() const {
    static const std::string s_empty = "";
    if (m_selectedIndex < m_options.size()) {
        return m_options[m_selectedIndex];
    }
    return s_empty;
}

ComboBox& ComboBox::selectIndex(size_t index) {
    if (index < m_options.size()) {
        m_selectedIndex = index;
        if (m_onSelectionChanged) {
            m_onSelectionChanged(index, selectedText());
        }
        markLayoutDirty();
    }
    return *this;
}

void ComboBox::setOpen(bool open) {
    if (m_isOpen != open) {
        m_isOpen = open;
        m_openAnim.setTarget(open ? 1.0f : 0.0f);
        m_hoveredIndex = -1;

        if (m_context) {
            if (m_isOpen) {
                m_context->setActiveComboBox(this);
            } else {
                m_context->clearActiveComboBox(this);
            }
        }
    }
}

void ComboBox::update(float deltaTime) {
    UIElement::update(deltaTime);

    bool isHovered = (currentState() == WidgetState::Hovered || currentState() == WidgetState::Pressed);
    m_hoverAnim.setTarget(isHovered ? 1.0f : 0.0f);
    m_hoverAnim.update(deltaTime);
    m_openAnim.update(deltaTime);
}

void ComboBox::measure(Dimensions availableSize) {
    (void)availableSize;

    float maxTextW = 60.0f;
    if (m_context && m_context->renderBackend()) {
        for (const auto& opt : m_options) {
            auto dim = m_context->renderBackend()->measureText(opt, m_textStyle);
            maxTextW = (std::max)(maxTextW, dim.width);
        }
    }

    float padH = 28.0f; // includes room for chevron ▼
    float w = maxTextW + padH;
    float h = 28.0f;

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

void ComboBox::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    float t = m_hoverAnim.value();
    Color bg = Color::Lerp(Color(20, 26, 36, 230), Color(32, 42, 58, 250), t);
    Color border = Color::Lerp(Color::BorderSubtle(), Color::BorderFocus(), (std::max)(t, m_isOpen ? 1.0f : 0.0f));

    if (t > 0.01f || m_isOpen) {
        Color glow = Color::FocusGlow();
        backend.drawShadow(m_bounds, m_cornerRadius, glow, 6.0f, { 0.0f, 1.0f });
    }

    backend.drawRoundedRect(m_bounds, bg, m_cornerRadius, border, 1.0f);

    // Selected text
    const std::string& text = selectedText();
    if (!text.empty()) {
        float textX = m_bounds.x + 10.0f;
        Dimensions textDim = backend.measureText(text, m_textStyle);
        float textY = m_bounds.y + (std::max)(0.0f, (m_bounds.height - textDim.height) * 0.5f);

        TextStyle style = m_textStyle;
        style.color = (t > 0.01f || m_isOpen) ? Color::White() : Color::TextPrimary();
        backend.drawText(text, Point{ textX, textY }, style);
    }

    // Chevron down/up icon
    TextStyle chevronStyle = m_textStyle;
    chevronStyle.color = Color::NordicGold();
    chevronStyle.fontSize = 10.0f;
    const char* chevron = m_isOpen ? "^" : "v";
    Dimensions chevDim = backend.measureText(chevron, chevronStyle);
    float chevX = m_bounds.x + m_bounds.width - chevDim.width - 10.0f;
    float chevY = m_bounds.y + (std::max)(0.0f, (m_bounds.height - chevDim.height) * 0.5f);
    backend.drawText(chevron, Point{ chevX, chevY }, chevronStyle);
}

Rect ComboBox::dropdownBounds() const {
    if (m_options.empty()) return Rect{ 0.0f, 0.0f, 0.0f, 0.0f };

    float itemH = 26.0f;
    float popupW = (std::max)(m_bounds.width, 140.0f);
    float popupH = static_cast<float>(m_options.size()) * itemH + 6.0f;

    float posX = m_bounds.x;
    float posY = m_bounds.y + m_bounds.height + 4.0f;

    if (m_context) {
        float viewportH = m_context->viewportSize().height;
        if (posY + popupH > viewportH - 8.0f) {
            posY = m_bounds.y - popupH - 4.0f;
            if (posY < 8.0f) {
                posY = 8.0f;
            }
        }
    }

    return Rect{ posX, posY, popupW, popupH };
}

void ComboBox::onDropdownMouseMove(const Point& screenPos) {
    Rect popupRect = dropdownBounds();
    if (!popupRect.contains(screenPos)) {
        m_hoveredIndex = -1;
        return;
    }

    float itemH = 26.0f;
    float posY = popupRect.y + 3.0f;
    m_hoveredIndex = -1;
    for (size_t i = 0; i < m_options.size(); ++i) {
        Rect itemRect{ popupRect.x + 4.0f, posY + static_cast<float>(i) * itemH, popupRect.width - 8.0f, itemH };
        if (itemRect.contains(screenPos)) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }
}

bool ComboBox::onDropdownPointerDown(const Point& screenPos) {
    Rect popupRect = dropdownBounds();
    if (!popupRect.contains(screenPos)) {
        setOpen(false);
        return false;
    }

    float itemH = 26.0f;
    float posY = popupRect.y + 3.0f;
    for (size_t i = 0; i < m_options.size(); ++i) {
        Rect itemRect{ popupRect.x + 4.0f, posY + static_cast<float>(i) * itemH, popupRect.width - 8.0f, itemH };
        if (itemRect.contains(screenPos)) {
            selectIndex(i);
            setOpen(false);
            return true;
        }
    }

    setOpen(false);
    return true;
}

void ComboBox::renderDropdownOverlay(UIRenderBackend& backend) {
    if (!m_isOpen || m_options.empty()) return;

    Rect popupRect = dropdownBounds();
    float itemH = 26.0f;

    // Drop shadow
    backend.drawShadow(popupRect, 6.0f, Color(0, 0, 0, 220), 16.0f, { 0.0f, 6.0f });

    // Background & accent border
    Color accentBorder = Theme::Current().colors.borderFocus;
    backend.drawRoundedRect(popupRect, Color(16, 22, 30, 250), 6.0f, accentBorder, 1.0f);

    Point mousePos = m_context ? m_context->lastMousePos() : Point{ -1.0f, -1.0f };
    float posX = popupRect.x;
    float posY = popupRect.y + 3.0f;

    for (size_t i = 0; i < m_options.size(); ++i) {
        Rect itemRect{ posX + 4.0f, posY + static_cast<float>(i) * itemH, popupRect.width - 8.0f, itemH };

        bool hovered = (static_cast<int>(i) == m_hoveredIndex) || itemRect.contains(mousePos);
        bool selected = (i == m_selectedIndex);

        if (selected) {
            Color selBg = Color(38, 50, 70, 255);
            backend.drawRoundedRect(itemRect, selBg, 4.0f, accentBorder, 1.0f);
        } else if (hovered) {
            backend.drawRoundedRect(itemRect, Color(30, 42, 58, 230), 4.0f);
        }

        TextStyle itemStyle = m_textStyle;
        itemStyle.color = selected ? accentBorder : (hovered ? Color::White() : Color::TextSecondary());

        Dimensions textDim = backend.measureText(m_options[i], itemStyle);
        float textY = itemRect.y + (std::max)(0.0f, (itemRect.height - textDim.height) * 0.5f);
        backend.drawText(m_options[i], Point{ itemRect.x + 8.0f, textY }, itemStyle);
    }
}

bool ComboBox::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    setOpen(!m_isOpen);
    return true;
}

} // namespace PerfUI
