#include "MarkupLayoutParser.h"
#include "PerfUI/Localization.h"
#include "PerfUI/Theme.h"
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

    const auto& curTheme = Theme::Current();

    // Theme Metrics Tokens
    if (s == "$spacingXS") return curTheme.metrics.spacingXS;
    if (s == "$spacingSM") return curTheme.metrics.spacingSM;
    if (s == "$spacingMD") return curTheme.metrics.spacingMD;
    if (s == "$spacingLG") return curTheme.metrics.spacingLG;
    if (s == "$spacingXL") return curTheme.metrics.spacingXL;

    if (s == "$radiusSmall") return curTheme.metrics.radiusSmall;
    if (s == "$radiusMedium") return curTheme.metrics.radiusMedium;
    if (s == "$radiusLarge") return curTheme.metrics.radiusLarge;

    // Theme Typography Tokens
    if (s == "$fontSmall" || s == "small") return curTheme.typography.fontSmall;
    if (s == "$fontBody" || s == "body") return curTheme.typography.fontBody;
    if (s == "$fontMedium" || s == "medium") return curTheme.typography.fontMedium;
    if (s == "$fontTitle" || s == "title") return curTheme.typography.fontTitle;
    if (s == "$fontHeader" || s == "header") return curTheme.typography.fontHeader;

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

    std::string lower = toLower(s);

    // CSS rgb(...) and rgba(...) syntax support
    if (lower.rfind("rgb", 0) == 0) {
        auto openParen = s.find('(');
        auto closeParen = s.rfind(')');
        if (openParen != std::string::npos && closeParen != std::string::npos && closeParen > openParen) {
            std::string_view inner(s.data() + openParen + 1, closeParen - openParen - 1);
            float vals[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            size_t count = 0;
            const char* ptr = inner.data();
            const char* end = ptr + inner.size();
            while (ptr < end && count < 4) {
                while (ptr < end && (*ptr == ' ' || *ptr == ',' || *ptr == '\t' || *ptr == '/')) ++ptr;
                if (ptr >= end) break;
                char* next = nullptr;
                float v = std::strtof(ptr, &next);
                if (next == ptr) break;
                vals[count++] = v;
                ptr = next;
            }
            if (count >= 3) {
                uint8_t r = static_cast<uint8_t>(std::clamp(vals[0], 0.0f, 255.0f));
                uint8_t g = static_cast<uint8_t>(std::clamp(vals[1], 0.0f, 255.0f));
                uint8_t b = static_cast<uint8_t>(std::clamp(vals[2], 0.0f, 255.0f));
                uint8_t a = 255;
                if (count >= 4) {
                    if (vals[3] <= 1.0f && vals[3] >= 0.0f) {
                        a = static_cast<uint8_t>(vals[3] * 255.0f + 0.5f);
                    } else {
                        a = static_cast<uint8_t>(std::clamp(vals[3], 0.0f, 255.0f));
                    }
                }
                return Color(r, g, b, a);
            }
        }
    }

    // Theme Color Tokens
    const auto& curTheme = Theme::Current();
    if (s == "$surfaceBase") return curTheme.colors.surfaceBase;
    if (s == "$surfaceElevated") return curTheme.colors.surfaceElevated;
    if (s == "$surfaceActive") return curTheme.colors.surfaceActive;
    if (s == "$borderSubtle") return curTheme.colors.borderSubtle;
    if (s == "$borderStrong") return curTheme.colors.borderStrong;
    if (s == "$borderFocus") return curTheme.colors.borderFocus;
    if (s == "$accent" || s == "$nordicGold") return curTheme.colors.textAccent;
    if (s == "$textPrimary") return curTheme.colors.textPrimary;
    if (s == "$textSecondary") return curTheme.colors.textSecondary;
    if (s == "$textAccent") return curTheme.colors.textAccent;
    if (s == "$textSuccess") return curTheme.colors.textSuccess;
    if (s == "$textDisabled") return curTheme.colors.textDisabled;
    if (s == "$textMuted") return curTheme.colors.textSecondary;
    if (s == "$focusGlow") return curTheme.colors.focusGlow;
    if (s == "$selectionFill") return curTheme.colors.selectionFill;
    if (s == "$white") return Color::White();
    if (s == "$black") return Color::Black();
    if (s == "$transparent") return Color::Transparent();

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
    if (s.size() > 2) {
        std::string suffix2 = toLower(s.substr(s.size() - 2));
        if (suffix2 == "vw") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 2), 0.0f);
            return DimensionConstraint::Vw(v);
        }
        if (suffix2 == "vh") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 2), 0.0f);
            return DimensionConstraint::Vh(v);
        }
        if (suffix2 == "px") {
            float px = parseNumberOrToken(s.substr(0, s.size() - 2), 0.0f);
            return DimensionConstraint::Fixed(px);
        }
    }
    if (s.size() > 4) {
        std::string suffix4 = toLower(s.substr(s.size() - 4));
        if (suffix4 == "vmin") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 4), 0.0f);
            return DimensionConstraint::Vmin(v);
        }
        if (suffix4 == "vmax") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 4), 0.0f);
            return DimensionConstraint::Vmax(v);
        }
    }
    float px = parseNumberOrToken(s, 0.0f);
    return DimensionConstraint::Fixed(px);
}

