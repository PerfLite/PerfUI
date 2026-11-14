#include "PerfUI/SettingsView.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/Theme.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace PerfUI {

bool SettingCondition::evaluate(const SettingsStore& store, const std::string& defaultModId) const {
    if (key.empty()) return true;
    std::string mod = defaultModId;
    std::string k = key;
    size_t colon = key.find(':');
    if (colon != std::string::npos) {
        mod = key.substr(0, colon);
        k = key.substr(colon + 1);
    }
    SettingValue val = store.get(mod, k);
    bool res = false;
    if (equals.has_value()) {
        res = SettingValuesEqual(val, *equals);
    } else {
        res = SettingValueToBool(val);
    }
    return invert ? !res : res;
}

// ==================== ColorButton Widget ====================

class ColorButton : public Panel {
public:
    ColorButton(Color initialColor, std::function<void(Color)> onChange)
        : Panel("ColorButton")
        , m_color(initialColor)
        , m_onChange(std::move(onChange))
    {
        layout().direction(LayoutDirection::Vertical).gap(4.0f);

        m_headerRow = add<Panel>("ColorHeaderRow");
        m_headerRow->layout().direction(LayoutDirection::Horizontal)
                    .alignment(Alignment::Center)
                    .gap(8.0f);

        m_swatch = m_headerRow->add<Panel>("ColorSwatch");
        m_swatch->backgroundColor(m_color);
        m_swatch->borderColor(Color::BorderSubtle());
        m_swatch->borderWidth(1.0f);
        m_swatch->cornerRadius(4.0f);
        m_swatch->layout().width(24.0f).height(24.0f);

        m_btn = m_headerRow->add<Button>(FormatColorHex(m_color));
        m_btn->layout().padding(8.0f, 4.0f);
        m_btn->fontSize(12.0f);

        m_btn->onClick([this]() {
            togglePicker();
        });
    }

    Color color() const { return m_color; }

    void setColor(Color c) {
        m_color = c;
        if (m_swatch) m_swatch->backgroundColor(m_color);
        if (m_btn) m_btn->label(FormatColorHex(m_color));
        updateSliders();
    }

private:
    static std::string FormatColorHex(const Color& c) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
        return buf;
    }

    void togglePicker() {
        if (m_pickerPanel) {
            removeChild(m_pickerPanel);
            m_pickerPanel = nullptr;
        } else {
            m_pickerPanel = add<Panel>("ColorPickerDrawer");
            m_pickerPanel->backgroundColor(Color(25, 30, 40, 240));
            m_pickerPanel->borderColor(Color::BorderSubtle());
            m_pickerPanel->borderWidth(1.0f);
            m_pickerPanel->cornerRadius(6.0f);
            m_pickerPanel->layout().direction(LayoutDirection::Vertical)
                                   .gap(4.0f)
                                   .padding(8.0f)
                                   .width(220.0f);

            auto addChannel = [this](const std::string& name, uint8_t initialVal, auto onChg) {
                auto* row = m_pickerPanel->add<Panel>("Channel_" + name);
                row->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(6.0f);
                auto* lbl = row->add<Text>(name);
                lbl->layout().width(16.0f);
                lbl->fontSize(11.0f);
                auto* sl = row->add<Slider>(static_cast<float>(initialVal), 0.0f, 255.0f);
                sl->layout().width(130.0f).height(18.0f);
                auto* valText = row->add<Text>(std::to_string(initialVal));
                valText->layout().width(30.0f);
                valText->fontSize(11.0f);

                sl->onValueChanged([this, valText, onChg](float v) {
                    uint8_t u = static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
                    valText->text(std::to_string(u));
                    onChg(u);
                    if (m_swatch) m_swatch->backgroundColor(m_color);
                    if (m_btn) m_btn->label(FormatColorHex(m_color));
                    if (m_onChange) m_onChange(m_color);
                });
                return sl;
            };

            m_sliderR = addChannel("R", m_color.r, [this](uint8_t v) { m_color.r = v; });
            m_sliderG = addChannel("G", m_color.g, [this](uint8_t v) { m_color.g = v; });
            m_sliderB = addChannel("B", m_color.b, [this](uint8_t v) { m_color.b = v; });
            m_sliderA = addChannel("A", m_color.a, [this](uint8_t v) { m_color.a = v; });
        }
        markLayoutDirty();
    }

    void updateSliders() {
        if (m_sliderR) m_sliderR->value(static_cast<float>(m_color.r));
        if (m_sliderG) m_sliderG->value(static_cast<float>(m_color.g));
        if (m_sliderB) m_sliderB->value(static_cast<float>(m_color.b));
        if (m_sliderA) m_sliderA->value(static_cast<float>(m_color.a));
    }

    Color m_color;
    std::function<void(Color)> m_onChange;
    Panel* m_headerRow{ nullptr };
    Panel* m_swatch{ nullptr };
    Button* m_btn{ nullptr };
    Panel* m_pickerPanel{ nullptr };
    Slider* m_sliderR{ nullptr };
    Slider* m_sliderG{ nullptr };
    Slider* m_sliderB{ nullptr };
    Slider* m_sliderA{ nullptr };
};

