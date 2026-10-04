#pragma once

#include "Types.h"
#include <cstdint>
#include <string_view>
#include <span>

namespace PerfUI {

using TextureId = uint64_t;

struct TextStyle {
    Color color{ Color::White() };
    float fontSize{ 14.0f };
    bool bold{ false };
    bool italic{ false };
    float wrapWidth{ 0.0f };
};

class PERFUI_API UIRenderBackend {
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

    // Primitives for Clean Client Overlays
    virtual void drawLine(const Point& a, const Point& b, Color color, float thickness = 1.0f) {
        (void)a; (void)b; (void)color; (void)thickness;
    }

    virtual void drawPolyline(std::span<const Point> pts, Color color, float thickness = 1.0f, bool closed = false) {
        (void)pts; (void)color; (void)thickness; (void)closed;
    }

    virtual void drawCircle(const Point& center, float radius, Color fill, Color border = Color::Transparent(), float borderWidth = 0.0f) {
        (void)center; (void)radius; (void)fill; (void)border; (void)borderWidth;
    }

    virtual void drawArc(const Point& center, float radius, float startAngle, float endAngle, Color color, float thickness = 1.0f) {
        (void)center; (void)radius; (void)startAngle; (void)endAngle; (void)color; (void)thickness;
    }

    virtual void drawTriangle(const Point& a, const Point& b, const Point& c, Color fill) {
        (void)a; (void)b; (void)c; (void)fill;
    }

    virtual void drawQuad(const Point& a, const Point& b, const Point& c, const Point& d, Color fill) {
        (void)a; (void)b; (void)c; (void)d; (void)fill;
    }

    virtual void drawPolygon(std::span<const Point> pts, Color fill) {
        (void)pts; (void)fill;
    }

    virtual void drawRectOutline(const Rect& rect, Color color, float thickness = 1.0f, float rounding = 0.0f) {
        (void)rect; (void)color; (void)thickness; (void)rounding;
    }

    virtual void drawImageRotated(TextureId tex, const Point& center, const Dimensions& size, float angleRadians, Color tint = Color::White()) {
        (void)tex; (void)center; (void)size; (void)angleRadians; (void)tint;
    }

    virtual void drawImageUV(TextureId tex, const Rect& rect, const Point& uv0, const Point& uv1, Color tint = Color::White()) {
        (void)tex; (void)rect; (void)uv0; (void)uv1; (void)tint;
    }

    virtual void drawImageQuad(TextureId tex, const Point& p1, const Point& p2, const Point& p3, const Point& p4,
                               const Point& uv1 = { 0.0f, 0.0f }, const Point& uv2 = { 1.0f, 0.0f },
                               const Point& uv3 = { 1.0f, 1.0f }, const Point& uv4 = { 0.0f, 1.0f },
                               Color tint = Color::White()) {
        (void)tex; (void)p1; (void)p2; (void)p3; (void)p4; (void)uv1; (void)uv2; (void)uv3; (void)uv4; (void)tint;
    }

    virtual void drawGradientRect(const Rect& rect, Color topLeft, Color topRight, Color bottomRight, Color bottomLeft) {
        (void)rect; (void)topLeft; (void)topRight; (void)bottomRight; (void)bottomLeft;
    }

    // Texture Management API (Stage 2 Clean Client)
    virtual TextureId loadTexture(std::string_view filePath) {
        (void)filePath;
        return 0;
    }

    virtual TextureId createDynamicTexture(uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
        (void)width; (void)height; (void)rgbaPixels;
        return 0;
    }

    virtual bool updateDynamicTexture(TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
        (void)id; (void)width; (void)height; (void)rgbaPixels;
        return false;
    }

    virtual void destroyTexture(TextureId id) {
        (void)id;
    }

    virtual Dimensions getTextureSize(TextureId id) {
        (void)id;
        return Dimensions{ 0.0f, 0.0f };
    }
};

} // namespace PerfUI