Length parseLength(std::string_view val, Length defaultLen) {
    std::string s = trim(val);
    if (s.empty()) return defaultLen;
    if (s.back() == '%') {
        float pct = parseNumberOrToken(s.substr(0, s.size() - 1), 0.0f);
        return Length::Percent(pct);
    }
    if (s.size() > 2) {
        std::string suffix2 = toLower(s.substr(s.size() - 2));
        if (suffix2 == "vw") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 2), 0.0f);
            return Length::Vw(v);
        }
        if (suffix2 == "vh") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 2), 0.0f);
            return Length::Vh(v);
        }
        if (suffix2 == "px") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 2), 0.0f);
            return Length::Px(v);
        }
    }
    if (s.size() > 4) {
        std::string suffix4 = toLower(s.substr(s.size() - 4));
        if (suffix4 == "vmin") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 4), 0.0f);
            return Length::Vmin(v);
        }
        if (suffix4 == "vmax") {
            float v = parseNumberOrToken(s.substr(0, s.size() - 4), 0.0f);
            return Length::Vmax(v);
        }
    }
    float v = parseNumberOrToken(s, 0.0f);
    return Length::Px(v);
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

std::string resolveLocalizedText(std::string_view val) {
    return LocalizationService::instance().expandText(val);
}

bool parseTransformOffset(std::string_view val, Point& pxOffset, Point& pctOffset, bool& hasPct) {
    std::string s = trim(val);
    if (s.empty()) return false;
    std::replace(s.begin(), s.end(), ',', ' ');
    std::stringstream ss(s);
    std::vector<std::string> tokens;
    std::string t;
    while (ss >> t) tokens.push_back(t);
    if (tokens.empty()) return false;

    pxOffset = { 0.0f, 0.0f };
    pctOffset = { 0.0f, 0.0f };
    hasPct = false;

    auto parseOne = [](std::string tok, float& px, float& pct, bool& isPct) {
        if (tok.empty()) return;
        if (tok.back() == '%') {
            pct = parseNumberOrToken(tok.substr(0, tok.size() - 1), 0.0f);
            isPct = true;
        } else {
            if (tok.size() >= 2 && tok.substr(tok.size() - 2) == "px") {
                tok = tok.substr(0, tok.size() - 2);
            }
            px = parseNumberOrToken(tok, 0.0f);
        }
    };

    bool isPctX = false;
    parseOne(tokens[0], pxOffset.x, pctOffset.x, isPctX);
    if (isPctX) hasPct = true;

    if (tokens.size() >= 2) {
        bool isPctY = false;
        parseOne(tokens[1], pxOffset.y, pctOffset.y, isPctY);
        if (isPctY) hasPct = true;
    }

    return true;
}

bool parseTransformScale(std::string_view val, Point& scale) {
    std::string s = trim(val);
    if (s.empty()) return false;
    std::replace(s.begin(), s.end(), ',', ' ');
    std::stringstream ss(s);
    std::vector<std::string> tokens;
    std::string t;
    while (ss >> t) tokens.push_back(t);
    if (tokens.empty()) return false;

    auto parseOne = [](std::string tok) -> float {
        if (tok.empty()) return 1.0f;
        if (tok.back() == '%') {
            return parseNumberOrToken(tok.substr(0, tok.size() - 1), 100.0f) / 100.0f;
        }
        return parseNumberOrToken(tok, 1.0f);
    };

    float sx = parseOne(tokens[0]);
    float sy = (tokens.size() >= 2) ? parseOne(tokens[1]) : sx;
    scale = Point{ sx, sy };
    return true;
}