// ==================== KeybindButton ====================

class KeybindButton : public Button {
public:
    KeybindButton(uint32_t currentKey, std::function<void(uint32_t)> onRebind)
        : Button(FormatKey(currentKey))
        , m_key(currentKey)
        , m_onRebind(std::move(onRebind))
    {
        setFocusable(true);
        onClick([this]() {
            if (m_isListening) {
                stopListening(false);
            } else {
                startListening();
            }
        });
    }

    uint32_t key() const { return m_key; }
    void setKey(uint32_t key) {
        m_key = key;
        label(FormatKey(m_key));
    }

    void setConflict(bool conflict) {
        if (conflict) {
            addClass("settings-conflict");
            normalColor(Theme::Current().colors.surfaceElevated, Color{ 220, 50, 50, 255 }, Color{ 255, 100, 100, 255 });
        } else {
            removeClass("settings-conflict");
            normalColor(Theme::Current().colors.surfaceElevated, Theme::Current().colors.borderSubtle, Theme::Current().colors.textPrimary);
        }
        markLayoutDirty();
    }

    void startListening() {
        m_isListening = true;
        label("[ Press Any Key / Esc / Backspace ]");
        const auto& theme = Theme::Current();
        normalColor(theme.colors.surfaceActive, theme.colors.borderFocus, theme.colors.textAccent);
        if (context()) {
            context()->setFocus(this);
            context()->setCapturingKeybind(true);
        }
        markLayoutDirty();
    }

    void stopListening(bool success) {
        m_isListening = false;
        label(FormatKey(m_key));
        const auto& theme = Theme::Current();
        normalColor(theme.colors.surfaceBase, theme.colors.borderSubtle, theme.colors.textPrimary);
        if (context()) {
            context()->setCapturingKeybind(false);
            if (success) {
                context()->playSound("UIMenuOK");
            }
        }
        markLayoutDirty();
    }

    bool onKeyDown(int keyCode) override {
        if (!m_isListening) return false;

        if (keyCode == 0x1B) { // VK_ESCAPE cancels rebinding
            stopListening(false);
            return true;
        }

        if (keyCode == 0x08 || keyCode == 0x2E) { // Backspace or Delete to Unbind
            m_key = 0;
            if (m_onRebind) {
                m_onRebind(m_key);
            }
            stopListening(true);
            return true;
        }

        m_key = static_cast<uint32_t>(keyCode);
        if (m_onRebind) {
            m_onRebind(m_key);
        }
        stopListening(true);
        return true;
    }

    void onFocusChanged(bool focused) override {
        Button::onFocusChanged(focused);
        if (!focused && m_isListening) {
            stopListening(false);
        }
    }

    static std::string FormatKey(uint32_t vk) {
        if (vk == 0) return "[ None ]";
        return "[ " + GetKeyName(vk) + " ]";
    }

