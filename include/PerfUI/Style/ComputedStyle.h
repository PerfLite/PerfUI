#pragma once

#include "PerfUI/Types.h"
#include "PerfUI/Layout.h"
#include "PerfUI/Style/StyleTypes.h"
#include <optional>
#include <string>
#include <vector>

namespace PerfUI {

struct ComputedStyle {
    // Layout properties
    std::optional<float> width;
    std::optional<float> height;
    std::optional<float> minWidth;
    std::optional<float> maxWidth;
    std::optional<float> minHeight;
    std::optional<float> maxHeight;
    std::optional<float> flex;
    std::optional<LayoutDirection> direction;
    std::optional<Alignment> alignment;
    std::optional<JustifyContent> justify;
    std::optional<float> gap;
    std::optional<Insets> padding;
    std::optional<Insets> margin;
    std::optional<bool> isAbsolute;
    std::optional<float> left;
    std::optional<float> right;
    std::optional<float> top;
    std::optional<float> bottom;
    std::optional<bool> centerX;
    std::optional<bool> visible;

    // Background & Border
    std::optional<Color> backgroundColor;
    std::optional<Color> borderColor;
    std::optional<float> borderWidth;
    std::optional<float> cornerRadius;
    std::optional<bool> shadow;
    std::optional<TextureId> backgroundImage;
    std::optional<Point> bgUv0;
    std::optional<Point> bgUv1;
    std::optional<Insets> bgSlice;
    std::optional<Insets> bgCorner;

    // Text properties (inheritable)
    std::optional<Color> color;
    std::optional<std::string> font;
    std::optional<float> fontSize;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<float> wrap;

    // Effects & Visual Transforms
    std::optional<float> opacity;
    std::optional<BlendMode> blendMode;
    std::optional<Point> transformOffsetPx;
    std::optional<Point> transformOffsetPct;
    std::optional<bool> hasTransformOffsetPct;
    std::optional<Point> transformScale;
    std::optional<Point> transformPivot;
    std::optional<Color> tint;

    // Widget specific
    std::optional<Color> fillColor;
    std::optional<Color> trackColor;

    // Transitions (CSS transition declarations)
    std::vector<TransitionSpec> transitions;

    bool operator==(const ComputedStyle& other) const = default;

    bool hasLayoutDifferences(const ComputedStyle& other) const {
        return width != other.width || height != other.height ||
               minWidth != other.minWidth || maxWidth != other.maxWidth ||
               minHeight != other.minHeight || maxHeight != other.maxHeight ||
               flex != other.flex || direction != other.direction ||
               alignment != other.alignment || justify != other.justify ||
               gap != other.gap || padding != other.padding || margin != other.margin ||
               isAbsolute != other.isAbsolute || left != other.left || right != other.right ||
               top != other.top || bottom != other.bottom || centerX != other.centerX ||
               visible != other.visible;
    }
};

} // namespace PerfUI
