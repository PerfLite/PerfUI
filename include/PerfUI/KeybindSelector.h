#pragma once

#include "UIElement.h"
#include "Types.h"
#include <string>
#include <functional>

namespace PerfUI {

class PERFUI_API KeybindSelector : public UIElement {
public:
    explicit KeybindSelector(uint32_t keyCode = 0, std::string label = "Keybind", std::string name = "KeybindSelector");
    ~KeybindSelector() override = default;

    uint32_t keyCode() const { return m_keyCode; }
    void setKeyCode(uint32_t key);
    KeybindSelector& key(uint32_t k) { setKeyCode(k); return *this; }

    const std::string& label() const { return m_label; }
    KeybindSelector& label(std::string text) { m_label = std::move(text); markLayoutDirty(); return *this; }

    bool isListening() const { return m_listening; }
    void startListening();
    void stopListening();

    KeybindSelector& onKeyChanged(std::function<void(uint32_t)> cb) {
        m_onKeyChanged = std::move(cb);
        return *this;
    }

    using UIElement::style;
    KeybindSelector& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onKeyDown(int keyCode) override;
    bool onAction(NavDirection dir) override;

    static std::string KeyCodeToString(uint32_t keyCode);

private:
    uint32_t m_keyCode{ 0 };
    std::string m_label;
    bool m_listening{ false };
    std::function<void(uint32_t)> m_onKeyChanged;
};

} // namespace PerfUI