    static std::string GetKeyName(uint32_t vk) {
        if (vk >= 0x70 && vk <= 0x87) { // F1 - F24
            return "F" + std::to_string(vk - 0x70 + 1);
        }
        if (vk >= 0x60 && vk <= 0x69) { // Numpad 0 - 9
            return "NUMPAD " + std::to_string(vk - 0x60);
        }
        switch (vk) {
        case 0x1B: return "ESC";
        case 0x09: return "TAB";
        case 0x20: return "SPACE";
        case 0x0D: return "ENTER";
        case 0x08: return "BACKSPACE";
        case 0x2E: return "DELETE";
        case 0x2D: return "INSERT";
        case 0x24: return "HOME";
        case 0x23: return "END";
        case 0x21: return "PAGE UP";
        case 0x22: return "PAGE DOWN";
        case 0x25: return "LEFT";
        case 0x26: return "UP";
        case 0x27: return "RIGHT";
        case 0x28: return "DOWN";
        case 0x2C: return "PRINTSCREEN";
        case 0x13: return "PAUSE";
        case 0x14: return "CAPS LOCK";
        case 0x90: return "NUM LOCK";
        case 0x91: return "SCROLL LOCK";
        case 0x10: return "SHIFT";
        case 0xA0: return "LSHIFT";
        case 0xA1: return "RSHIFT";
        case 0x11: return "CTRL";
        case 0xA2: return "LCTRL";
        case 0xA3: return "RCTRL";
        case 0x12: return "ALT";
        case 0xA4: return "LALT";
        case 0xA5: return "RALT";
        case 0x5B: return "LWIN";
        case 0x5C: return "RWIN";
        case 0x6A: return "NUM *";
        case 0x6B: return "NUM +";
        case 0x6C: return "NUM SEP";
        case 0x6D: return "NUM -";
        case 0x6E: return "NUM .";
        case 0x6F: return "NUM /";
        case 0xBA: return ";";
        case 0xBB: return "=";
        case 0xBC: return ",";
        case 0xBD: return "-";
        case 0xBE: return ".";
        case 0xBF: return "/";
        case 0xC0: return "~";
        case 0xDB: return "[";
        case 0xDC: return "\\";
        case 0xDD: return "]";
        case 0xDE: return "'";
        default:
            break;
        }
        if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
            return std::string(1, static_cast<char>(vk));
        }
#ifdef _WIN32
        UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        if (scanCode > 0) {
            LONG lParam = (scanCode << 16);
            char name[64] = { 0 };
            if (GetKeyNameTextA(lParam, name, sizeof(name)) > 0) {
                return std::string(name);
            }
        }
#endif
        char buf[16];
        std::snprintf(buf, sizeof(buf), "0x%02X", vk);
        return buf;
    }

private:
    uint32_t m_key{ 0x7A };
    bool m_isListening{ false };
    std::function<void(uint32_t)> m_onRebind;
};

// ==================== SettingsSection ====================

SettingsSection::SettingsSection(std::string title, std::string modId)
    : Panel("SettingsSection_" + title)
    , m_title(std::move(title))
    , m_modId(std::move(modId))
{
    addClass("settings-section");
    backgroundColor(Color::Transparent());
    borderWidth(0.0f);
    layout().direction(LayoutDirection::Vertical)
            .gap(6.0f)
            .margin(0.0f, 0.0f, 16.0f, 0.0f)
            .width(DimensionConstraint::Flex(1.0f));

    // Section Header
    auto* header = add<Text>(m_title);
    header->addClass("settings-header");
    header->color(Color::TextAccent()).fontSize(14.0f).bold(true);

    // Separator line
    auto* sep = add<Panel>("Sep");
    sep->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    sep->layout().height(1.0f).width(DimensionConstraint::Flex(1.0f)).margin(0.0f, 2.0f, 6.0f, 0.0f);

    m_itemsContainer = add<Panel>("ItemsContainer");
    m_itemsContainer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    m_itemsContainer->layout().direction(LayoutDirection::Vertical).gap(6.0f).width(DimensionConstraint::Flex(1.0f));
}

SettingsSection::~SettingsSection() {
    auto& store = SettingsStore::GetSingleton();
    for (const auto& r : m_boundRows) {
        if (r.subscriptionId != 0) {
            store.unsubscribe(r.subscriptionId);
        }
    }
}

void SettingsSection::setModId(std::string modId) {
    m_modId = std::move(modId);
}

