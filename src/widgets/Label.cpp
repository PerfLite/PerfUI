#include "PerfUI/Label.h"

namespace PerfUI {

Label::Label(std::string text, LabelVariant variant, std::string name)
    : Text(std::move(text), std::move(name))
    , m_variant(variant)
{
    applyVariantStyle();
}

Label& Label::variant(LabelVariant v) {
    if (m_variant != v) {
        m_variant = v;
        applyVariantStyle();
    }
    return *this;
}

void Label::applyVariantStyle() {
    const auto& theme = Theme::Current();
    switch (m_variant) {
    case LabelVariant::Heading:
        fontSize(theme.typography.fontTitle);
        bold(true);
        color(theme.colors.textPrimary);
        break;
    case LabelVariant::Body:
        fontSize(theme.typography.fontBody);
        bold(false);
        color(theme.colors.textPrimary);
        break;
    case LabelVariant::Caption:
        fontSize(theme.typography.fontSmall);
        bold(false);
        color(theme.colors.textSecondary);
        break;
    case LabelVariant::Accent:
        fontSize(theme.typography.fontBody);
        bold(true);
        color(theme.colors.textAccent);
        break;
    }
}

} // namespace PerfUI
