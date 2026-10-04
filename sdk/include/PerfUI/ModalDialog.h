#pragma once

#include "Panel.h"
#include "Text.h"
#include "Button.h"
#include "Animation.h"
#include <string>
#include <functional>

namespace PerfUI {

class PERFUI_API ModalDialog : public Panel {
public:
    ModalDialog(
        std::string title,
        std::string message,
        std::string confirmText = "Confirm",
        std::string cancelText = "Cancel"
    );
    ~ModalDialog() override = default;

    ModalDialog& onConfirm(std::function<void()> cb) {
        m_onConfirm = std::move(cb);
        return *this;
    }

    ModalDialog& onCancel(std::function<void()> cb) {
        m_onCancel = std::move(cb);
        return *this;
    }

    void open();
    void close();
    bool isOpen() const { return m_isOpen; }
    float alpha() const { return m_scrimAlpha.value(); }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onKeyDown(int keyCode) override;

private:
    std::string m_title;
    std::string m_message;
    std::string m_confirmText;
    std::string m_cancelText;

    std::function<void()> m_onConfirm;
    std::function<void()> m_onCancel;

    bool m_isOpen{ true };
    AnimatedFloat m_scrimAlpha{ 0.0f, 16.0f };

    Panel* m_card{ nullptr };
    Text* m_titleText{ nullptr };
    Text* m_messageText{ nullptr };
    Button* m_cancelBtn{ nullptr };
    Button* m_confirmBtn{ nullptr };
};

} // namespace PerfUI