Panel* SettingsSection::createRow(const std::string& label, const std::string& tooltip, const std::string& /*key*/) {
    const auto& theme = Theme::Current();
    auto* row = m_itemsContainer->add<Panel>("SettingRow");
    row->addClass("settings-row");
    row->backgroundColor(Color(theme.colors.surfaceElevated.r, theme.colors.surfaceElevated.g, theme.colors.surfaceElevated.b, 35));
    row->borderColor(Color(theme.colors.borderSubtle.r, theme.colors.borderSubtle.g, theme.colors.borderSubtle.b, 50));
    row->borderWidth(1.0f);
    row->cornerRadius(theme.metrics.radiusSmall > 0.0f ? theme.metrics.radiusSmall : 6.0f);
    row->layout().direction(LayoutDirection::Horizontal)
                 .alignment(Alignment::Center)
                 .justify(JustifyContent::SpaceBetween)
                 .padding(12.0f, 8.0f)
                 .width(DimensionConstraint::Flex(1.0f));

    auto* lbl = row->add<Text>(label);
    lbl->addClass("settings-label");
    lbl->color(theme.colors.textPrimary).fontSize(13.0f);

    if (!tooltip.empty()) {
        row->tooltip(tooltip);
        row->addClass("settings-help");
    }

    return row;
}

void SettingsSection::setupRowBinding(
    const std::string& key,
    Panel* row,
    Text* lbl,
    UIElement* ctrl,
    Button* resetBtn,
    std::function<void(const SettingValue&)> updateWidget
) {
    BoundRow bRow;
    bRow.key = key;
    bRow.rowPanel = row;
    bRow.labelText = lbl;
    bRow.controlWidget = ctrl;
    bRow.resetBtn = resetBtn;
    bRow.updateWidget = std::move(updateWidget);

    auto& store = SettingsStore::GetSingleton();
    bRow.subscriptionId = store.onChanged(m_modId, key, [this, key](const std::string&, const std::string&, const SettingValue& val) {
        if (m_isUpdatingFromStore) return;
        for (auto& r : m_boundRows) {
            if (r.key == key && r.updateWidget) {
                m_isUpdatingFromStore = true;
                r.updateWidget(val);
                updateRowModifiedState(r);
                m_isUpdatingFromStore = false;
                break;
            }
        }
        updateConditions();
    });

    updateRowModifiedState(bRow);
    m_boundRows.push_back(std::move(bRow));
    updateConditions();
}

void SettingsSection::updateRowModifiedState(BoundRow& row) {
    if (m_modId.empty() || row.key.empty()) return;
    auto& store = SettingsStore::GetSingleton();
    bool isDef = store.isDefault(m_modId, row.key);

    if (row.rowPanel) {
        if (!isDef) {
            row.rowPanel->addClass("settings-modified");
        } else {
            row.rowPanel->removeClass("settings-modified");
        }
    }

    if (row.resetBtn) {
        row.resetBtn->setVisible(!isDef);
    }
}

void SettingsSection::setRowVisibleIf(const std::string& key, SettingCondition condition) {
    for (auto& r : m_boundRows) {
        if (r.key == key) {
            r.visibleCond = std::move(condition);
            break;
        }
    }
    updateConditions();
}

void SettingsSection::setRowEnabledIf(const std::string& key, SettingCondition condition) {
    for (auto& r : m_boundRows) {
        if (r.key == key) {
            r.enabledCond = std::move(condition);
            break;
        }
    }
    updateConditions();
}

void SettingsSection::updateConditions() {
    auto& store = SettingsStore::GetSingleton();
    for (auto& r : m_boundRows) {
        if (r.visibleCond.has_value() && r.rowPanel) {
            bool vis = r.visibleCond->evaluate(store, m_modId);
            r.rowPanel->setVisible(vis);
        }
        if (r.enabledCond.has_value() && r.rowPanel && r.controlWidget) {
            bool en = r.enabledCond->evaluate(store, m_modId);
            r.controlWidget->setEnabled(en);
            if (en) {
                r.rowPanel->removeClass("settings-disabled");
            } else {
                r.rowPanel->addClass("settings-disabled");
            }
        }
    }
}

void SettingsSection::resetAllToDefaults() {
    auto& store = SettingsStore::GetSingleton();
    store.resetAll(m_modId);
    for (auto& r : m_boundRows) {
        updateRowModifiedState(r);
    }
    updateConditions();
}

void SettingsSection::setKeybindConflict(const std::string& key, bool conflict) {
    for (auto& r : m_boundRows) {
        if (r.key == key && r.controlWidget) {
            auto* kb = dynamic_cast<KeybindButton*>(r.controlWidget);
            if (kb) {
                kb->setConflict(conflict);
            }
            break;
        }
    }
}

// ---------------- Legacy callback overloads ----------------

