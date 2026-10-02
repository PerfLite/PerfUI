#include "PerfUI/TextInput.h"
#include "PerfUI/UIRenderBackend.h"
#include <algorithm>
#include <cmath>

namespace PerfUI {

TextInput::TextInput(std::string placeholder)
    : m_placeholder(std::move(placeholder)) {
    setFocusable(true);
    layout().height(30.0f);
}

TextInput& TextInput::text(std::string newText) {
    m_text = std::move(newText);
    if (m_cursorPos > m_text.size()) {
        m_cursorPos = m_text.size();
    }
    m_blinkTimer = 0.0f;
    return *this;
}

TextInput& TextInput::placeholder(std::string newPlaceholder) {
    m_placeholder = std::move(newPlaceholder);
    return *this;
}

void TextInput::clear() {
    if (!m_text.empty()) {
        m_text.clear();
        m_cursorPos = 0;
        m_blinkTimer = 0.0f;
        if (m_onTextChanged) {
            m_onTextChanged(m_text);
        }
    }
}

void TextInput::update(float deltaTime) {
    UIElement::update(deltaTime);
    m_focusGlow.update(deltaTime);
    m_hoverGlow.update(deltaTime);

    if (isFocused()) {
        m_blinkTimer += deltaTime;
    } else {
        m_blinkTimer = 0.0f;
    }
}

void TextInput::measure(Dimensions availableSize) {
    float h = 30.0f;
    if (m_layout.height().mode == SizeMode::Fixed) {
        h = m_layout.height().value;
    }

    float w = 200.0f;
    if (m_layout.width().mode == SizeMode::Fixed) {
        w = m_layout.width().value;
    } else if (availableSize.width > 0.0f) {
        w = (std::min)(availableSize.width, 400.0f);
    }

    setDesiredSize({ w, h });
}

void TextInput::render(UIRenderBackend& backend) {
    if (!isVisible() || m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) return;

    // Nordic Theme border and background
    Color bg(18, 22, 28, 230);
    Color borderNormal(60, 68, 80, 180);
    Color borderHover(170, 145, 80, 210);
    Color borderFocus(235, 195, 85, 255);

    Color border = Color::Lerp(borderNormal, borderHover, m_hoverGlow.value());
    border = Color::Lerp(border, borderFocus, m_focusGlow.value());
    float borderWidth = 1.0f + 0.5f * m_focusGlow.value();

    backend.drawRoundedRect(m_bounds, bg, 5.0f, border, borderWidth);

    // Clip inner text to avoid overflow
    Rect clipRect(m_bounds.x + 3.0f, m_bounds.y + 2.0f, m_bounds.width - 6.0f, m_bounds.height - 4.0f);
    backend.pushClipRect(clipRect);

    TextStyle style;
    style.fontSize = m_fontSize;

    const float padLeft = 8.0f;
    const float padTop = (m_bounds.height - m_fontSize) * 0.5f - 1.0f;

    float cursorRelX = 0.0f;
    if (m_cursorPos > 0 && m_cursorPos <= m_text.size()) {
        std::string sub = m_text.substr(0, m_cursorPos);
        cursorRelX = backend.measureText(sub, style).width;
    }

    const float visibleWidth = m_bounds.width - padLeft * 2.0f;
    float scrollOffset = 0.0f;
    if (cursorRelX > visibleWidth) {
        scrollOffset = cursorRelX - visibleWidth;
    }

    if (m_text.empty()) {
        style.color = Color(125, 135, 150, 160);
        backend.drawText(m_placeholder, Point(m_bounds.x + padLeft, m_bounds.y + padTop), style);
    } else {
        style.color = Color(240, 242, 245, 255);
        Point textPos(m_bounds.x + padLeft - scrollOffset, m_bounds.y + padTop);
        backend.drawText(m_text, textPos, style);
    }

    // Caret / Cursor
    if (isFocused()) {
        if (std::fmod(m_blinkTimer, 0.9f) < 0.45f) {
            float cx = m_bounds.x + padLeft + cursorRelX - scrollOffset;
            float cy = m_bounds.y + 5.0f;
            float ch = m_bounds.height - 10.0f;
            backend.drawRect(Rect(cx, cy, 1.5f, ch), Color(240, 200, 100, 255));
        }
    }

    backend.popClipRect();
}

bool TextInput::onPointerDown(const Point& localPoint) {
    const float padLeft = 8.0f;
    float clickX = localPoint.x - padLeft;

    if (clickX <= 0.0f || m_text.empty()) {
        m_cursorPos = 0;
    } else {
        size_t approx = static_cast<size_t>(clickX / (m_fontSize * 0.55f));
        if (approx > m_text.size()) approx = m_text.size();

        // Snap to UTF-8 code point start
        while (approx > 0 && approx < m_text.size() && (static_cast<unsigned char>(m_text[approx]) & 0xC0) == 0x80) {
            ++approx;
        }
        m_cursorPos = approx;
    }

    m_blinkTimer = 0.0f;
    return true;
}

void TextInput::onPointerEnter() {
    m_hoverGlow.setTarget(1.0f);
}

void TextInput::onPointerLeave() {
    m_hoverGlow.setTarget(0.0f);
}

void TextInput::onFocusChanged(bool focused) {
    m_focusGlow.setTarget(focused ? 1.0f : 0.0f);
    m_blinkTimer = 0.0f;
}

bool TextInput::onCharInput(uint32_t charCode) {
    if (charCode < 32 || charCode == 127) {
        return false;
    }

    // Convert Unicode code point to UTF-8
    std::string utf8;
    if (charCode <= 0x7F) {
        utf8 += static_cast<char>(charCode);
    } else if (charCode <= 0x7FF) {
        utf8 += static_cast<char>(0xC0 | ((charCode >> 6) & 0x1F));
        utf8 += static_cast<char>(0x80 | (charCode & 0x3F));
    } else if (charCode <= 0xFFFF) {
        utf8 += static_cast<char>(0xE0 | ((charCode >> 12) & 0x0F));
        utf8 += static_cast<char>(0x80 | ((charCode >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (charCode & 0x3F));
    } else if (charCode <= 0x10FFFF) {
        utf8 += static_cast<char>(0xF0 | ((charCode >> 18) & 0x07));
        utf8 += static_cast<char>(0x80 | ((charCode >> 12) & 0x3F));
        utf8 += static_cast<char>(0x80 | ((charCode >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (charCode & 0x3F));
    }

    if (m_cursorPos >= m_text.size()) {
        m_text += utf8;
        m_cursorPos = m_text.size();
    } else {
        m_text.insert(m_cursorPos, utf8);
        m_cursorPos += utf8.size();
    }

    m_blinkTimer = 0.0f;
    if (m_onTextChanged) {
        m_onTextChanged(m_text);
    }
    return true;
}

bool TextInput::onKeyDown(int keyCode) {
    switch (keyCode) {
    case 0x08: { // VK_BACK
        if (m_cursorPos > 0) {
            size_t prevPos = m_cursorPos;
            do {
                --m_cursorPos;
            } while (m_cursorPos > 0 && (static_cast<unsigned char>(m_text[m_cursorPos]) & 0xC0) == 0x80);

            m_text.erase(m_cursorPos, prevPos - m_cursorPos);
            m_blinkTimer = 0.0f;
            if (m_onTextChanged) m_onTextChanged(m_text);
            return true;
        }
        return false;
    }
    case 0x2E: { // VK_DELETE
        if (m_cursorPos < m_text.size()) {
            size_t nextPos = m_cursorPos + 1;
            while (nextPos < m_text.size() && (static_cast<unsigned char>(m_text[nextPos]) & 0xC0) == 0x80) {
                ++nextPos;
            }
            m_text.erase(m_cursorPos, nextPos - m_cursorPos);
            m_blinkTimer = 0.0f;
            if (m_onTextChanged) m_onTextChanged(m_text);
            return true;
        }
        return false;
    }
    case 0x25: { // VK_LEFT
        if (m_cursorPos > 0) {
            do {
                --m_cursorPos;
            } while (m_cursorPos > 0 && (static_cast<unsigned char>(m_text[m_cursorPos]) & 0xC0) == 0x80);
            m_blinkTimer = 0.0f;
            return true;
        }
        return false;
    }
    case 0x27: { // VK_RIGHT
        if (m_cursorPos < m_text.size()) {
            do {
                ++m_cursorPos;
            } while (m_cursorPos < m_text.size() && (static_cast<unsigned char>(m_text[m_cursorPos]) & 0xC0) == 0x80);
            m_blinkTimer = 0.0f;
            return true;
        }
        return false;
    }
    case 0x24: { // VK_HOME
        m_cursorPos = 0;
        m_blinkTimer = 0.0f;
        return true;
    }
    case 0x23: { // VK_END
        m_cursorPos = m_text.size();
        m_blinkTimer = 0.0f;
        return true;
    }
    case 0x0D: { // VK_RETURN
        if (m_onEnter) {
            m_onEnter(m_text);
        }
        return true;
    }
    case 0x1B: { // VK_ESCAPE
        if (!m_text.empty()) {
            clear();
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

} // namespace PerfUI
