#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include <functional>

namespace PerfUI {

class PERFUI_API Switch : public UIElement {
public:
    explicit Switch(bool checked = false, std::string name = "Switch");
    ~Switch() override = default;

    bool isChecked() const { return m_checked; }
    void setChecked(bool checked);
    Switch& checked(bool c) { setChecked(c); return *this; }

    Switch& onToggle(std::function<void(bool)> callback) {
        m_onToggle = std::move(callback);
        return *this;
    }

    using UIElement::style;
    Switch& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onAction(NavDirection dir) override;

private:
    bool m_checked{ false };
    std::function<void(bool)> m_onToggle;

    AnimatedFloat m_thumbAnim{ 0.0f, 14.0f };
    AnimatedColor m_trackColorAnim{ Color(48, 54, 61, 200), 14.0f };
};

} // namespace PerfUI
