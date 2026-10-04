#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "PerfUI/Types.h"
#include "PerfUI/Layout.h"
#include "PerfUI/UIElement.h"

namespace PerfUI::MarkupInternal {

float parseNumberOrToken(std::string_view val, float defaultValue = 0.0f);
Color parseColorOrToken(std::string_view val, Color defaultColor = Color::White());
bool parseBool(std::string_view val, bool defaultValue = false);
DimensionConstraint parseDimension(std::string_view val);
void parsePaddingOrMargin(std::string_view val, LayoutProps& props, bool isPadding);

bool isCommonLayoutAttribute(const std::string& key);
bool applyCommonLayoutAttribute(UIElement* element, const std::string& key, const std::string& val);

} // namespace PerfUI::MarkupInternal
