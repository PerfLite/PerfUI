#pragma once

#include "Panel.h"
#include "ScrollView.h"
#include "Text.h"
#include "Button.h"
#include "Checkbox.h"
#include "Slider.h"
#include "ComboBox.h"
#include "TextInput.h"
#include "PerfUI/SettingsStore.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <optional>
#include <memory>

namespace PerfUI {

struct SettingCondition {
    std::string key;
    bool invert{ false };
    std::optional<SettingValue> equals;

    SettingCondition() = default;
    SettingCondition(std::string k, bool inv = false)
        : key(std::move(k)), invert(inv) {}
    SettingCondition(std::string k, SettingValue eq, bool inv = false)
        : key(std::move(k)), invert(inv), equals(std::move(eq)) {}

    PERFUI_API bool evaluate(const SettingsStore& store, const std::string& defaultModId) const;
};

class PERFUI_API SettingsSection : public Panel {
public:
    explicit SettingsSection(std::string title, std::string modId = "");
    ~SettingsSection() override;

    const std::string& title() const { return m_title; }
    const std::string& modId() const { return m_modId; }
    void setModId(std::string modId);

    // Legacy callback-based overloads (100% backward compatible)
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

    // Store-bound overloads (Two-way automatic binding)
    Checkbox* addToggle(
        std::string label,
        std::string key,
        std::string tooltip = ""
    );

    Slider* addSlider(
        std::string label,
        std::string key,
        std::string tooltip = ""
    );

    Slider* addStepper(
        std::string label,
        std::string key,
        std::string tooltip = ""
    );

    ComboBox* addChoice(
        std::string label,
        std::string key,
        std::string tooltip = ""
    );

    ComboBox* addMenu(
        std::string label,
        std::string key,
        std::string tooltip = ""
    );

    TextInput* addTextInput(
        std::string label,
        std::string key,
        std::string placeholder = "",
        std::string tooltip = ""
    );

    Button* addKeybind(
        std::string label,
        std::string key,
        std::string tooltip = ""
    );

    UIElement* addColor(
        std::string label,
        std::string key,
        std::string tooltip = ""
    );

    // Structural elements
    Text* addHeader(std::string text);
    Text* addText(std::string text, std::string tooltip = "");
    Panel* addSeparator();
    Panel* addEmpty(float height = 8.0f);
    Button* addResetSectionButton(std::string label = "Reset Section to Defaults");

    // Conditional visibility & enablement
    void setRowVisibleIf(const std::string& key, SettingCondition condition);
    void setRowEnabledIf(const std::string& key, SettingCondition condition);
    void updateConditions();
    void resetAllToDefaults();

    // Check if row has conflict (e.g. keybinds)
    void setKeybindConflict(const std::string& key, bool conflict);

    Panel* itemsContainer() const { return m_itemsContainer; }

    void update(float dt) override;

private:
    struct PendingStoreUpdate {
        std::string key;
        SettingValue val;
    };

    struct BoundRow {
        std::string key;
        Panel* rowPanel{ nullptr };
        Text* labelText{ nullptr };
        UIElement* controlWidget{ nullptr };
        Button* resetBtn{ nullptr };
        std::optional<SettingCondition> visibleCond;
        std::optional<SettingCondition> enabledCond;
        uint64_t subscriptionId{ 0 };
        std::function<void(const SettingValue&)> updateWidget;
    };

    Panel* createRow(const std::string& label, const std::string& tooltip, const std::string& key = "");
    void setupRowBinding(const std::string& key, Panel* row, Text* lbl, UIElement* ctrl, Button* resetBtn, std::function<void(const SettingValue&)> updateWidget);
    void applyStoreUpdate(const std::string& key, const SettingValue& val);
    void updateRowModifiedState(BoundRow& row);

    std::string m_title;
    std::string m_modId;
    Panel* m_itemsContainer{ nullptr };
    std::vector<BoundRow> m_boundRows;
    bool m_isUpdatingFromStore{ false };
    std::mutex m_pendingUpdatesMutex;
    std::vector<PendingStoreUpdate> m_pendingUpdates;
};

class PERFUI_API SettingsView : public ScrollView {
public:
    explicit SettingsView(std::string name = "SettingsView", std::string modId = "");
    ~SettingsView() override = default;

    const std::string& modId() const { return m_modId; }
    void setModId(std::string modId);

    SettingsSection* addSection(std::string title);
    void clearSections();

    Button* addResetPageButton(std::string label = "Reset Page to Defaults");
    void resetAllToDefaults();
    void updateConditions();

private:
    std::string m_modId;
    std::vector<SettingsSection*> m_sections;
};

} // namespace PerfUI
