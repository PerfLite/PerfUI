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
    std::optional<DimensionConstraint> widthConstraint;
    std::optional<DimensionConstraint> heightConstraint;
    std::optional<float> minWidth;
    std::optional<float> maxWidth;
    std::optional<float> minHeight;
    std::optional<float> maxHeight;
    std::optional<float> flex;
    std::optional<LayoutDirection> direction;
    std::optional<Alignment> alignment;
    std::optional<JustifyContent> justify;
    std::optional<float> gap;
    std::optional<Length> gapLength;
    std::optional<Insets> padding;
    std::optional<LengthInsets> paddingLength;
    std::optional<Insets> margin;
    std::optional<LengthInsets> marginLength;
    std::optional<bool> isAbsolute;
    std::optional<float> left;
    std::optional<float> right;
    std::optional<float> top;
    std::optional<float> bottom;
    std::optional<Length> leftLength;
    std::optional<Length> rightLength;
    std::optional<Length> topLength;
    std::optional<Length> bottomLength;
    std::optional<bool> centerX;
    std::optional<bool> visible;

    // Background & Border
    std::optional<Color> backgroundColor;
    std::optional<Color> borderColor;
    std::optional<float> borderWidth;
    std::optional<Length> borderWidthLength;
    std::optional<float> cornerRadius;
    std::optional<Length> cornerRadiusLength;
    std::optional<bool> shadow;
    std::optional<TextureId> backgroundImage;
    std::optional<Point> bgUv0;
    std::optional<Point> bgUv1;
    std::optional<Insets> bgSlice;
    std::optional<Insets> bgCorner;
    std::optional<std::string> borderImagePath;

    // Outline (CSS Outline)
    std::optional<Color> outlineColor;
    std::optional<float> outlineWidth;
    std::optional<Length> outlineWidthLength;
    std::optional<float> outlineOffset;
    std::optional<Length> outlineOffsetLength;

    // Gradient
    std::optional<LinearGradientDef> backgroundGradient;

    // Text properties (inheritable)
    std::optional<Color> color;
    std::optional<std::string> font;
    std::optional<float> fontSize;
    std::optional<Length> fontSizeLength;
    std::optional<Length> minFontSizeLength;
    std::optional<Length> maxFontSizeLength;
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

    // Transitions & Animations (CSS declarations)
    std::vector<TransitionSpec> transitions;
    std::vector<AnimationSpec> animations;

    bool operator==(const ComputedStyle& other) const = default;

    bool isViewportDependent() const {
        if (leftLength.has_value() && leftLength->isViewportDependent()) return true;
        if (rightLength.has_value() && rightLength->isViewportDependent()) return true;
        if (topLength.has_value() && topLength->isViewportDependent()) return true;
        if (bottomLength.has_value() && bottomLength->isViewportDependent()) return true;
        if (gapLength.has_value() && gapLength->isViewportDependent()) return true;
        if (paddingLength.has_value() && paddingLength->isViewportDependent()) return true;
        if (marginLength.has_value() && marginLength->isViewportDependent()) return true;
        if (borderWidthLength.has_value() && borderWidthLength->isViewportDependent()) return true;
        if (cornerRadiusLength.has_value() && cornerRadiusLength->isViewportDependent()) return true;
        if (outlineWidthLength.has_value() && outlineWidthLength->isViewportDependent()) return true;
        if (outlineOffsetLength.has_value() && outlineOffsetLength->isViewportDependent()) return true;
        if (fontSizeLength.has_value() && fontSizeLength->isViewportDependent()) return true;
        if (minFontSizeLength.has_value() && minFontSizeLength->isViewportDependent()) return true;
        if (maxFontSizeLength.has_value() && maxFontSizeLength->isViewportDependent()) return true;
        if (widthConstraint.has_value() && (widthConstraint->mode == SizeMode::Vw || widthConstraint->mode == SizeMode::Vh || widthConstraint->mode == SizeMode::Vmin || widthConstraint->mode == SizeMode::Vmax)) return true;
        if (heightConstraint.has_value() && (heightConstraint->mode == SizeMode::Vw || heightConstraint->mode == SizeMode::Vh || heightConstraint->mode == SizeMode::Vmin || heightConstraint->mode == SizeMode::Vmax)) return true;
        return false;
    }

    bool hasLayoutDifferences(const ComputedStyle& other) const {
        return width != other.width || height != other.height ||
               widthConstraint != other.widthConstraint || heightConstraint != other.heightConstraint ||
               minWidth != other.minWidth || maxWidth != other.maxWidth ||
               minHeight != other.minHeight || maxHeight != other.maxHeight ||
               flex != other.flex || direction != other.direction ||
               alignment != other.alignment || justify != other.justify ||
               gap != other.gap || gapLength != other.gapLength ||
               padding != other.padding || paddingLength != other.paddingLength ||
               margin != other.margin || marginLength != other.marginLength ||
               isAbsolute != other.isAbsolute || left != other.left || right != other.right ||
               top != other.top || bottom != other.bottom ||
               leftLength != other.leftLength || rightLength != other.rightLength ||
               topLength != other.topLength || bottomLength != other.bottomLength ||
               fontSize != other.fontSize || fontSizeLength != other.fontSizeLength ||
               minFontSizeLength != other.minFontSizeLength || maxFontSizeLength != other.maxFontSizeLength ||
               centerX != other.centerX || visible != other.visible;
    }
};

} // namespace PerfUI
