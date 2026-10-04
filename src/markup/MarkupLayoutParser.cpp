#include "MarkupLayoutParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace PerfUI::MarkupInternal {

static std::string trim(std::string_view sv) {
    size_t first = 0;
    while (first < sv.size() && std::isspace(static_cast<unsigned char>(sv[first]))) {
        ++first;
    }
    size_t last = sv.size();
    while (last > first && std::isspace(static_cast<unsigned char>(sv[last - 1]))) {
        --last;
    }
    return std::string(sv.substr(first, last - first));
}

static std::string toLower(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) {
        s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return s;
}

float parseNumberOrToken(std::string_view val, float defaultValue) {
    std::string s = trim(val);
    if (s.empty()) return defaultValue;

    // Theme Metrics Tokens
    if (s == "$spacingXS") return 4.0f;
    if (s == "$spacingSM") return 8.0f;
    if (s == "$spacingMD") return 16.0f;
    if (s == "$spacingLG") return 24.0f;
    if (s == "$spacingXL") return 32.0f;

    if (s == "$radiusSmall") return 4.0f;
    if (s == "$radiusMedium") return 8.0f;
    if (s == "$radiusLarge") return 12.0f;

    // Theme Typography Tokens
    if (s == "$fontSmall" || s == "small") return 12.0f;
    if (s == "$fontBody" || s == "body") return 14.0f;
    if (s == "$fontMedium" || s == "medium") return 16.0f;
    if (s == "$fontTitle" || s == "title") return 20.0f;
    if (s == "$fontHeader" || s == "header") return 26.0f;

    char* endPtr = nullptr;
    float res = std::strtof(s.c_str(), &endPtr);
    if (endPtr != s.c_str()) {
        return res;
    }
    return defaultValue;
}

static uint8_t parseHexByte(std::string_view s) {
    char buf[3] = { s[0], s[1], '\0' };
    return static_cast<uint8_t>(std::strtoul(buf, nullptr, 16));
}

static uint8_t parseHexNibble(char c) {
    char buf[2] = { c, '\0' };
    unsigned long n = std::strtoul(buf, nullptr, 16);
    return static_cast<uint8_t>(n * 17);
}

Color parseColorOrToken(std::string_view val, Color defaultColor) {
    std::string s = trim(val);
    if (s.empty()) return defaultColor;

    if (s[0] == '#') {
        if (s.size() == 7) { // #RRGGBB
            uint8_t r = parseHexByte(std::string_view(s.data() + 1, 2));
            uint8_t g = parseHexByte(std::string_view(s.data() + 3, 2));
            uint8_t b = parseHexByte(std::string_view(s.data() + 5, 2));
            return Color(r, g, b, 255);
        } else if (s.size() == 9) { // #RRGGBBAA
            uint8_t r = parseHexByte(std::string_view(s.data() + 1, 2));
            uint8_t g = parseHexByte(std::string_view(s.data() + 3, 2));
            uint8_t b = parseHexByte(std::string_view(s.data() + 5, 2));
            uint8_t a = parseHexByte(std::string_view(s.data() + 7, 2));
            return Color(r, g, b, a);
        } else if (s.size() == 4) { // #RGB
            uint8_t r = parseHexNibble(s[1]);
            uint8_t g = parseHexNibble(s[2]);
            uint8_t b = parseHexNibble(s[3]);
            return Color(r, g, b, 255);
        } else if (s.size() == 5) { // #RGBA
            uint8_t r = parseHexNibble(s[1]);
            uint8_t g = parseHexNibble(s[2]);
            uint8_t b = parseHexNibble(s[3]);
            uint8_t a = parseHexNibble(s[4]);
            return Color(r, g, b, a);
        }
    }

    // Theme Color Tokens
    if (s == "$surfaceBase") return Color(13, 17, 23, 235);
    if (s == "$surfaceElevated") return Color(22, 27, 34, 220);
    if (s == "$surfaceActive") return Color(33, 38, 45, 245);
    if (s == "$borderSubtle") return Color::BorderSubtle();
    if (s == "$borderStrong") return Color::BorderStrong();
    if (s == "$borderFocus") return Color::BorderFocus();
    if (s == "$accent" || s == "$nordicGold") return Color::NordicGold();
    if (s == "$textPrimary") return Color::TextPrimary();
    if (s == "$textSecondary") return Color::TextSecondary();
    if (s == "$textMuted") return Color(90, 100, 115, 255);
    if (s == "$white") return Color::White();
    if (s == "$black") return Color::Black();
    if (s == "$transparent") return Color::Transparent();

    std::string lower = toLower(s);
    if (lower == "white") return Color::White();
    if (lower == "black") return Color::Black();
    if (lower == "transparent") return Color::Transparent();
    if (lower == "gold") return Color::NordicGold();

    return defaultColor;
}

bool parseBool(std::string_view val, bool defaultValue) {
    std::string s = toLower(trim(val));
    if (s == "true" || s == "1" || s == "yes" || s == "on") return true;
    if (s == "false" || s == "0" || s == "no" || s == "off") return false;
    return defaultValue;
}

