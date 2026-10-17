#pragma once

#include "Types.h"
#include "UIRenderBackend.h"
#include <optional>
#include <string>

namespace PerfUI {

struct ShadowStyle {
    Color color{ 0, 0, 0, 0 };
    float blur{ 0.0f };
    Point offset{ 0.0f, 0.0f };

    constexpr bool operator==(const ShadowStyle& other) const {
        return color.r == other.color.r && color.g == other.color.g &&
               color.b == other.color.b && color.a == other.color.a &&
               blur == other.blur && offset.x == other.offset.x && offset.y == other.offset.y;
    }
};

struct BoxStyle {
    std::optional<Color> backgroundColor;
    std::optional<Color> borderColor;
    std::optional<float> borderWidth;
    std::optional<float> cornerRadius;
    std::optional<ShadowStyle> shadow;
    std::optional<float> opacity;
    TextureId backgroundTexture{ 0 };

    BoxStyle& bg(Color c) { backgroundColor = c; return *this; }
    BoxStyle& border(Color c, float width = 1.0f) { borderColor = c; borderWidth = width; return *this; }
    BoxStyle& radius(float r) { cornerRadius = r; return *this; }
    BoxStyle& dropShadow(Color c, float blur = 16.0f, Point offset = { 0.0f, 4.0f }) {
        shadow = ShadowStyle{ c, blur, offset };
        return *this;
    }
    BoxStyle& opac(float o) { opacity = o; return *this; }
    BoxStyle& texture(TextureId tex) { backgroundTexture = tex; return *this; }

    BoxStyle mergeWith(const BoxStyle& fallback) const {
        BoxStyle res = *this;
        if (!res.backgroundColor.has_value()) res.backgroundColor = fallback.backgroundColor;
        if (!res.borderColor.has_value()) res.borderColor = fallback.borderColor;
        if (!res.borderWidth.has_value()) res.borderWidth = fallback.borderWidth;
        if (!res.cornerRadius.has_value()) res.cornerRadius = fallback.cornerRadius;
        if (!res.shadow.has_value()) res.shadow = fallback.shadow;
        if (!res.opacity.has_value()) res.opacity = fallback.opacity;
        if (res.backgroundTexture == 0) res.backgroundTexture = fallback.backgroundTexture;
        return res;
    }
};

struct TextStyleOverride {
    std::optional<Color> color;
    std::optional<float> fontSize;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<float> wrapWidth;
    std::optional<Color> shadowColor;
    std::optional<Point> shadowOffset;

    TextStyleOverride& col(Color c) { color = c; return *this; }
    TextStyleOverride& size(float s) { fontSize = s; return *this; }
    TextStyleOverride& setBold(bool b = true) { bold = b; return *this; }
    TextStyleOverride& setItalic(bool i = true) { italic = i; return *this; }
    TextStyleOverride& wrap(float w) { wrapWidth = w; return *this; }
    TextStyleOverride& shadow(Color c, Point offset = { 1.0f, 1.0f }) {
        shadowColor = c;
        shadowOffset = offset;
        return *this;
    }

    TextStyle resolve(const TextStyle& fallback) const {
        TextStyle res = fallback;
        if (color.has_value()) res.color = *color;
        if (fontSize.has_value()) res.fontSize = *fontSize;
        if (bold.has_value()) res.bold = *bold;
        if (italic.has_value()) res.italic = *italic;
        if (wrapWidth.has_value()) res.wrapWidth = *wrapWidth;
        return res;
    }

    TextStyleOverride mergeWith(const TextStyleOverride& fallback) const {
        TextStyleOverride res = *this;
        if (!res.color.has_value()) res.color = fallback.color;
        if (!res.fontSize.has_value()) res.fontSize = fallback.fontSize;
        if (!res.bold.has_value()) res.bold = fallback.bold;
        if (!res.italic.has_value()) res.italic = fallback.italic;
        if (!res.wrapWidth.has_value()) res.wrapWidth = fallback.wrapWidth;
        if (!res.shadowColor.has_value()) res.shadowColor = fallback.shadowColor;
        if (!res.shadowOffset.has_value()) res.shadowOffset = fallback.shadowOffset;
        return res;
    }
};

template <typename T>
struct StateStyle {
    std::optional<T> normal;
    std::optional<T> hover;
    std::optional<T> pressed;
    std::optional<T> focused;
    std::optional<T> disabled;
    std::optional<T> selected;

    T resolve(WidgetState state, const T& fallback = T{}) const {
        switch (state) {
        case WidgetState::Pressed:
            if (pressed.has_value()) return *pressed;
            if (hover.has_value()) return *hover;
            if (normal.has_value()) return *normal;
            return fallback;
        case WidgetState::Hovered:
            if (hover.has_value()) return *hover;
            if (normal.has_value()) return *normal;
            return fallback;
        case WidgetState::Focused:
            if (focused.has_value()) return *focused;
            if (hover.has_value()) return *hover;
            if (normal.has_value()) return *normal;
            return fallback;
        case WidgetState::Selected:
            if (selected.has_value()) return *selected;
            if (normal.has_value()) return *normal;
            return fallback;
        case WidgetState::Disabled:
            if (disabled.has_value()) return *disabled;
            if (normal.has_value()) return *normal;
            return fallback;
        case WidgetState::Normal:
        default:
            if (normal.has_value()) return *normal;
            return fallback;
        }
    }

    StateStyle<T> mergeWith(const StateStyle<T>& fallback) const {
        StateStyle<T> r = *this;
        if (!r.normal.has_value()) r.normal = fallback.normal;
        if (!r.hover.has_value()) r.hover = fallback.hover;
        if (!r.pressed.has_value()) r.pressed = fallback.pressed;
        if (!r.focused.has_value()) r.focused = fallback.focused;
        if (!r.disabled.has_value()) r.disabled = fallback.disabled;
        if (!r.selected.has_value()) r.selected = fallback.selected;
        return r;
    }
};

struct WidgetStyle {
    StateStyle<BoxStyle> box;
    StateStyle<TextStyleOverride> text;

    BoxStyle resolveBox(WidgetState state, const BoxStyle& fallback = BoxStyle{}) const {
        return box.resolve(state, fallback);
    }

    TextStyle resolveText(WidgetState state, const TextStyle& fallback = TextStyle{}) const {
        TextStyleOverride ov = text.resolve(state, TextStyleOverride{});
        return ov.resolve(fallback);
    }

    WidgetStyle mergeWith(const WidgetStyle& fallback) const {
        WidgetStyle r;
        r.box = box.mergeWith(fallback.box);
        r.text = text.mergeWith(fallback.text);
        return r;
    }
};

} // namespace PerfUI
