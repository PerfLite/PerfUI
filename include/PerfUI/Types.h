#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

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
};

struct Color {
    uint8_t r{ 255 };
    uint8_t g{ 255 };
    uint8_t b{ 255 };
    uint8_t a{ 255 };

    constexpr Color() = default;
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr Color White()       { return { 255, 255, 255, 255 }; }
    static constexpr Color Black()       { return { 0, 0, 0, 255 }; }
    static constexpr Color Transparent() { return { 0, 0, 0, 0 }; }
    static constexpr Color NordicGold()  { return { 212, 175, 55, 255 }; }
    static constexpr Color SlateDark()   { return { 13, 17, 23, 240 }; }
    static constexpr Color SlateCard()   { return { 22, 27, 34, 230 }; }

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
