#pragma once

#include "UIElement.h"
#include "RadioButton.h"
#include <functional>

namespace PerfUI {

class PERFUI_API RadioGroup : public UIElement {
public:
    explicit RadioGroup(std::string name = "RadioGroup");
    ~RadioGroup() override = default;

    int selectedValue() const { return m_selectedValue; }
    void select(int value);

    RadioGroup& onSelectionChanged(std::function<void(int)> callback) {
        m_onSelectionChanged = std::move(callback);
        return *this;
    }

    RadioButton* addOption(int value, const std::string& label);

    void notifySelection(RadioButton* button);

private:
    int m_selectedValue{ -1 };
    std::function<void(int)> m_onSelectionChanged;
};

} // namespace PerfUI
