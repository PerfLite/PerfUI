#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include "UIRenderBackend.h"
#include <string>
#include <string_view>
#include <functional>

namespace PerfUI {

class PERFUI_API Button : public UIElement {
public:
    explicit Button(std::string label = "Button", std::string name = "Button");
    ~Button() override = default;

    const std::string& label() const { return m_label; }
    Button& label(std::string_view text);

    const std::string& text() const { return m_label; }
    Button& text(std::string_view t) { return label(t); }

    Button& onClick(std::function<void()> callback) {
        m_onClick = std::move(callback);
        return *this;
    }

    Button& fontSize(float size) {
        m_textStyle.fontSize = size;
        markLayoutDirty();
        return *this;
    }

    Button& cornerRadius(float radius) {
        m_cornerRadius = radius;
        return *this;
    }

    Button& normalColor(Color bg, Color border = Color::BorderSubtle(), Color text = Color::TextPrimary()) {
        m_normalBg = bg;
        m_normalBorder = border;
        m_normalTextColor = text;
        return *this;
    }

    Button& hoverColor(Color bg, Color border = Color::BorderStrong(), Color text = Color::White()) {
        m_hoverBg = bg;
        m_hoverBorder = border;
        m_hoverTextColor = text;
        return *this;
    }

    Button& pressedColor(Color bg, Color border = Color::BorderFocus(), Color text = Color::White()) {
        m_pressBg = bg;
        m_pressBorder = border;
        m_pressTextColor = text;
        return *this;
    }


    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onPointerUp(const Point& localPoint) override;
    bool onAction(NavDirection dir) override;

private:
    std::string m_label;
    std::function<void()> m_onClick;
    bool m_triggerOnDown{ true };

    TextStyle m_textStyle{ Color::TextPrimary(), 14.0f, false, false };
    float m_cornerRadius{ 6.0f };
    float m_borderWidth{ 1.0f };

    Color m_normalBg{ 26, 33, 44, 230 };
    Color m_normalBorder{ Color::BorderSubtle() };
    Color m_normalTextColor{ Color::TextPrimary() };

    Color m_hoverBg{ 40, 50, 68, 250 };
    Color m_hoverBorder{ Color::BorderFocus() };
    Color m_hoverTextColor{ Color::White() };

    Color m_pressBg{ 20, 24, 32, 255 };
    Color m_pressBorder{ Color::BorderFocus() };
    Color m_pressTextColor{ Color::White() };

    Color m_focusBorder{ Color::BorderFocus() };
    AnimatedFloat m_hoverAnim{ 0.0f, 14.0f };
};

} // namespace PerfUI
