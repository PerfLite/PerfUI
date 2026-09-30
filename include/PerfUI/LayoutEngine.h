#pragma once

#include "Types.h"
#include "Layout.h"

namespace PerfUI {

class UIElement;

class LayoutEngine {
public:
    static void Measure(UIElement* element, Dimensions availableSize);
    static void Arrange(UIElement* element, const Rect& finalRect);
    static void ArrangeContent(UIElement* element, const Rect& contentRect);

private:
    static void ArrangeHorizontal(UIElement* element, const Rect& contentRect);
    static void ArrangeVertical(UIElement* element, const Rect& contentRect);
};

} // namespace PerfUI
