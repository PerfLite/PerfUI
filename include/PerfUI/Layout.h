#pragma once

#include "Types.h"
#include <algorithm>
#include <optional>

namespace PerfUI {

enum class JustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround
};

struct DimensionConstraint {
    SizeMode mode{ SizeMode::Auto };
    float value{ 0.0f };

    constexpr DimensionConstraint() = default;
    constexpr DimensionConstraint(SizeMode m, float v = 0.0f) : mode(m), value(v) {}

    static constexpr DimensionConstraint Auto() { return { SizeMode::Auto, 0.0f }; }
    static constexpr DimensionConstraint Fixed(float px) { return { SizeMode::Fixed, px }; }
    static constexpr DimensionConstraint Percent(float pct) { return { SizeMode::Percent, pct }; }
    static constexpr DimensionConstraint Flex(float weight = 1.0f) { return { SizeMode::Flex, weight }; }
    static constexpr DimensionConstraint Vw(float vw) { return { SizeMode::Vw, vw }; }
    static constexpr DimensionConstraint Vh(float vh) { return { SizeMode::Vh, vh }; }
    static constexpr DimensionConstraint Vmin(float vmin) { return { SizeMode::Vmin, vmin }; }
    static constexpr DimensionConstraint Vmax(float vmax) { return { SizeMode::Vmax, vmax }; }

    constexpr float resolve(float parentSize, const Dimensions& viewport) const {
        switch (mode) {
        case SizeMode::Fixed: return value;
        case SizeMode::Percent: return parentSize * (value / 100.0f);
        case SizeMode::Flex: return parentSize;
        case SizeMode::Vw: return viewport.width * (value / 100.0f);
        case SizeMode::Vh: return viewport.height * (value / 100.0f);
        case SizeMode::Vmin: return (viewport.width < viewport.height ? viewport.width : viewport.height) * (value / 100.0f);
        case SizeMode::Vmax: return (viewport.width > viewport.height ? viewport.width : viewport.height) * (value / 100.0f);
        default: return -1.0f;
        }
    }

    constexpr bool operator==(const DimensionConstraint&) const = default;
};

class LayoutProps {
public:
    LayoutProps() = default;

    LayoutDirection direction() const { return m_direction; }
    LayoutProps& direction(LayoutDirection dir) { m_direction = dir; return *this; }

    Alignment alignment() const { return m_alignment; }
    LayoutProps& alignment(Alignment align) { m_alignment = align; return *this; }

    JustifyContent justify() const { return m_justify; }
    LayoutProps& justify(JustifyContent just) { m_justify = just; return *this; }

    const Insets& padding() const { return m_padding; }
    LayoutProps& padding(float uniform) { m_padding = Insets(uniform); return *this; }
    LayoutProps& padding(float h, float v) { m_padding = Insets(h, v); return *this; }
    LayoutProps& padding(float l, float t, float r, float b) { m_padding = Insets(l, t, r, b); return *this; }

    const Insets& margin() const { return m_margin; }
    LayoutProps& margin(float uniform) { m_margin = Insets(uniform); return *this; }
    LayoutProps& margin(float h, float v) { m_margin = Insets(h, v); return *this; }
    LayoutProps& margin(float l, float t, float r, float b) { m_margin = Insets(l, t, r, b); return *this; }

    float gap() const { return m_gap; }
    LayoutProps& gap(float gapPx) { m_gap = gapPx; return *this; }

    DimensionConstraint width() const { return m_width; }
    LayoutProps& width(DimensionConstraint w) { m_width = w; return *this; }
    LayoutProps& width(float px) { m_width = DimensionConstraint::Fixed(px); return *this; }

    DimensionConstraint height() const { return m_height; }
    LayoutProps& height(DimensionConstraint h) { m_height = h; return *this; }
    LayoutProps& height(float px) { m_height = DimensionConstraint::Fixed(px); return *this; }

    PERFUI_API std::optional<float> left() const;
    PERFUI_API LayoutProps& left(float px);
    PERFUI_API LayoutProps& left(Length len);
    PERFUI_API std::optional<Length> leftLength() const;

    PERFUI_API std::optional<float> right() const;
    PERFUI_API LayoutProps& right(float px);
    PERFUI_API LayoutProps& right(Length len);
    PERFUI_API std::optional<Length> rightLength() const;

    PERFUI_API std::optional<float> top() const;
    PERFUI_API LayoutProps& top(float px);
    PERFUI_API LayoutProps& top(Length len);
    PERFUI_API std::optional<Length> topLength() const;

    PERFUI_API std::optional<float> bottom() const;
    PERFUI_API LayoutProps& bottom(float px);
    PERFUI_API LayoutProps& bottom(Length len);
    PERFUI_API std::optional<Length> bottomLength() const;

    PERFUI_API bool centerX() const;
    PERFUI_API LayoutProps& centerX(bool enabled);
    PERFUI_API bool hasAbsoluteAnchors() const;
    PERFUI_API void clearAbsoluteAnchors();

    float flexGrow() const { return m_flexGrow; }
    LayoutProps& flex(float weight = 1.0f) {
        m_flexGrow = weight;
        return *this;
    }

    float minWidth() const { return m_minWidth; }
    LayoutProps& minWidth(float minW) { m_minWidth = minW; return *this; }

    float maxWidth() const { return m_maxWidth; }
    LayoutProps& maxWidth(float maxW) { m_maxWidth = maxW; return *this; }

    float minHeight() const { return m_minHeight; }
    LayoutProps& minHeight(float minH) { m_minHeight = minH; return *this; }

    float maxHeight() const { return m_maxHeight; }
    LayoutProps& maxHeight(float maxH) { m_maxHeight = maxH; return *this; }

private:
    LayoutDirection m_direction{ LayoutDirection::Vertical };
    Alignment m_alignment{ Alignment::Stretch };
    JustifyContent m_justify{ JustifyContent::Start };

    Insets m_padding{};
    Insets m_margin{};
    float m_gap{ 0.0f };

    DimensionConstraint m_width{ DimensionConstraint::Auto() };
    DimensionConstraint m_height{ DimensionConstraint::Auto() };
    float m_flexGrow{ 0.0f };

    float m_minWidth{ 0.0f };
    float m_maxWidth{ 100000.0f };
    float m_minHeight{ 0.0f };
    float m_maxHeight{ 100000.0f };
};

namespace Size {
    inline constexpr DimensionConstraint Auto() { return DimensionConstraint::Auto(); }
    inline constexpr DimensionConstraint Fixed(float px) { return DimensionConstraint::Fixed(px); }
    inline constexpr DimensionConstraint Percent(float pct) { return DimensionConstraint::Percent(pct); }
    inline constexpr DimensionConstraint Flex(float weight = 1.0f) { return DimensionConstraint::Flex(weight); }
    inline constexpr DimensionConstraint Vw(float vw) { return DimensionConstraint::Vw(vw); }
    inline constexpr DimensionConstraint Vh(float vh) { return DimensionConstraint::Vh(vh); }
    inline constexpr DimensionConstraint Vmin(float vmin) { return DimensionConstraint::Vmin(vmin); }
    inline constexpr DimensionConstraint Vmax(float vmax) { return DimensionConstraint::Vmax(vmax); }
}

} // namespace PerfUI
