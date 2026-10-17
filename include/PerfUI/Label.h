#pragma once

#include "Text.h"
#include "Theme.h"

namespace PerfUI {

enum class LabelVariant {
    Heading,
    Body,
    Caption,
    Accent
};

class PERFUI_API Label : public Text {
public:
    explicit Label(std::string text = "", LabelVariant variant = LabelVariant::Body, std::string name = "Label");
    ~Label() override = default;

    LabelVariant variant() const { return m_variant; }
    Label& variant(LabelVariant v);

    using Text::text;
    using Text::color;
    using Text::fontSize;
    using Text::textStyle;
    using UIElement::style;

    Label& text(std::string_view str) { Text::text(str); return *this; }
    Label& color(Color c) { Text::color(c); return *this; }
    Label& fontSize(float size) { Text::fontSize(size); return *this; }

private:
    LabelVariant m_variant{ LabelVariant::Body };
    void applyVariantStyle();
};

} // namespace PerfUI
