#include "PerfUI/RadioGroup.h"

namespace PerfUI {

RadioGroup::RadioGroup(std::string name)
    : UIElement(std::move(name))
{
    layout().direction(LayoutDirection::Vertical);
    layout().gap(8.0f);
}

void RadioGroup::select(int value) {
    m_selectedValue = value;
    for (const auto& child : children()) {
        if (auto* btn = dynamic_cast<RadioButton*>(child.get())) {
            btn->setSelected(btn->value() == value);
        }
    }
    if (m_onSelectionChanged) {
        m_onSelectionChanged(m_selectedValue);
    }
}

RadioButton* RadioGroup::addOption(int value, const std::string& label) {
    auto* btn = add<RadioButton>(value, label);
    if (m_selectedValue == value) {
        btn->setSelected(true);
    }
    return btn;
}

void RadioGroup::notifySelection(RadioButton* selectedBtn) {
    if (!selectedBtn) return;
    m_selectedValue = selectedBtn->value();

    for (const auto& child : children()) {
        if (auto* btn = dynamic_cast<RadioButton*>(child.get())) {
            if (btn != selectedBtn && btn->isSelected()) {
                btn->m_selected = false;
            }
        }
    }

    if (m_onSelectionChanged) {
        m_onSelectionChanged(m_selectedValue);
    }
}

} // namespace PerfUI
