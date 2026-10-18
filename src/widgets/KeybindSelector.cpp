#include "PerfUI/KeybindSelector.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

KeybindSelector::KeybindSelector(uint32_t keyCode, std::string label, std::string name)
    : UIElement(std::move(name))
    , m_keyCode(keyCode)
    , m_label(std::move(label))
{
    setFocusable(true);
}

void KeybindSelector::setKeyCode(uint32_t key) {
    if (m_keyCode != key) {
        m_keyCode = key;
        markLayoutDirty();
        if (m_onKeyChanged) {
            m_onKeyChanged(m_keyCode);
        }
    }
}

void KeybindSelector::startListening() {
    m_listening = true;
}

void KeybindSelector::stopListening() {
    m_listening = false;
}

std::string KeybindSelector::KeyCodeToString(uint32_t keyCode) {
    if (keyCode == 0) return "None";

    // Standard ASCII letters & numbers
    if (keyCode >= 'A' && keyCode <= 'Z') {
        return std::string(1, static_cast<char>(keyCode));
    }
    if (keyCode >= '0' && keyCode <= '9') {
        return std::string(1, static_cast<char>(keyCode));
    }

    // Common Virtual Keys
    switch (keyCode) {
    case 0x08: return "Backspace";
    case 0x09: return "Tab";
    case 0x0D: return "Enter";
    case 0x10: return "Shift";
    case 0x11: return "Ctrl";
    case 0x12: return "Alt";
    case 0x13: return "Pause";
    case 0x14: return "CapsLock";
    case 0x1B: return "Esc";
    case 0x20: return "Space";
    case 0x21: return "PageUp";
    case 0x22: return "PageDown";
    case 0x23: return "End";
    case 0x24: return "Home";
    case 0x25: return "Left";
    case 0x26: return "Up";
    case 0x27: return "Right";
    case 0x28: return "Down";
    case 0x2D: return "Insert";
    case 0x2E: return "Delete";
    case 0x70: return "F1";
    case 0x71: return "F2";
    case 0x72: return "F3";
    case 0x73: return "F4";
    case 0x74: return "F5";
    case 0x75: return "F6";
    case 0x76: return "F7";
    case 0x77: return "F8";
    case 0x78: return "F9";
    case 0x79: return "F10";
    case 0x7A: return "F11";
    case 0x7B: return "F12";
    case 0xA0: return "LShift";
    case 0xA1: return "RShift";
    case 0xA2: return "LCtrl";
    case 0xA3: return "RCtrl";
    case 0xA4: return "LAlt";
    case 0xA5: return "RAlt";
    default:
        return "Key " + std::to_string(keyCode);
    }
}

void KeybindSelector::measure(Dimensions availableSize) {
    (void)availableSize;
    float w = 180.0f;
    float h = 32.0f;

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    }
    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    }

    m_desiredSize = Dimensions{ w, h };
}

void KeybindSelector::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    WidgetState state = currentState();
    BoxStyle resolvedBox = resolveBoxStyle(state);

    float labelWidth = m_bounds.width * 0.55f;
    float keyBoxWidth = m_bounds.width - labelWidth - 8.0f;

    // Render Label
    if (!m_label.empty()) {
        TextStyle lblStyle;
        lblStyle.color = Theme::Current().colors.textPrimary;
        lblStyle.fontSize = 14.0f;

        Point lblPos{ m_bounds.x, m_bounds.y + (m_bounds.height - 14.0f * 1.25f) * 0.5f };
        backend.drawText(m_label, lblPos, lblStyle);
    }

    // Key Box
    Rect keyRect{ m_bounds.x + labelWidth + 8.0f, m_bounds.y, keyBoxWidth, m_bounds.height };

    Color bg = resolvedBox.backgroundColor.value_or(Color(26, 33, 44, 230));
    Color border = resolvedBox.borderColor.value_or(Theme::Current().colors.borderSubtle);
    float radius = resolvedBox.cornerRadius.value_or(4.0f);
    float borderW = 1.0f;

    if (m_listening) {
        border = Theme::Current().colors.borderFocus;
        bg = Color(40, 35, 20, 240); // gold-tinted glow background while listening
        borderW = 2.0f;
    } else if (isFocused() || state == WidgetState::Focused) {
        border = Theme::Current().colors.borderFocus;
        borderW = 1.5f;
    } else if (state == WidgetState::Hovered) {
        border = Theme::Current().colors.borderStrong;
    }

    backend.drawRoundedRect(keyRect, bg, radius, border, borderW);

    // Key Text
    std::string keyText = m_listening ? "[Press Key]" : KeyCodeToString(m_keyCode);
    TextStyle keyStyle;
    keyStyle.color = m_listening ? Theme::Current().colors.textAccent : (m_keyCode == 0 ? Theme::Current().colors.textDisabled : Color::White());
    keyStyle.fontSize = 13.0f;
    keyStyle.bold = true;

    Dimensions textDim = backend.measureText(keyText, keyStyle);
    float textX = keyRect.x + (std::max)(0.0f, (keyRect.width - textDim.width) * 0.5f);
    float textY = keyRect.y + (std::max)(0.0f, (keyRect.height - textDim.height) * 0.5f);

    backend.drawText(keyText, Point{ textX, textY }, keyStyle);

    UIElement::render(backend);
}

bool KeybindSelector::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    if (!m_listening) {
        startListening();
    } else {
        stopListening();
    }
    return true;
}

bool KeybindSelector::onKeyDown(int key) {
    if (m_listening) {
        if (key == 0x1B) { // Esc cancels listening or clears
            setKeyCode(0);
        } else {
            setKeyCode(static_cast<uint32_t>(key));
        }
        stopListening();
        return true;
    }
    return false;
}

bool KeybindSelector::onAction(NavDirection dir) {
    (void)dir;
    if (!m_listening) {
        startListening();
    } else {
        stopListening();
    }
    return true;
}

} // namespace PerfUI
