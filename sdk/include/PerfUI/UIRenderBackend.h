#pragma once

#include "Types.h"
#include <cstdint>
#include <string_view>

namespace PerfUI {

using TextureId = uint64_t;

struct TextStyle {
    Color color{ Color::White() };
    float fontSize{ 14.0f };
    bool bold{ false };
    bool italic{ false };
    float wrapWidth{ 0.0f };
};

class UIRenderBackend {
public:
    virtual ~UIRenderBackend() = default;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void pushClipRect(const Rect& rect) = 0;
    virtual void popClipRect() = 0;

    virtual void drawRect(const Rect& rect, Color fillColor) = 0;
    virtual void drawRoundedRect(
        const Rect& rect,
        Color fillColor,
        float radius,
        Color borderColor = Color::Transparent(),
        float borderWidth = 0.0f
    ) = 0;

    virtual void drawText(
        std::string_view text,
        const Point& position,
        const TextStyle& style
    ) = 0;

    virtual Dimensions measureText(
        std::string_view text,
        const TextStyle& style
    ) = 0;

    virtual void drawImage(
        TextureId texture,
        const Rect& rect,
        Color tint = Color::White()
    ) = 0;

    virtual void drawShadow(
        const Rect& rect,
        float radius,
        Color shadowColor,
        float blurRadius,
        const Point& offset
    ) = 0;
};

} // namespace PerfUI
