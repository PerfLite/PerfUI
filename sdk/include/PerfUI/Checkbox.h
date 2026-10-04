#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include "UIRenderBackend.h"
#include <string>
#include <string_view>
#include <functional>

namespace PerfUI {

class PERFUI_API Checkbox : public UIElement {
public:
    explicit Checkbox(std::string label = "", bool checked = false, std::string name = "Checkbox");
    ~Checkbox() override = default;

    bool isChecked() const { return m_checked; }
    Checkbox& checked(bool chk);

    const std::string& label() const { return m_label; }
    Checkbox& label(std::string_view text);

    Checkbox& onToggle(std::function<void(bool)> callback) {
        m_onToggle = std::move(callback);
        return *this;
    }

    Checkbox& boxSize(float size) { m_boxSize = size; markLayoutDirty(); return *this; }
    Checkbox& checkColor(Color color) { m_checkColor = color; return *this; }
    Checkbox& labelColor(Color color) { m_labelStyle.color = color; return *this; }
    Checkbox& fontSize(float size) { m_labelStyle.fontSize = size; markLayoutDirty(); return *this; }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onPointerUp(const Point& localPoint) override;
    bool onAction(NavDirection dir) override;

private:
    bool m_checked{ false };
    std::string m_label;
    float m_boxSize{ 18.0f };
    float m_cornerRadius{ 4.0f };

    Color m_checkColor{ Color::NordicGold() };
    Color m_boxBg{ 20, 26, 36, 230 };
    Color m_boxBorder{ Color::BorderSubtle() };
    TextStyle m_labelStyle{ Color::TextPrimary(), 14.0f, false, false };

    std::function<void(bool)> m_onToggle;
    AnimatedFloat m_checkAnim{ 0.0f, 16.0f };
    AnimatedFloat m_hoverAnim{ 0.0f, 12.0f };
};

} // namespace PerfUI