DimensionConstraint parseDimension(std::string_view val) {
    std::string s = trim(val);
    if (s.empty() || s == "auto") {
        return DimensionConstraint::Auto();
    }
    if (s == "flex") {
        return DimensionConstraint::Flex(1.0f);
    }
    if (s.rfind("flex:", 0) == 0) {
        float weight = parseNumberOrToken(s.substr(5), 1.0f);
        return DimensionConstraint::Flex(weight);
    }
    if (!s.empty() && s.back() == '%') {
        float pct = parseNumberOrToken(s.substr(0, s.size() - 1), 0.0f);
        return DimensionConstraint::Percent(pct);
    }
    float px = parseNumberOrToken(s, 0.0f);
    return DimensionConstraint::Fixed(px);
}

void parsePaddingOrMargin(std::string_view val, LayoutProps& props, bool isPadding) {
    std::string s = trim(val);
    std::replace(s.begin(), s.end(), ',', ' ');
    std::stringstream ss(s);
    std::vector<std::string> tokens;
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() == 1) {
        float v0 = parseNumberOrToken(tokens[0]);
        if (isPadding) props.padding(v0);
        else props.margin(v0);
    } else if (tokens.size() == 2) {
        float v0 = parseNumberOrToken(tokens[0]);
        float v1 = parseNumberOrToken(tokens[1]);
        if (isPadding) props.padding(v0, v1);
        else props.margin(v0, v1);
    } else if (tokens.size() >= 4) {
        float v0 = parseNumberOrToken(tokens[0]);
        float v1 = parseNumberOrToken(tokens[1]);
        float v2 = parseNumberOrToken(tokens[2]);
        float v3 = parseNumberOrToken(tokens[3]);
        if (isPadding) props.padding(v0, v1, v2, v3);
        else props.margin(v0, v1, v2, v3);
    }
}

bool isCommonLayoutAttribute(const std::string& key) {
    static const std::unordered_map<std::string, bool> s_keys = {
        { "name", true },
        { "direction", true },
        { "align", true },
        { "justify", true },
        { "width", true },
        { "height", true },
        { "flex", true },
        { "padding", true },
        { "margin", true },
        { "gap", true },
        { "min-width", true },
        { "max-width", true },
        { "min-height", true },
        { "max-height", true },
        { "visible", true },
        { "enabled", true },
        { "focusable", true },
        { "tooltip", true },
        { "class", true },
        { "bind", true }
    };
    return s_keys.find(key) != s_keys.end();
}

bool applyCommonLayoutAttribute(UIElement* element, const std::string& key, const std::string& val) {
    if (!element) return false;
    auto& props = element->layout();

    if (key == "direction") {
        std::string lower = toLower(trim(val));
        if (lower == "row" || lower == "horizontal") props.direction(LayoutDirection::Horizontal);
        else if (lower == "column" || lower == "vertical") props.direction(LayoutDirection::Vertical);
        return true;
    }
    if (key == "align") {
        std::string lower = toLower(trim(val));
        if (lower == "start") props.alignment(Alignment::Start);
        else if (lower == "center") props.alignment(Alignment::Center);
        else if (lower == "end") props.alignment(Alignment::End);
        else if (lower == "stretch") props.alignment(Alignment::Stretch);
        return true;
    }
    if (key == "justify") {
        std::string lower = toLower(trim(val));
        if (lower == "start") props.justify(JustifyContent::Start);
        else if (lower == "center") props.justify(JustifyContent::Center);
        else if (lower == "end") props.justify(JustifyContent::End);
        else if (lower == "space-between") props.justify(JustifyContent::SpaceBetween);
        else if (lower == "space-around") props.justify(JustifyContent::SpaceAround);
        return true;
    }
    if (key == "width") {
        props.width(parseDimension(val));
        return true;
    }
    if (key == "height") {
        props.height(parseDimension(val));
        return true;
    }
    if (key == "flex") {
        props.flex(parseNumberOrToken(val, 1.0f));
        return true;
    }
    if (key == "padding") {
        parsePaddingOrMargin(val, props, true);
        return true;
    }
    if (key == "margin") {
        parsePaddingOrMargin(val, props, false);
        return true;
    }
    if (key == "gap") {
        props.gap(parseNumberOrToken(val));
        return true;
    }
    if (key == "min-width") {
        props.minWidth(parseNumberOrToken(val));
        return true;
    }
    if (key == "max-width") {
        props.maxWidth(parseNumberOrToken(val));
        return true;
    }
    if (key == "min-height") {
        props.minHeight(parseNumberOrToken(val));
        return true;
    }
    if (key == "max-height") {
        props.maxHeight(parseNumberOrToken(val));
        return true;
    }
    if (key == "visible") {
        element->setVisible(parseBool(val, true));
        return true;
    }
    if (key == "enabled") {
        element->setEnabled(parseBool(val, true));
        return true;
    }
    if (key == "focusable") {
        element->setFocusable(parseBool(val, true));
        return true;
    }
    if (key == "tooltip") {
        element->tooltip(std::string(val));
        return true;
    }
    if (key == "class") {
        std::string s = trim(val);
        std::stringstream ss(s);
        std::string c;
        while (ss >> c) {
            element->addClass(c);
        }
        return true;
    }
    if (key == "name" || key == "bind") {
        return true;
    }

    return false;
}

} // namespace PerfUI::MarkupInternal