Checkbox* SettingsSection::addToggle(
    std::string label,
    bool initialValue,
    std::string tooltip,
    std::function<void(bool)> onChange
) {
    auto* row = createRow(label, tooltip);
    auto* cb = row->add<Checkbox>("");
    cb->addClass("settings-control");
    cb->checked(initialValue);
    if (onChange) {
        cb->onToggle(std::move(onChange));
    }
    return cb;
}

Slider* SettingsSection::addSlider(
    std::string label,
    float min,
    float max,
    float initialValue,
    std::string tooltip,
    std::function<void(float)> onChange,
    std::string format
) {
    auto* row = createRow(label, tooltip);

    auto* right = row->add<Panel>("SliderContainer");
    right->addClass("settings-control");
    right->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    right->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(10.0f);

    auto* sl = right->add<Slider>(initialValue, min, max);
    sl->layout().width(150.0f).height(24.0f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), format.c_str(), initialValue);

    auto* valText = right->add<Text>(buf);
    valText->layout().width(45.0f);
    valText->color(Color::TextSecondary()).fontSize(12.0f);

    sl->onValueChanged([valText, format, onChange = std::move(onChange)](float val) {
        char b[32];
        std::snprintf(b, sizeof(b), format.c_str(), val);
        valText->text(b);
        if (onChange) {
            onChange(val);
        }
    });

    return sl;
}

ComboBox* SettingsSection::addChoice(
    std::string label,
    std::vector<std::string> options,
    size_t initialIndex,
    std::string tooltip,
    std::function<void(size_t, const std::string&)> onChange
) {
    auto* row = createRow(label, tooltip);
    auto* combo = row->add<ComboBox>(std::move(options), initialIndex);
    combo->addClass("settings-control");
    combo->layout().width(180.0f);
    if (onChange) {
        combo->onSelectionChanged(std::move(onChange));
    }
    return combo;
}

TextInput* SettingsSection::addTextInput(
    std::string label,
    std::string placeholder,
    std::string initialValue,
    std::string tooltip,
    std::function<void(const std::string&)> onChange
) {
    auto* row = createRow(label, tooltip);
    auto* input = row->add<TextInput>(std::move(placeholder));
    input->addClass("settings-control");
    input->text(std::move(initialValue));
    input->layout().width(180.0f).height(28.0f);
    if (onChange) {
        input->onTextChanged(std::move(onChange));
    }
    return input;
}

Button* SettingsSection::addButton(
    std::string label,
    std::string buttonText,
    std::string tooltip,
    std::function<void()> onClick
) {
    auto* row = createRow(label, tooltip);
    Button* btn = row->add<Button>(std::move(buttonText));
    btn->addClass("settings-control");
    btn->layout().padding(14.0f, 6.0f);
    btn->fontSize(12.0f);
    if (onClick) {
        btn->onClick(std::move(onClick));
    }
    return btn;
}

Button* SettingsSection::addKeybind(
    std::string label,
    uint32_t currentKey,
    std::string tooltip,
    std::function<void(uint32_t)> onRebind
) {
    auto* row = createRow(label, tooltip);
    auto* btn = row->add<KeybindButton>(currentKey, std::move(onRebind));
    btn->addClass("settings-control");
    btn->layout().padding(14.0f, 6.0f);
    btn->fontSize(12.0f);
    return btn;
}

// ---------------- Store-bound overloads ----------------

Checkbox* SettingsSection::addToggle(std::string label, std::string key, std::string tooltip) {
    auto* row = createRow(label, tooltip, key);

    auto* right = row->add<Panel>("ControlWrapper");
    right->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    right->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(8.0f);

    auto* cb = right->add<Checkbox>("");
    cb->addClass("settings-control");

    auto* resetBtn = right->add<Button>("↺");
    resetBtn->addClass("settings-reset-btn");
    resetBtn->tooltip("Reset to default");
    resetBtn->layout().padding(6.0f, 2.0f);
    resetBtn->fontSize(11.0f);

    auto& store = SettingsStore::GetSingleton();
    bool curVal = store.getBool(m_modId, key);
    cb->checked(curVal);

    cb->onToggle([this, key](bool val) {
        if (!m_isUpdatingFromStore) {
            auto& s = SettingsStore::GetSingleton();
            s.setBool(m_modId, key, val);
            for (auto& r : m_boundRows) {
                if (r.key == key) {
                    updateRowModifiedState(r);
                    break;
                }
            }
            updateConditions();
        }
    });

    resetBtn->onClick([this, key]() {
        auto& s = SettingsStore::GetSingleton();
        s.reset(m_modId, key);
    });

    setupRowBinding(key, row, nullptr, cb, resetBtn, [cb](const SettingValue& v) {
        cb->checked(SettingValueToBool(v));
    });

    return cb;
}

