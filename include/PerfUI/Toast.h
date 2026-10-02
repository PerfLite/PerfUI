#pragma once

#include "Types.h"
#include "Animation.h"
#include "UIRenderBackend.h"
#include <string>
#include <vector>

namespace PerfUI {

enum class ToastType {
    Info,
    Success,
    Warning
};

struct ToastItem {
    std::string title;
    std::string message;
    ToastType type{ ToastType::Info };
    float duration{ 3.2f };
    float timer{ 0.0f };
    AnimatedFloat alpha{ 0.0f, 14.0f };
    AnimatedFloat slideOffset{ 25.0f, 14.0f };
    bool closing{ false };
};

class ToastManager {
public:
    ToastManager() = default;

    void show(std::string title, std::string message, ToastType type = ToastType::Info, float duration = 3.2f);
    void update(float deltaTime);
    void render(UIRenderBackend& backend, Dimensions viewportSize);

    bool hasActiveToasts() const { return !m_toasts.empty(); }

private:
    std::vector<ToastItem> m_toasts;
};

} // namespace PerfUI
