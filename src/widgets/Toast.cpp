#include "PerfUI/Toast.h"
#include <algorithm>

namespace PerfUI {

void ToastManager::show(std::string title, std::string message, ToastType type, float duration) {
    ToastItem item;
    item.title = std::move(title);
    item.message = std::move(message);
    item.type = type;
    item.duration = duration;
    item.timer = 0.0f;
    item.alpha.setTarget(1.0f);
    item.slideOffset.setTarget(0.0f);
    m_toasts.push_back(std::move(item));
}

void ToastManager::update(float deltaTime) {
    for (auto& toast : m_toasts) {
        toast.timer += deltaTime;
        toast.alpha.update(deltaTime);
        toast.slideOffset.update(deltaTime);

        if (toast.timer >= toast.duration && !toast.closing) {
            toast.closing = true;
            toast.alpha.setTarget(0.0f);
            toast.slideOffset.setTarget(30.0f);
        }
    }

    m_toasts.erase(
        std::remove_if(m_toasts.begin(), m_toasts.end(), [](const ToastItem& t) {
            return t.closing && t.alpha.value() <= 0.02f;
        }),
        m_toasts.end()
    );
}

void ToastManager::render(UIRenderBackend& backend, Dimensions viewportSize) {
    if (m_toasts.empty()) return;

    const float toastWidth = 280.0f;
    const float toastHeight = 56.0f;
    const float marginX = 24.0f;
    const float marginY = 24.0f;
    const float stackGap = 10.0f;

    for (size_t i = 0; i < m_toasts.size(); ++i) {
        const auto& toast = m_toasts[i];
        float alpha = toast.alpha.value();
        if (alpha <= 0.01f) continue;

        float x = viewportSize.width - marginX - toastWidth;
        float y = viewportSize.height - marginY - static_cast<float>(i + 1) * (toastHeight + stackGap) + toast.slideOffset.value();

        Rect cardRect(x, y, toastWidth, toastHeight);

        // Soft drop shadow
        backend.drawShadow(cardRect, 6.0f, Color(0, 0, 0, static_cast<uint8_t>(180.0f * alpha)), 16.0f, { 0.0f, 4.0f });

        // Background card
        Color bg(16, 21, 28, static_cast<uint8_t>(245.0f * alpha));
        Color border(60, 70, 85, static_cast<uint8_t>(190.0f * alpha));
        backend.drawRoundedRect(cardRect, bg, 6.0f, border, 1.0f);

        // Accent indicator
        Color accentColor;
        switch (toast.type) {
        case ToastType::Success:
            accentColor = Color(45, 205, 115, static_cast<uint8_t>(255.0f * alpha));
            break;
        case ToastType::Warning:
            accentColor = Color(240, 160, 45, static_cast<uint8_t>(255.0f * alpha));
            break;
        case ToastType::Error:
            accentColor = Color(248, 81, 73, static_cast<uint8_t>(255.0f * alpha));
            break;
        case ToastType::Info:
        default:
            accentColor = Color(212, 175, 55, static_cast<uint8_t>(255.0f * alpha));
            break;
        }

        backend.drawRoundedRect(Rect(x + 4.0f, y + 6.0f, 4.0f, toastHeight - 12.0f), accentColor, 2.0f);

        // Title text
        TextStyle titleStyle;
        titleStyle.fontSize = 13.0f;
        titleStyle.bold = true;
        titleStyle.color = Color(245, 245, 250, static_cast<uint8_t>(255.0f * alpha));
        backend.drawText(toast.title, Point(x + 18.0f, y + 8.0f), titleStyle);

        // Message text
        TextStyle msgStyle;
        msgStyle.fontSize = 11.0f;
        msgStyle.color = Color(160, 170, 185, static_cast<uint8_t>(230.0f * alpha));
        backend.drawText(toast.message, Point(x + 18.0f, y + 28.0f), msgStyle);
    }
}

} // namespace PerfUI
