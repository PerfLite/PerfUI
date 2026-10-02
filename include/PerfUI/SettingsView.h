#pragma once

#include "Panel.h"
#include "ScrollView.h"
#include "Text.h"
#include "Button.h"
#include "Checkbox.h"
#include "Slider.h"
#include "ComboBox.h"
#include "TextInput.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

class SettingsSection : public Panel {
public:
    explicit SettingsSection(std::string title);
    ~SettingsSection() override = default;

    const std::string& title() const { return m_title; }

    Checkbox* addToggle(
        std::string label,
        bool initialValue,
        std::string tooltip,
        std::function<void(bool)> onChange
    );

    Slider* addSlider(
        std::string label,
        float min,
        float max,
        float initialValue,
        std::string tooltip,
        std::function<void(float)> onChange,
        std::string format = "%.2f"
    );

    ComboBox* addChoice(
        std::string label,
        std::vector<std::string> options,
        size_t initialIndex,
        std::string tooltip,
        std::function<void(size_t, const std::string&)> onChange
    );

    TextInput* addTextInput(
        std::string label,
        std::string placeholder,
        std::string initialValue,
        std::string tooltip,
        std::function<void(const std::string&)> onChange
    );

    Button* addButton(
        std::string label,
        std::string buttonText,
        std::string tooltip,
        std::function<void()> onClick
    );

    Button* addKeybind(
        std::string label,
        uint32_t currentKey,
        std::string tooltip,
        std::function<void(uint32_t)> onRebind
    );

private:
    Panel* createRow(const std::string& label, const std::string& tooltip);

    std::string m_title;
    Panel* m_itemsContainer{ nullptr };
};

class SettingsView : public ScrollView {
public:
    explicit SettingsView(std::string name = "SettingsView");
    ~SettingsView() override = default;

    SettingsSection* addSection(std::string title);
    void clearSections();

private:
    std::vector<SettingsSection*> m_sections;
};

} // namespace PerfUI
