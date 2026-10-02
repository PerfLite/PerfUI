#include "PerfUI/SettingsView.h"
#include "PerfUI/UIContext.h"
#include <cstdio>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace PerfUI {

SettingsSection::SettingsSection(std::string title)
    : Panel("SettingsSection_" + title)
    , m_title(std::move(title)) {
    backgroundColor(Color::Transparent());
    borderWidth(0.0f);
    layout().direction(LayoutDirection::Vertical)
            .gap(6.0f)
            .margin(0.0f, 0.0f, 16.0f, 0.0f)
            .width(DimensionConstraint::Flex(1.0f));

    // Section Header
    auto* header = add<Text>(m_title);
    header->color(Color::TextAccent()).fontSize(14.0f).bold(true);

    // Separator line
    auto* sep = add<Panel>("Sep");
    sep->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    sep->layout().height(1.0f).width(DimensionConstraint::Flex(1.0f)).margin(0.0f, 2.0f, 6.0f, 0.0f);

    m_itemsContainer = add<Panel>("ItemsContainer");
    m_itemsContainer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    m_itemsContainer->layout().direction(LayoutDirection::Vertical).gap(6.0f).width(DimensionConstraint::Flex(1.0f));
}

Panel* SettingsSection::createRow(const std::string& label, const std::string& tooltip) {
    auto* row = m_itemsContainer->add<Panel>("SettingRow");
    row->backgroundColor(Color(18, 23, 31, 190));
    row->borderColor(Color::BorderSubtle());
    row->borderWidth(1.0f);
    row->cornerRadius(6.0f);
    row->layout().direction(LayoutDirection::Horizontal)
                 .alignment(Alignment::Center)
                 .justify(JustifyContent::SpaceBetween)
                 .padding(12.0f, 8.0f)
                 .width(DimensionConstraint::Flex(1.0f));

    auto* lbl = row->add<Text>(label);
    lbl->color(Color::TextPrimary()).fontSize(13.0f);

    if (!tooltip.empty()) {
        row->tooltip(tooltip);
    }

    return row;
}

Checkbox* SettingsSection::addToggle(
    std::string label,
    bool initialValue,
    std::string tooltip,
    std::function<void(bool)> onChange
) {
    auto* row = createRow(label, tooltip);
    auto* cb = row->add<Checkbox>("");
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
    btn->layout().padding(14.0f, 6.0f);
    btn->fontSize(12.0f);
    if (onClick) {
        btn->onClick(std::move(onClick));
    }
    return btn;
}

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

    void startListening() {
        m_isListening = true;
        label("[ Press Any Key... ]");
        normalColor(Color(35, 45, 60, 255), Color::NordicGold(), Color::NordicGold());
        if (context()) {
            context()->setFocus(this);
            context()->setCapturingKeybind(true);
        }
        markLayoutDirty();
    }

    void stopListening(bool success) {
        m_isListening = false;
        label(FormatKey(m_key));
        normalColor(Color(22, 28, 38, 220), Color::BorderSubtle(), Color::TextPrimary());
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

Button* SettingsSection::addKeybind(
    std::string label,
    uint32_t currentKey,
    std::string tooltip,
    std::function<void(uint32_t)> onRebind
) {
    auto* row = createRow(label, tooltip);
    auto* btn = row->add<KeybindButton>(currentKey, std::move(onRebind));
    btn->layout().padding(14.0f, 6.0f);
    btn->fontSize(12.0f);
    return btn;
}

SettingsView::SettingsView(std::string name)
    : ScrollView(std::move(name)) {
    layout().direction(LayoutDirection::Vertical)
            .gap(12.0f)
            .padding(16.0f)
            .width(DimensionConstraint::Flex(1.0f));
}

SettingsSection* SettingsView::addSection(std::string title) {
    auto* sec = add<SettingsSection>(std::move(title));
    m_sections.push_back(sec);
    return sec;
}

void SettingsView::clearSections() {
    clearChildren();
    m_sections.clear();
}

} // namespace PerfUI