Slider* SettingsSection::addSlider(std::string label, std::string key, std::string tooltip) {
    auto* row = createRow(label, tooltip, key);

    auto* right = row->add<Panel>("ControlWrapper");
    right->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    right->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(8.0f);

    auto& store = SettingsStore::GetSingleton();
    const auto* def = store.getDefinition(m_modId, key);
    float minVal = def && def->minValue.has_value() ? *def->minValue : 0.0f;
    float maxVal = def && def->maxValue.has_value() ? *def->maxValue : 100.0f;
    std::string format = def && !def->formatString.empty() ? def->formatString : "%.2f";

    float curVal = store.getFloat(m_modId, key);

    auto* sl = right->add<Slider>(curVal, minVal, maxVal);
    sl->addClass("settings-control");
    sl->layout().width(150.0f).height(24.0f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), format.c_str(), curVal);
    auto* valText = right->add<Text>(buf);
    valText->layout().width(45.0f);
    valText->color(Color::TextSecondary()).fontSize(12.0f);

    auto* resetBtn = right->add<Button>("↺");
    resetBtn->addClass("settings-reset-btn");
    resetBtn->tooltip("Reset to default");
    resetBtn->layout().padding(6.0f, 2.0f);
    resetBtn->fontSize(11.0f);

    sl->onValueChanged([this, key, valText, format](float val) {
        char b[32];
        std::snprintf(b, sizeof(b), format.c_str(), val);
        valText->text(b);

        if (!m_isUpdatingFromStore) {
            auto& s = SettingsStore::GetSingleton();
            s.setFloat(m_modId, key, val);
            for (auto& r : m_boundRows) {
                if (r.key == key) {
                    updateRowModifiedState(r);
                    break;
                }
            }
            updateConditions();
        }
    });

    resetBtn->onClick([this, key]() {
        auto& s = SettingsStore::GetSingleton();
        s.reset(m_modId, key);
    });

    setupRowBinding(key, row, nullptr, sl, resetBtn, [sl, valText, format](const SettingValue& v) {
        float f = SettingValueToFloat(v);
        sl->value(f);
        char b[32];
        std::snprintf(b, sizeof(b), format.c_str(), f);
        valText->text(b);
    });

    return sl;
}

Slider* SettingsSection::addStepper(std::string label, std::string key, std::string tooltip) {
    return addSlider(std::move(label), std::move(key), std::move(tooltip));
}

ComboBox* SettingsSection::addChoice(std::string label, std::string key, std::string tooltip) {
    auto* row = createRow(label, tooltip, key);

    auto* right = row->add<Panel>("ControlWrapper");
    right->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    right->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(8.0f);

    auto& store = SettingsStore::GetSingleton();
    const auto* def = store.getDefinition(m_modId, key);

    std::vector<std::string> displayOptions;
    std::vector<std::string> optionIds;
    size_t initialIdx = 0;
    std::string curVal = store.getString(m_modId, key);

    if (def && !def->options.empty()) {
        for (size_t i = 0; i < def->options.size(); ++i) {
            optionIds.push_back(def->options[i].id);
            displayOptions.push_back(def->options[i].label);
            if (def->options[i].id == curVal) {
                initialIdx = i;
            }
        }
    } else {
        displayOptions.push_back(curVal.empty() ? "None" : curVal);
        optionIds.push_back(curVal);
    }

    auto* combo = right->add<ComboBox>(displayOptions, initialIdx);
    combo->addClass("settings-control");
    combo->layout().width(180.0f);

    auto* resetBtn = right->add<Button>("↺");
    resetBtn->addClass("settings-reset-btn");
    resetBtn->tooltip("Reset to default");
    resetBtn->layout().padding(6.0f, 2.0f);
    resetBtn->fontSize(11.0f);

    combo->onSelectionChanged([this, key, optionIds](size_t idx, const std::string&) {
        if (!m_isUpdatingFromStore) {
            std::string id = idx < optionIds.size() ? optionIds[idx] : "";
            auto& s = SettingsStore::GetSingleton();
            s.setString(m_modId, key, id);
            for (auto& r : m_boundRows) {
                if (r.key == key) {
                    updateRowModifiedState(r);
                    break;
                }
            }
            updateConditions();
        }
    });

    resetBtn->onClick([this, key]() {
        auto& s = SettingsStore::GetSingleton();
        s.reset(m_modId, key);
    });

    setupRowBinding(key, row, nullptr, combo, resetBtn, [combo, optionIds](const SettingValue& v) {
        std::string s = SettingValueToString(v);
        for (size_t i = 0; i < optionIds.size(); ++i) {
            if (optionIds[i] == s) {
                combo->selectIndex(i);
                break;
            }
        }
    });

    return combo;
}

