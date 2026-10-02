#pragma once

#include "UIElement.h"
#include "Animation.h"
#include <string>
#include <functional>

namespace PerfUI {

class TextInput : public UIElement {
public:
    explicit TextInput(std::string placeholder = "Search...");
    ~TextInput() override = default;

    // Content
    const std::string& text() const { return m_text; }
    TextInput& text(std::string newText);

    const std::string& placeholder() const { return m_placeholder; }
    TextInput& placeholder(std::string newPlaceholder);

    float fontSize() const { return m_fontSize; }
    TextInput& fontSize(float size) { m_fontSize = size; return *this; }

    void clear();

    // Callbacks
    TextInput& onTextChanged(std::function<void(const std::string&)> cb) {
        m_onTextChanged = std::move(cb);
        return *this;
    }

    TextInput& onEnter(std::function<void(const std::string&)> cb) {
        m_onEnter = std::move(cb);
        return *this;
    }

    // Lifecycle
    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    // Events
    bool onPointerDown(const Point& localPoint) override;
    void onPointerEnter() override;
    void onPointerLeave() override;
    void onFocusChanged(bool focused) override;
    bool onCharInput(uint32_t charCode) override;
    bool onKeyDown(int keyCode) override;

private:
    std::string m_text;
    std::string m_placeholder;
    float m_fontSize{ 13.0f };
    size_t m_cursorPos{ 0 };

    float m_blinkTimer{ 0.0f };
    AnimatedFloat m_focusGlow{ 0.0f, 14.0f };
    AnimatedFloat m_hoverGlow{ 0.0f, 14.0f };

    std::function<void(const std::string&)> m_onTextChanged;
    std::function<void(const std::string&)> m_onEnter;
};

} // namespace PerfUI
