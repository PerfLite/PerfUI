#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include <string>
#include <functional>

namespace PerfUI {

enum class NotificationLevel {
    Info,
    Success,
    Warning,
    Error
};

class PERFUI_API Notification : public UIElement {
public:
    explicit Notification(std::string name = "Notification");
    ~Notification() override = default;

    Notification& setTitle(std::string title) {
        m_title = std::move(title);
        return *this;
    }
    const std::string& title() const { return m_title; }

    Notification& setMessage(std::string message) {
        m_message = std::move(message);
        return *this;
    }
    const std::string& message() const { return m_message; }

    Notification& setLevel(NotificationLevel level) {
        m_level = level;
        return *this;
    }
    NotificationLevel level() const { return m_level; }

    Notification& setDuration(float seconds) {
        m_duration = seconds;
        return *this;
    }
    float duration() const { return m_duration; }

    Notification& setClosable(bool closable) {
        m_closable = closable;
        return *this;
    }
    bool isClosable() const { return m_closable; }

    Notification& setAction(std::string actionText, std::function<void()> onAction) {
        m_actionText = std::move(actionText);
        m_onAction = std::move(onAction);
        return *this;
    }

    Notification& onClose(std::function<void()> callback) {
        m_onClose = std::move(callback);
        return *this;
    }

    void dismiss();
    bool isDismissed() const { return m_dismissed; }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;
    void onPointerLeave() override;

private:
    std::string m_title;
    std::string m_message;
    NotificationLevel m_level{ NotificationLevel::Info };
    float m_duration{ 4.0f }; // 0 = persistent
    float m_elapsed{ 0.0f };
    bool m_closable{ true };
    bool m_dismissed{ false };
    bool m_hoveredClose{ false };
    bool m_hoveredAction{ false };

    std::string m_actionText;
    std::function<void()> m_onAction;
    std::function<void()> m_onClose;

    Rect m_closeBtnBounds;
    Rect m_actionBtnBounds;

    AnimatedFloat m_alpha{ 0.0f, 14.0f };
};

} // namespace PerfUI