ComboBox* SettingsSection::addMenu(std::string label, std::string key, std::string tooltip) {
    return addChoice(std::move(label), std::move(key), std::move(tooltip));
}

TextInput* SettingsSection::addTextInput(std::string label, std::string key, std::string placeholder, std::string tooltip) {
    auto* row = createRow(label, tooltip, key);

    auto* right = row->add<Panel>("ControlWrapper");
    right->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    right->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(8.0f);

    auto& store = SettingsStore::GetSingleton();
    std::string curVal = store.getString(m_modId, key);

    auto* input = right->add<TextInput>(std::move(placeholder));
    input->addClass("settings-control");
    input->text(curVal);
    input->layout().width(180.0f).height(28.0f);

    auto* resetBtn = right->add<Button>("↺");
    resetBtn->addClass("settings-reset-btn");
    resetBtn->tooltip("Reset to default");
    resetBtn->layout().padding(6.0f, 2.0f);
    resetBtn->fontSize(11.0f);

    input->onTextChanged([this, key](const std::string& val) {
        if (!m_isUpdatingFromStore) {
            auto& s = SettingsStore::GetSingleton();
            s.setString(m_modId, key, val);
            for (auto& r : m_boundRows) {
                if (r.key == key) {
                    updateRowModifiedState(r);
                    break;
                }
            }
            updateConditions();
        }
    });

    resetBtn->onClick([this, key]() {
        auto& s = SettingsStore::GetSingleton();
        s.reset(m_modId, key);
    });

    setupRowBinding(key, row, nullptr, input, resetBtn, [input](const SettingValue& v) {
        input->text(SettingValueToString(v));
    });

    return input;
}

Button* SettingsSection::addKeybind(std::string label, std::string key, std::string tooltip) {
    auto* row = createRow(label, tooltip, key);

    auto* right = row->add<Panel>("ControlWrapper");
    right->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    right->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(8.0f);

    auto& store = SettingsStore::GetSingleton();
    uint32_t curKey = store.getKeyCode(m_modId, key);

    auto* btn = right->add<KeybindButton>(curKey, [this, key](uint32_t newKey) {
        if (!m_isUpdatingFromStore) {
            auto& s = SettingsStore::GetSingleton();
            s.setKeyCode(m_modId, key, newKey);
            for (auto& r : m_boundRows) {
                if (r.key == key) {
                    updateRowModifiedState(r);
                    break;
                }
            }
            updateConditions();
        }
    });
    btn->addClass("settings-control");
    btn->layout().padding(14.0f, 6.0f);
    btn->fontSize(12.0f);

    auto* resetBtn = right->add<Button>("↺");
    resetBtn->addClass("settings-reset-btn");
    resetBtn->tooltip("Reset to default");
    resetBtn->layout().padding(6.0f, 2.0f);
    resetBtn->fontSize(11.0f);

    resetBtn->onClick([this, key]() {
        auto& s = SettingsStore::GetSingleton();
        s.reset(m_modId, key);
    });

    setupRowBinding(key, row, nullptr, btn, resetBtn, [btn](const SettingValue& v) {
        btn->setKey(SettingValueToKeyCode(v));
    });

    return btn;
}

