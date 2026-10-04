#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>
#include "Export.h"

namespace PerfUI {

using ElementId = uint64_t;

struct Point {
    float x{ 0.0f };
    float y{ 0.0f };

    constexpr Point() = default;
    constexpr Point(float x_, float y_) : x(x_), y(y_) {}

    constexpr Point operator+(const Point& other) const { return { x + other.x, y + other.y }; }
    constexpr Point operator-(const Point& other) const { return { x - other.x, y - other.y }; }
};

struct Dimensions {
    float width{ 0.0f };
    float height{ 0.0f };

    constexpr Dimensions() = default;
    constexpr Dimensions(float w, float h) : width(w), height(h) {}
};

struct Insets {
    float left{ 0.0f };
    float top{ 0.0f };
    float right{ 0.0f };
    float bottom{ 0.0f };

    constexpr Insets() = default;
    constexpr Insets(float uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    constexpr Insets(float horizontal, float vertical) : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    constexpr Insets(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}

    constexpr float horizontal() const { return left + right; }
    constexpr float vertical() const { return top + bottom; }
};

struct Rect {
    float x{ 0.0f };
    float y{ 0.0f };
    float width{ 0.0f };
    float height{ 0.0f };

    constexpr Rect() = default;
    constexpr Rect(float x_, float y_, float w, float h) : x(x_), y(y_), width(w), height(h) {}
    constexpr Rect(Point pos, Dimensions dim) : x(pos.x), y(pos.y), width(dim.width), height(dim.height) {}

    constexpr Point topLeft() const { return { x, y }; }
    constexpr Point bottomRight() const { return { x + width, y + height }; }

    constexpr bool contains(const Point& pt) const {
        return pt.x >= x && pt.x <= (x + width) && pt.y >= y && pt.y <= (y + height);
    }

    constexpr Rect inset(const Insets& insets) const {
        float newW = width - insets.horizontal();
        float newH = height - insets.vertical();
        return {
            x + insets.left,
            y + insets.top,
            newW > 0.0f ? newW : 0.0f,
            newH > 0.0f ? newH : 0.0f
        };
    }
};

struct Color {
    uint8_t r{ 255 };
    uint8_t g{ 255 };
    uint8_t b{ 255 };
    uint8_t a{ 255 };

    constexpr Color() = default;
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}

    // Basic Colors
    static constexpr Color White()       { return { 255, 255, 255, 255 }; }
    static constexpr Color Black()       { return { 0, 0, 0, 255 }; }
    static constexpr Color Transparent() { return { 0, 0, 0, 0 }; }

    // Nordic Fantasy Theme Tokens (from DESIGN.md)
    static constexpr Color BackgroundBase()     { return { 13, 17, 23, 235 }; }
    static constexpr Color BackgroundElevated() { return { 22, 27, 34, 220 }; }
    static constexpr Color BackgroundActive()   { return { 33, 38, 45, 245 }; }
    static constexpr Color BorderSubtle()       { return { 48, 54, 61, 155 }; }
    static constexpr Color BorderStrong()       { return { 72, 79, 88, 205 }; }
    static constexpr Color BorderFocus()        { return { 212, 175, 55, 255 }; }
    static constexpr Color TextPrimary()        { return { 240, 246, 252, 255 }; }
    static constexpr Color TextSecondary()      { return { 139, 148, 158, 255 }; }
    static constexpr Color TextAccent()         { return { 229, 192, 123, 255 }; }
    static constexpr Color TextSuccess()        { return { 126, 231, 135, 255 }; }
    static constexpr Color TextDisabled()       { return { 72, 79, 88, 255 }; }
    static constexpr Color FocusGlow()          { return { 212, 175, 55, 90 }; }
    static constexpr Color SelectionFill()      { return { 212, 175, 55, 40 }; }

    // Backward compatibility aliases
    static constexpr Color NordicGold()  { return BorderFocus(); }
    static constexpr Color SlateDark()   { return BackgroundBase(); }
    static constexpr Color SlateCard()   { return BackgroundElevated(); }

    static constexpr Color Lerp(const Color& c1, const Color& c2, float t) {
        float f = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        return {
            static_cast<uint8_t>(static_cast<float>(c1.r) + static_cast<float>(c2.r - c1.r) * f),
            static_cast<uint8_t>(static_cast<float>(c1.g) + static_cast<float>(c2.g - c1.g) * f),
            static_cast<uint8_t>(static_cast<float>(c1.b) + static_cast<float>(c2.b - c1.b) * f),
            static_cast<uint8_t>(static_cast<float>(c1.a) + static_cast<float>(c2.a - c1.a) * f)
        };
    }

    constexpr uint32_t toRGBA32() const {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               (static_cast<uint32_t>(r));
    }
};

enum class LayoutDirection {
    Vertical,
    Horizontal
};

enum class Alignment {
    Start,
    Center,
    End,
    Stretch
};

enum class SizeMode {
    Auto,      // Shrink-to-content
    Fixed,     // Explicit pixel count
    Percent,   // Percentage of parent space
    Flex       // Weighted fraction of leftover space
};

struct SizeConstraint {
    SizeMode mode{ SizeMode::Auto };
    float value{ 0.0f };

    static constexpr SizeConstraint Auto() { return { SizeMode::Auto, 0.0f }; }
    static constexpr SizeConstraint Fixed(float px) { return { SizeMode::Fixed, px }; }
    static constexpr SizeConstraint Percent(float pct) { return { SizeMode::Percent, pct }; }
    static constexpr SizeConstraint Flex(float weight = 1.0f) { return { SizeMode::Flex, weight }; }
};

enum class WidgetState {
    Normal,
    Hovered,
    Pressed,
    Focused,
    Disabled,
    Selected
};

enum class NavDirection {
    Up,
    Down,
    Left,
    Right
};

} // namespace PerfUI
