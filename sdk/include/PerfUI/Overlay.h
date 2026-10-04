#pragma once

#include "Types.h"
#include "UIRenderBackend.h"
#include "Export.h"
#include <functional>
#include <string>
#include <cstdint>

namespace PerfUI {

using OverlayId = uint32_t;

struct OverlayContext {
    UIRenderBackend& renderer;
    Dimensions screenSize;
    float uiScale{ 1.0f };
    float deltaTime{ 0.0f };
};

using OverlayCallback = std::function<void(OverlayContext&)>;

struct OverlayEntry {
    OverlayId id{ 0 };
    std::string name;
    OverlayCallback callback;
    int zOrder{ 0 };
    bool visible{ true };
    bool alwaysVisible{ true };
    bool enabled{ true };
};

} // namespace PerfUI
