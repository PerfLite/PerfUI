#pragma once

#include "Panel.h"
#include <string>

namespace PerfUI {

/**
 * @brief Top-level retained-mode window container.
 * Inherits from Panel and provides window semantics, theme defaults, and styling.
 */
class UIWindow : public Panel {
public:
    explicit UIWindow(std::string title = "Window", std::string name = "UIWindow")
        : Panel(std::move(name))
        , m_title(std::move(title))
    {
        layout().direction(LayoutDirection::Vertical);
        cornerRadius(6.0f);
        backgroundColor(Color(18, 22, 28, 245));
        borderColor(Color(140, 160, 190, 200));
        borderWidth(1.0f);
        shadow(true, Color(0, 0, 0, 200), 16.0f, { 0.0f, 6.0f });
    }

    ~UIWindow() override = default;

    [[nodiscard]] const std::string& title() const noexcept { return m_title; }
    UIWindow& setTitle(std::string title) {
        m_title = std::move(title);
        markLayoutDirty();
        return *this;
    }

    [[nodiscard]] bool isClosable() const noexcept { return m_closable; }
    UIWindow& setClosable(bool closable) noexcept {
        m_closable = closable;
        return *this;
    }

private:
    std::string m_title;
    bool m_closable{ true };
};

} // namespace PerfUI
