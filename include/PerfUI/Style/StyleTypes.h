#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <compare>
#include <optional>
#include "PerfUI/Types.h"
#include "PerfUI/Animation.h"

namespace PerfUI {

enum class PseudoClass : uint8_t {
    None       = 0,
    Hover      = 1 << 0,
    Active     = 1 << 1,
    Focus      = 1 << 2,
    Disabled   = 1 << 3,
    Checked    = 1 << 4,
    FirstChild = 1 << 5,
    LastChild  = 1 << 6
};

inline constexpr PseudoClass operator|(PseudoClass a, PseudoClass b) {
    return static_cast<PseudoClass>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline constexpr PseudoClass operator&(PseudoClass a, PseudoClass b) {
    return static_cast<PseudoClass>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline constexpr bool hasPseudoClass(uint8_t mask, PseudoClass pc) {
    return (mask & static_cast<uint8_t>(pc)) == static_cast<uint8_t>(pc);
}

struct Specificity {
    uint32_t idCount{ 0 };
    uint32_t classAndPseudoCount{ 0 };
    uint32_t tagCount{ 0 };

    constexpr auto operator<=>(const Specificity&) const = default;
};

enum class Combinator : uint8_t {
    None,
    Descendant, // 'A B'
    Child       // 'A > B'
};

struct SimpleSelector {
    std::string tag;                  // e.g. "Panel", "*" or empty
    std::string id;                   // e.g. "my_id" or empty
    std::vector<std::string> classes; // e.g. ["card", "primary"]
    uint8_t pseudoClasses{ 0 };       // bitmask of PseudoClass
};

struct ComplexSelector {
    struct Component {
        SimpleSelector selector;
        Combinator combinator{ Combinator::None };
    };

    std::vector<Component> components;
    Specificity specificity;
};

struct StyleDeclaration {
    std::unordered_map<std::string, std::string> properties;
};

struct StyleRule {
    ComplexSelector selector;
    StyleDeclaration declaration;
    uint32_t sourceOrder{ 0 };
};

enum class TransitionProperty : uint8_t {
    All,
    Background,
    BorderColor,
    BorderWidth,
    CornerRadius,
    Color,
    Opacity,
    TransformScale,
    TransformOffset,
    TransformOffsetX,
    TransformOffsetY,
    OutlineColor,
    OutlineWidth,
    OutlineOffset,
    Width,
    Height,
    None
};

enum class TransitionTimingType : uint8_t {
    Easing,
    Spring
};

struct SpringConfig {
    float stiffness{ 180.0f };
    float damping{ 18.0f };
    float mass{ 1.0f };

    bool operator==(const SpringConfig&) const = default;
};

struct TransitionSpec {
    TransitionProperty property{ TransitionProperty::All };
    TransitionTimingType timingType{ TransitionTimingType::Easing };
    float duration{ 0.0f }; // in seconds
    EasingDef easing{ Easing::EaseOutCubic };
    SpringConfig spring{};
    float delay{ 0.0f };    // in seconds

    bool operator==(const TransitionSpec&) const = default;
};

enum class GradientDirection : uint8_t {
    ToBottom,
    ToTop,
    ToRight,
    ToLeft
};

struct LinearGradientDef {
    GradientDirection direction{ GradientDirection::ToBottom };
    Color startColor{ Color::Transparent() };
    Color endColor{ Color::Transparent() };
    std::optional<Color> midColor;

    bool operator==(const LinearGradientDef&) const = default;
};

} // namespace PerfUI