bool parseTransformPivot(std::string_view val, Point& pivot) {
    std::string s = trim(val);
    if (s.empty()) return false;
    std::replace(s.begin(), s.end(), ',', ' ');
    std::stringstream ss(s);
    std::vector<std::string> tokens;
    std::string t;
    while (ss >> t) tokens.push_back(t);
    if (tokens.empty()) return false;

    auto parseOne = [](std::string tok) -> float {
        std::string lower = toLower(tok);
        if (lower == "center") return 0.5f;
        if (lower == "left" || lower == "top") return 0.0f;
        if (lower == "right" || lower == "bottom") return 1.0f;
        if (tok.back() == '%') {
            return parseNumberOrToken(tok.substr(0, tok.size() - 1), 50.0f) / 100.0f;
        }
        return parseNumberOrToken(tok, 0.5f);
    };

    float px = parseOne(tokens[0]);
    float py = (tokens.size() >= 2) ? parseOne(tokens[1]) : px;
    pivot = Point{ px, py };
    return true;
}
bool isCommonLayoutAttribute(const std::string& key) {
    static const std::unordered_map<std::string, bool> s_keys = {
        { "name", true },
        { "id", true },
        { "opacity", true },
        { "transform-offset", true },
        { "transformOffset", true },
        { "transform-scale", true },
        { "transformScale", true },
        { "transform-pivot", true },
        { "transformPivot", true },
        { "direction", true },
        { "flex-direction", true },
        { "align", true },
        { "align-items", true },
        { "justify", true },
        { "justify-content", true },
        { "width", true },
        { "height", true },
        { "flex", true },
        { "padding", true },
        { "margin", true },
        { "margin-top", true },
        { "margin-bottom", true },
        { "margin-left", true },
        { "margin-right", true },
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
        { "blend-mode", true },
        { "blendMode", true },
        { "bind", true },
        { "position", true },
        { "left", true }, { "right", true }, { "top", true }, { "bottom", true }, { "center-x", true }
    };
    return s_keys.find(key) != s_keys.end();
}

bool applyCommonLayoutAttribute(UIElement* element, const std::string& key, const std::string& val) {
    if (!element) return false;
    auto& props = element->layout();

    if (key == "id") {
        element->name(val);
        return true;
    }
    if (key == "name") {
        if (element->name().empty()) {
            element->name(val);
        }
        return true;
    }

    if (key == "position") {
        std::string lower = toLower(trim(val));
        if (lower == "absolute") element->setAbsolute(true);
        else if (lower == "relative" || lower == "static") element->setAbsolute(false);
        return true;
    }
    if (key == "left") { props.left(parseLength(val)); return true; }
    if (key == "right") { props.right(parseLength(val)); return true; }
    if (key == "top") { props.top(parseLength(val)); return true; }
    if (key == "bottom") { props.bottom(parseLength(val)); return true; }
    if (key == "center-x") { props.centerX(parseBool(val, true)); return true; }
    if (key == "direction" || key == "flex-direction") {
        std::string lower = toLower(trim(val));
        if (lower == "row" || lower == "horizontal") props.direction(LayoutDirection::Horizontal);
        else if (lower == "column" || lower == "vertical") props.direction(LayoutDirection::Vertical);
        return true;
    }
    if (key == "align" || key == "align-items") {
        std::string lower = toLower(trim(val));
        if (lower == "start") props.alignment(Alignment::Start);
        else if (lower == "center") props.alignment(Alignment::Center);
        else if (lower == "end") props.alignment(Alignment::End);
        else if (lower == "stretch") props.alignment(Alignment::Stretch);
        return true;
    }
    if (key == "justify" || key == "justify-content") {
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
    if (key == "margin-top") {
        auto m = props.margin();
        m.top = parseNumberOrToken(val);
        props.margin(m.left, m.top, m.right, m.bottom);
        return true;
    }
    if (key == "margin-bottom") {
        auto m = props.margin();
        m.bottom = parseNumberOrToken(val);
        props.margin(m.left, m.top, m.right, m.bottom);
        return true;
    }
    if (key == "margin-left") {
        auto m = props.margin();
        m.left = parseNumberOrToken(val);
        props.margin(m.left, m.top, m.right, m.bottom);
        return true;
    }
    if (key == "margin-right") {
        auto m = props.margin();
        m.right = parseNumberOrToken(val);
        props.margin(m.left, m.top, m.right, m.bottom);
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
        element->tooltip(resolveLocalizedText(val));
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
    if (key == "blend-mode" || key == "blendMode") {
        if (val == "additive" || val == "add") element->blendMode(BlendMode::Additive);
        else if (val == "multiply" || val == "mult") element->blendMode(BlendMode::Multiply);
        else if (val == "screen") element->blendMode(BlendMode::Screen);
        else element->blendMode(BlendMode::Normal);
        return true;
    }
    if (key == "opacity") {
        element->setOpacity(parseNumberOrToken(val, 1.0f));
        return true;
    }
    if (key == "transform-offset" || key == "transformOffset") {
        Point px{ 0.0f, 0.0f };
        Point pct{ 0.0f, 0.0f };
        bool hasPct = false;
        if (parseTransformOffset(val, px, pct, hasPct)) {
            element->setRenderOffset(px, pct);
        }
        return true;
    }
    if (key == "transform-scale" || key == "transformScale") {
        Point scale{ 1.0f, 1.0f };
        if (parseTransformScale(val, scale)) {
            element->setRenderScale(scale);
        }
        return true;
    }
    if (key == "transform-pivot" || key == "transformPivot") {
        Point piv{ 0.5f, 0.5f };
        if (parseTransformPivot(val, piv)) {
            element->setPivot(piv);
        }
        return true;
    }
    if (key == "id") {
        element->name(val);
        return true;
    }
    if (key == "name" || key == "bind") {
        return true;
    }

    return false;
}

} // namespace PerfUI::MarkupInternal
