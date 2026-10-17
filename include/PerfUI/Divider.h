#pragma once

#include "UIElement.h"
#include "Types.h"
#include <optional>

namespace PerfUI {

class PERFUI_API Divider : public UIElement {
public:
    explicit Divider(LayoutDirection orientation = LayoutDirection::Horizontal, std::string name = "Divider");
    ~Divider() override = default;

    LayoutDirection orientation() const { return m_orientation; }
    Divider& orientation(LayoutDirection dir) {
        if (m_orientation != dir) {
            m_orientation = dir;
            markLayoutDirty();
        }
        return *this;
    }

    float thickness() const { return m_thickness; }
    Divider& thickness(float t) {
        if (m_thickness != t) {
            m_thickness = t;
            markLayoutDirty();
        }
        return *this;
    }

    Color color() const;
    Divider& color(Color c) {
        m_customColor = c;
        return *this;
    }

    using UIElement::style;
    Divider& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

private:
    LayoutDirection m_orientation{ LayoutDirection::Horizontal };
    float m_thickness{ 1.0f };
    std::optional<Color> m_customColor;
};

} // namespace PerfUI
