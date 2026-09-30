#pragma once

#include "UIElement.h"
#include "Types.h"
#include "UIRenderBackend.h"
#include <string>
#include <string_view>

namespace PerfUI {

class Text : public UIElement {
public:
    explicit Text(std::string text = "", std::string name = "Text");
    ~Text() override = default;

    const std::string& text() const { return m_text; }
    Text& text(std::string_view str);

    const TextStyle& style() const { return m_style; }
    TextStyle& style() { markLayoutDirty(); return m_style; }

    Text& color(Color c) { m_style.color = c; return *this; }
    Text& fontSize(float size) { m_style.fontSize = size; markLayoutDirty(); return *this; }
    Text& bold(bool isBold = true) { m_style.bold = isBold; markLayoutDirty(); return *this; }
    Text& italic(bool isItalic = true) { m_style.italic = isItalic; markLayoutDirty(); return *this; }

    Alignment textAlign() const { return m_textAlign; }
    Text& textAlign(Alignment align) { m_textAlign = align; return *this; }

    bool wrap() const { return m_wrap; }
    Text& wrap(bool enable) { m_wrap = enable; markLayoutDirty(); return *this; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

private:
    std::string m_text;
    TextStyle m_style{ Color::TextPrimary(), 15.0f, false, false };
    Alignment m_textAlign{ Alignment::Start };
    bool m_wrap{ false };
};

} // namespace PerfUI
