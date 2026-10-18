#include "PerfUI/ItemSlot.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

ItemSlot::ItemSlot(std::string name)
    : UIElement(std::move(name))
{
    setFocusable(true);
}

Color ItemSlot::getRarityBorderColor() const {
    switch (m_rarity) {
    case ItemRarity::Fine:
        return Color(60, 185, 80, 255);
    case ItemRarity::Superior:
        return Color(65, 145, 245, 255);
    case ItemRarity::Epic:
        return Color(170, 75, 235, 255);
    case ItemRarity::Legendary:
        return Color(245, 185, 45, 255);
    case ItemRarity::Common:
    default:
        return Theme::Current().colors.borderSubtle;
    }
}

void ItemSlot::measure(Dimensions availableSize) {
    (void)availableSize;
    float w = 50.0f;
    float h = 50.0f;

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    }
    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    }

    m_desiredSize = Dimensions{ w, h };
}

void ItemSlot::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    WidgetState state = currentState();
    if (m_selected) {
        state = WidgetState::Selected;
    }

    BoxStyle resolvedBox = resolveBoxStyle(state);

    Color bg = resolvedBox.backgroundColor.value_or(Color(20, 25, 32, 235));
    Color border = resolvedBox.borderColor.value_or(getRarityBorderColor());
    float radius = resolvedBox.cornerRadius.value_or(6.0f);
    float borderW = resolvedBox.borderWidth.value_or(1.5f);

    if (isFocused() || state == WidgetState::Focused) {
        border = Theme::Current().colors.borderFocus;
        borderW = 2.0f;
    } else if (state == WidgetState::Hovered) {
        border = Color::Lerp(border, Color::White(), 0.35f);
        borderW = 2.0f;
    }

    // Shadow on hover / focus
    if (state == WidgetState::Hovered || isFocused()) {
        Color glow = isFocused() ? Theme::Current().colors.focusGlow : Color(0, 0, 0, 140);
        backend.drawShadow(m_bounds, radius, glow, 8.0f, { 0.0f, 2.0f });
    }

    // Slot Background and border
    backend.drawRoundedRect(m_bounds, bg, radius, border, borderW);

    // Icon image
    if (m_icon != 0) {
        Rect iconRect = m_bounds.inset(Insets{ 4.0f });
        backend.drawImage(m_icon, iconRect, Color::White());
    }

    // Equipped badge (top-left diamond / indicator)
    if (m_equipped) {
        Point eqCenter{ m_bounds.x + 8.0f, m_bounds.y + 8.0f };
        backend.drawCircle(eqCenter, 4.0f, Theme::Current().colors.borderFocus, Color(0, 0, 0, 200), 1.0f);
    }

    // Stack Count (bottom-right text)
    if (m_count > 1) {
        std::string countStr = std::to_string(m_count);
        TextStyle textStyle;
        textStyle.color = Color::White();
        textStyle.fontSize = 11.0f;
        textStyle.bold = true;

        Dimensions countDim = backend.measureText(countStr, textStyle);
        Point textPos{
            m_bounds.x + m_bounds.width - countDim.width - 4.0f,
            m_bounds.y + m_bounds.height - countDim.height - 2.0f
        };

        // Text drop shadow for clarity
        TextStyle shadowStyle = textStyle;
        shadowStyle.color = Color::Black();
        backend.drawText(countStr, Point{ textPos.x + 1.0f, textPos.y + 1.0f }, shadowStyle);

        backend.drawText(countStr, textPos, textStyle);
    }

    UIElement::render(backend);
}

bool ItemSlot::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    if (m_onClick) {
        m_onClick();
    }
    return true;
}

bool ItemSlot::onAction(NavDirection dir) {
    (void)dir;
    if (m_onClick) {
        m_onClick();
    }
    return true;
}

} // namespace PerfUI
