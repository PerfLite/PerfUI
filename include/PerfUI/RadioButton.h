#pragma once

#include "UIElement.h"
#include "Types.h"
#include <string>
#include <functional>

namespace PerfUI {

class RadioGroup;

class PERFUI_API RadioButton : public UIElement {
public:
    explicit RadioButton(int value = 0, std::string label = "Option", std::string name = "RadioButton");
    ~RadioButton() override = default;

    int value() const { return m_value; }
    RadioButton& value(int val) { m_value = val; return *this; }

    const std::string& label() const { return m_label; }
    RadioButton& label(std::string text) {
        m_label = std::move(text);
        markLayoutDirty();
        return *this;
    }

    bool isSelected() const { return m_selected; }
    bool isChecked() const override { return m_selected; }
    void setSelected(bool sel);
    RadioButton& selected(bool sel) { setSelected(sel); return *this; }

    RadioButton& onSelect(std::function<void(int)> callback) {
        m_onSelect = std::move(callback);
        return *this;
    }

    using UIElement::style;
    RadioButton& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onAction(NavDirection dir) override;

private:
    int m_value{ 0 };
    std::string m_label;
    bool m_selected{ false };
    std::function<void(int)> m_onSelect;

    friend class RadioGroup;
};

} // namespace PerfUI