UIElement* SettingsSection::addColor(std::string label, std::string key, std::string tooltip) {
    auto* row = createRow(label, tooltip, key);

    auto* right = row->add<Panel>("ControlWrapper");
    right->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    right->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(8.0f);

    auto& store = SettingsStore::GetSingleton();
    Color curColor = store.getColor(m_modId, key);

    auto* colorBtn = right->add<ColorButton>(curColor, [this, key](Color newC) {
        if (!m_isUpdatingFromStore) {
            auto& s = SettingsStore::GetSingleton();
            s.setColor(m_modId, key, newC);
            for (auto& r : m_boundRows) {
                if (r.key == key) {
                    updateRowModifiedState(r);
                    break;
                }
            }
            updateConditions();
        }
    });
    colorBtn->addClass("settings-control");

    auto* resetBtn = right->add<Button>("↺");
    resetBtn->addClass("settings-reset-btn");
    resetBtn->tooltip("Reset to default");
    resetBtn->layout().padding(6.0f, 2.0f);
    resetBtn->fontSize(11.0f);

    resetBtn->onClick([this, key]() {
        auto& s = SettingsStore::GetSingleton();
        s.reset(m_modId, key);
    });

    setupRowBinding(key, row, nullptr, colorBtn, resetBtn, [colorBtn](const SettingValue& v) {
        colorBtn->setColor(SettingValueToColor(v));
    });

    return colorBtn;
}

Text* SettingsSection::addHeader(std::string text) {
    auto* h = m_itemsContainer->add<Text>(std::move(text));
    h->addClass("settings-header");
    h->color(Color::TextAccent()).fontSize(13.0f).bold(true);
    h->layout().margin(0.0f, 6.0f, 2.0f, 0.0f);
    return h;
}

Text* SettingsSection::addText(std::string text, std::string tooltip) {
    auto* t = m_itemsContainer->add<Text>(std::move(text));
    t->addClass("settings-help");
    t->color(Color::TextSecondary()).fontSize(12.0f);
    if (!tooltip.empty()) {
        t->tooltip(std::move(tooltip));
    }
    return t;
}

Panel* SettingsSection::addSeparator() {
    auto* sep = m_itemsContainer->add<Panel>("Separator");
    sep->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    sep->layout().height(1.0f).width(DimensionConstraint::Flex(1.0f)).margin(0.0f, 4.0f, 4.0f, 0.0f);
    return sep;
}

Panel* SettingsSection::addEmpty(float height) {
    auto* empty = m_itemsContainer->add<Panel>("Empty");
    empty->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    empty->layout().height(height).width(DimensionConstraint::Flex(1.0f));
    return empty;
}

Button* SettingsSection::addResetSectionButton(std::string label) {
    auto* btn = m_itemsContainer->add<Button>(std::move(label));
    btn->addClass("settings-reset-section-btn");
    btn->layout().padding(14.0f, 6.0f).alignment(Alignment::End);
    btn->fontSize(12.0f);
    btn->onClick([this]() {
        resetAllToDefaults();
    });
    return btn;
}

// ==================== SettingsView ====================

SettingsView::SettingsView(std::string name, std::string modId)
    : ScrollView(std::move(name))
    , m_modId(std::move(modId))
{
    addClass("settings-page");
    layout().direction(LayoutDirection::Vertical)
            .gap(12.0f)
            .padding(16.0f)
            .width(DimensionConstraint::Flex(1.0f));
}

void SettingsView::setModId(std::string modId) {
    m_modId = std::move(modId);
    for (auto* sec : m_sections) {
        if (sec) {
            sec->setModId(m_modId);
        }
    }
}

SettingsSection* SettingsView::addSection(std::string title) {
    auto* sec = add<SettingsSection>(std::move(title), m_modId);
    m_sections.push_back(sec);
    return sec;
}

void SettingsView::clearSections() {
    clearChildren();
    m_sections.clear();
}

Button* SettingsView::addResetPageButton(std::string label) {
    auto* btn = add<Button>(std::move(label));
    btn->addClass("settings-reset-page-btn");
    btn->layout().padding(16.0f, 8.0f).alignment(Alignment::End);
    btn->fontSize(12.0f);
    btn->onClick([this]() {
        resetAllToDefaults();
    });
    return btn;
}

void SettingsView::resetAllToDefaults() {
    for (auto* sec : m_sections) {
        if (sec) {
            sec->resetAllToDefaults();
        }
    }
}

void SettingsView::updateConditions() {
    for (auto* sec : m_sections) {
        if (sec) {
            sec->updateConditions();
        }
    }
}

} // namespace PerfUI
