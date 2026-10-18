#pragma once

#include "UIElement.h"
#include "Types.h"
#include <string>

namespace PerfUI {

class PERFUI_API TextArea : public UIElement {
public:
    explicit TextArea(std::string text = "", std::string name = "TextArea");
    ~TextArea() override = default;

    const std::string& text() const { return m_text; }
    TextArea& text(std::string t);

    float fontSize() const { return m_textStyle.fontSize; }
    TextArea& fontSize(float s) { m_textStyle.fontSize = s; markLayoutDirty(); return *this; }

    Color textColor() const { return m_textStyle.color; }
    TextArea& textColor(Color c) { m_textStyle.color = c; return *this; }

    bool isReadOnly() const { return m_readOnly; }
    TextArea& readOnly(bool ro) { m_readOnly = ro; return *this; }

    using UIElement::style;
    TextArea& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onMouseWheel(float delta, const Point& localPoint) override;

private:
    std::string m_text;
    TextStyle m_textStyle{ Color::TextPrimary(), 14.0f, false, false };
    bool m_readOnly{ true };
    float m_scrollOffset{ 0.0f };
    float m_contentHeight{ 0.0f };
};

} // namespace PerfUI
