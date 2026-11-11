#include "PerfUI/Theme.h"
#include <pugixml.hpp>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

namespace PerfUI {

static Theme CreateNordicFantasyTheme() {
    Theme t;
    t.colors.surfaceBase = Color(13, 17, 23, 235);
    t.colors.surfaceElevated = Color(22, 27, 34, 220);
    t.colors.surfaceActive = Color(33, 38, 45, 245);
    t.colors.borderSubtle = Color(48, 54, 61, 155);
    t.colors.borderStrong = Color(72, 79, 88, 205);
    t.colors.borderFocus = Color(212, 175, 55, 255);
    t.colors.textPrimary = Color(240, 246, 252, 255);
    t.colors.textSecondary = Color(139, 148, 158, 255);
    t.colors.textAccent = Color(229, 192, 123, 255);
    t.colors.textSuccess = Color(126, 231, 135, 255);
    t.colors.textDisabled = Color(72, 79, 88, 255);
    t.colors.focusGlow = Color(212, 175, 55, 90);
    t.colors.selectionFill = Color(212, 175, 55, 40);

    t.metrics.radiusSmall = 4.0f;
    t.metrics.radiusMedium = 8.0f;
    t.metrics.radiusLarge = 12.0f;

    // Component Styles
    t.components.button.box.normal = BoxStyle{}.bg(Color(26, 33, 44, 230)).border(t.colors.borderSubtle, 1.0f).radius(6.0f);
    t.components.button.box.hover = BoxStyle{}.bg(Color(35, 44, 58, 245)).border(t.colors.borderStrong, 1.0f).radius(6.0f);
    t.components.button.box.pressed = BoxStyle{}.bg(Color(20, 26, 35, 255)).border(t.colors.borderFocus, 1.0f).radius(6.0f);
    t.components.button.box.focused = BoxStyle{}.border(t.colors.borderFocus, 1.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(14.0f);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(14.0f);
    t.components.button.text.pressed = TextStyleOverride{}.col(Color::White()).size(14.0f);

    t.components.panel.box.normal = BoxStyle{};

    t.components.switchToggle.box.normal = BoxStyle{}.bg(Color(48, 54, 61, 200)).radius(12.0f);
    t.components.switchToggle.box.selected = BoxStyle{}.bg(t.colors.borderFocus).radius(12.0f);

    t.components.divider.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).border(t.colors.borderSubtle, 1.0f);

    return t;
}

Theme& Theme::MutableCurrent() {
    static Theme s_current = CreateNordicFantasyTheme();
    return s_current;
}

const Theme& Theme::Current() {
    return MutableCurrent();
}

void Theme::SetCurrent(const Theme& theme) {
    MutableCurrent() = theme;
}

Theme Theme::NordicFantasy() {
    return CreateNordicFantasyTheme();
}

Theme Theme::DarkFantasy() {
    Theme t;
    t.colors.surfaceBase = Color(10, 8, 12, 245);
    t.colors.surfaceElevated = Color(18, 14, 20, 235);
    t.colors.surfaceActive = Color(28, 20, 30, 250);
    t.colors.borderSubtle = Color(45, 30, 40, 180);
    t.colors.borderStrong = Color(75, 40, 60, 220);
    t.colors.borderFocus = Color(180, 30, 40, 255); // Crimson Blood
    t.colors.textPrimary = Color(225, 220, 230, 255);
    t.colors.textSecondary = Color(140, 130, 145, 255);
    t.colors.textAccent = Color(200, 50, 60, 255);
    t.colors.textSuccess = Color(100, 200, 120, 255);
    t.colors.textDisabled = Color(65, 50, 60, 255);
    t.colors.focusGlow = Color(200, 40, 50, 120);
    t.colors.selectionFill = Color(180, 30, 40, 50);

    t.metrics.radiusSmall = 3.0f;
    t.metrics.radiusMedium = 6.0f;
    t.metrics.radiusLarge = 10.0f;

    t.components.button.box.normal = BoxStyle{}.bg(Color(25, 18, 28, 235)).border(t.colors.borderSubtle, 1.0f).radius(4.0f);
    t.components.button.box.hover = BoxStyle{}.bg(Color(40, 24, 38, 250)).border(t.colors.borderFocus, 1.0f).radius(4.0f);
    t.components.button.box.pressed = BoxStyle{}.bg(Color(15, 10, 18, 255)).border(Color(220, 40, 50, 255), 1.0f).radius(4.0f);
    t.components.button.box.focused = BoxStyle{}.border(t.colors.borderFocus, 1.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(14.0f);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(14.0f);
    t.components.button.text.pressed = TextStyleOverride{}.col(Color::White()).size(14.0f);

    t.components.panel.box.normal = BoxStyle{};

    t.components.switchToggle.box.normal = BoxStyle{}.bg(Color(45, 30, 40, 200)).radius(12.0f);
    t.components.switchToggle.box.selected = BoxStyle{}.bg(t.colors.borderFocus).radius(12.0f);

    t.components.divider.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).border(t.colors.borderSubtle, 1.0f);

    return t;
}

Theme Theme::Minimalist() {
    Theme t;
    t.colors.surfaceBase = Color(18, 18, 20, 250);
    t.colors.surfaceElevated = Color(26, 26, 30, 250);
    t.colors.surfaceActive = Color(34, 34, 40, 250);
    t.colors.borderSubtle = Color(45, 45, 50, 200);
    t.colors.borderStrong = Color(70, 70, 78, 220);
    t.colors.borderFocus = Color(90, 120, 220, 255); // Indigo
    t.colors.textPrimary = Color(245, 245, 248, 255);
    t.colors.textSecondary = Color(150, 150, 160, 255);
    t.colors.textAccent = Color(110, 140, 240, 255);
    t.colors.textSuccess = Color(80, 200, 130, 255);
    t.colors.textDisabled = Color(65, 65, 70, 255);
    t.colors.focusGlow = Color(90, 120, 220, 80);
    t.colors.selectionFill = Color(90, 120, 220, 40);

    t.metrics.radiusSmall = 2.0f;
    t.metrics.radiusMedium = 4.0f;
    t.metrics.radiusLarge = 6.0f;

    t.components.button.box.normal = BoxStyle{}.bg(Color(30, 30, 35, 240)).border(t.colors.borderSubtle, 1.0f).radius(4.0f);
    t.components.button.box.hover = BoxStyle{}.bg(Color(42, 42, 50, 250)).border(t.colors.borderFocus, 1.0f).radius(4.0f);
    t.components.button.box.pressed = BoxStyle{}.bg(Color(24, 24, 28, 255)).border(t.colors.borderFocus, 1.0f).radius(4.0f);
    t.components.button.box.focused = BoxStyle{}.border(t.colors.borderFocus, 1.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(14.0f);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(14.0f);

    t.components.panel.box.normal = BoxStyle{};

    t.components.switchToggle.box.normal = BoxStyle{}.bg(Color(45, 45, 50, 220)).radius(12.0f);
    t.components.switchToggle.box.selected = BoxStyle{}.bg(t.colors.borderFocus).radius(12.0f);

    t.components.divider.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).border(t.colors.borderSubtle, 1.0f);

    return t;
}

Theme Theme::VanillaSkyrim() {
    Theme t;
    t.colors.surfaceBase = Color(18, 17, 16, 245);
    t.colors.surfaceElevated = Color(28, 26, 24, 235);
    t.colors.surfaceActive = Color(40, 36, 32, 250);
    t.colors.borderSubtle = Color(65, 58, 50, 200);
    t.colors.borderStrong = Color(95, 85, 72, 220);
    t.colors.borderFocus = Color(216, 185, 100, 255); // Skyrim Gold
    t.colors.textPrimary = Color(238, 230, 210, 255);
    t.colors.textSecondary = Color(168, 158, 140, 255);
    t.colors.textAccent = Color(230, 204, 128, 255);
    t.colors.textSuccess = Color(140, 190, 120, 255);
    t.colors.textDisabled = Color(75, 70, 62, 255);
    t.colors.focusGlow = Color(216, 185, 100, 90);
    t.colors.selectionFill = Color(216, 185, 100, 45);

    t.metrics.radiusSmall = 0.0f;
    t.metrics.radiusMedium = 2.0f;
    t.metrics.radiusLarge = 4.0f;

    t.components.button.box.normal = BoxStyle{}.bg(Color(30, 28, 25, 235)).border(t.colors.borderSubtle, 1.0f).radius(2.0f);
    t.components.button.box.hover = BoxStyle{}.bg(Color(45, 40, 35, 250)).border(t.colors.borderFocus, 1.0f).radius(2.0f);
    t.components.button.box.pressed = BoxStyle{}.bg(Color(22, 20, 18, 255)).border(t.colors.borderFocus, 1.0f).radius(2.0f);
    t.components.button.box.focused = BoxStyle{}.border(t.colors.borderFocus, 1.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(14.0f);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(14.0f);

    t.components.panel.box.normal = BoxStyle{};

    t.components.switchToggle.box.normal = BoxStyle{}.bg(Color(65, 58, 50, 200)).radius(12.0f);
    t.components.switchToggle.box.selected = BoxStyle{}.bg(t.colors.borderFocus).radius(12.0f);

    t.components.divider.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).border(t.colors.borderSubtle, 1.0f);

    return t;
}

Theme Theme::Futuristic() {
    Theme t;
    t.colors.surfaceBase = Color(6, 10, 18, 245);
    t.colors.surfaceElevated = Color(12, 18, 30, 240);
    t.colors.surfaceActive = Color(20, 30, 48, 250);
    t.colors.borderSubtle = Color(25, 45, 70, 180);
    t.colors.borderStrong = Color(40, 75, 120, 220);
    t.colors.borderFocus = Color(0, 229, 255, 255); // Neon Cyan
    t.colors.textPrimary = Color(230, 245, 255, 255);
    t.colors.textSecondary = Color(120, 160, 190, 255);
    t.colors.textAccent = Color(0, 229, 255, 255);
    t.colors.textSuccess = Color(0, 255, 160, 255);
    t.colors.textDisabled = Color(40, 60, 80, 255);
    t.colors.focusGlow = Color(0, 229, 255, 120);
    t.colors.selectionFill = Color(0, 229, 255, 50);

    t.metrics.radiusSmall = 0.0f;
    t.metrics.radiusMedium = 3.0f;
    t.metrics.radiusLarge = 6.0f;

    t.components.button.box.normal = BoxStyle{}.bg(Color(15, 24, 38, 240)).border(t.colors.borderSubtle, 1.0f).radius(2.0f);
    t.components.button.box.hover = BoxStyle{}.bg(Color(24, 40, 65, 250)).border(t.colors.borderFocus, 1.0f).radius(2.0f);
    t.components.button.box.pressed = BoxStyle{}.bg(Color(10, 16, 28, 255)).border(t.colors.borderFocus, 1.0f).radius(2.0f);
    t.components.button.box.focused = BoxStyle{}.border(t.colors.borderFocus, 1.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(14.0f);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(14.0f);

    t.components.panel.box.normal = BoxStyle{};

    t.components.switchToggle.box.normal = BoxStyle{}.bg(Color(25, 45, 70, 200)).radius(12.0f);
    t.components.switchToggle.box.selected = BoxStyle{}.bg(t.colors.borderFocus).radius(12.0f);

    t.components.divider.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).border(t.colors.borderSubtle, 1.0f);

    return t;
}

Theme Theme::LiquidGlass() {
    Theme t;
    // Deep obsidian icy glass with hardware Kawase backdrop blur and multi-layer liquid specular gloss
    t.colors.surfaceBase = Color(10, 14, 22, 160);
    t.colors.surfaceElevated = Color(16, 22, 34, 180);
    t.colors.surfaceActive = Color(24, 34, 52, 210);
    t.colors.borderSubtle = Color(130, 175, 220, 90);
    t.colors.borderStrong = Color(160, 205, 245, 160);
    t.colors.borderFocus = Color(180, 225, 255, 240); // Cold Silver Cyan
    t.colors.textPrimary = Color(240, 246, 255, 255);
    t.colors.textSecondary = Color(165, 190, 215, 240);
    t.colors.textAccent = Color(180, 225, 255, 255);
    t.colors.textSuccess = Color(120, 220, 150, 255);
    t.colors.textDisabled = Color(60, 80, 105, 255);
    t.colors.focusGlow = Color(180, 225, 255, 110);
    t.colors.selectionFill = Color(180, 225, 255, 50);

    t.metrics.radiusSmall = 4.0f;
    t.metrics.radiusMedium = 8.0f;
    t.metrics.radiusLarge = 12.0f;

    // Component styles
    t.components.panel.box.normal = BoxStyle{}.glass(true);
    t.components.button.box.normal = BoxStyle{}.bg(Color(20, 28, 42, 180)).border(t.colors.borderSubtle, 1.0f).radius(4.0f);
    t.components.button.box.hover = BoxStyle{}.bg(Color(32, 46, 68, 220)).border(t.colors.borderFocus, 1.5f).radius(4.0f);
    t.components.button.box.pressed = BoxStyle{}.bg(Color(14, 20, 30, 240)).border(t.colors.borderFocus, 1.5f).radius(4.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(13.0f);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(13.0f);

    return t;
}

Theme Theme::NordicSlate() {
    Theme t;
    // Dark Nordic slate & wrought iron (fits Vanilla Skyrim / Dear Diary / Nordic UI)
    t.colors.surfaceBase = Color(14, 16, 18, 245);
    t.colors.surfaceElevated = Color(22, 25, 28, 240);
    t.colors.surfaceActive = Color(32, 36, 42, 245);
    t.colors.borderSubtle = Color(65, 72, 80, 180);
    t.colors.borderStrong = Color(110, 120, 135, 220);
    t.colors.borderFocus = Color(190, 205, 220, 255); // Cold forged steel
    t.colors.textPrimary = Color(230, 235, 242, 255);
    t.colors.textSecondary = Color(140, 150, 165, 255);
    t.colors.textAccent = Color(210, 225, 240, 255);
    t.colors.textSuccess = Color(120, 190, 130, 255);
    t.colors.textDisabled = Color(55, 62, 70, 255);
    t.colors.focusGlow = Color(190, 205, 220, 80);
    t.colors.selectionFill = Color(190, 205, 220, 40);

    t.metrics.radiusSmall = 2.0f;
    t.metrics.radiusMedium = 4.0f;
    t.metrics.radiusLarge = 6.0f;

    t.components.button.box.normal = BoxStyle{}.bg(Color(25, 28, 32, 235)).border(t.colors.borderSubtle, 1.0f).radius(2.0f);
    t.components.button.box.hover = BoxStyle{}.bg(Color(38, 42, 48, 250)).border(t.colors.borderFocus, 1.0f).radius(2.0f);
    t.components.button.box.pressed = BoxStyle{}.bg(Color(18, 20, 22, 255)).border(t.colors.borderFocus, 1.0f).radius(2.0f);
    t.components.button.box.focused = BoxStyle{}.border(t.colors.borderFocus, 1.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(13.0f);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(13.0f);

    t.components.panel.box.normal = BoxStyle{};

    return t;
}

Theme Theme::GlassFrost() {
    return LiquidGlass();
}

static std::unordered_map<std::string, Theme>& GetCustomThemes() {
    static std::unordered_map<std::string, Theme> s_customThemes;
    return s_customThemes;
}

void Theme::RegisterTheme(std::string name, const Theme& theme) {
    GetCustomThemes()[std::move(name)] = theme;
}

bool Theme::HasTheme(const std::string& name) {
    if (name == "Liquid Glass" || name == "Liquid Obsidian" || name == "Liquid" ||
        name == "Nordic Slate" || name == "Nordic Gold" || name == "Nordic Fantasy" ||
        name == "Dark Fantasy" || name == "Vanilla Skyrim" ||
        name == "Minimalist" || name == "Futuristic" || name == "SciFi" ||
        name == "Imperial Ruby" || name == "Dawnguard Amber" || name == "Winterhold Frost") {
        return true;
    }
    const auto& custom = GetCustomThemes();
    return custom.find(name) != custom.end();
}

Theme Theme::GetTheme(const std::string& name) {
    if (name == "Liquid Glass" || name == "Liquid Obsidian" || name == "Liquid" ||
        name == "Glass Frost" || name == "Frosted Glass" || name == "Glass") {
        return LiquidGlass();
    }
    if (name == "Nordic Gold" || name == "Nordic Fantasy") return NordicFantasy();
    if (name == "Nordic Slate") return NordicSlate();
    if (name == "Dark Fantasy") return DarkFantasy();
    if (name == "Vanilla Skyrim") return VanillaSkyrim();
    if (name == "Minimalist") return Minimalist();
    if (name == "Futuristic" || name == "SciFi") return Futuristic();
    if (name == "Imperial Ruby") {
        auto t = NordicFantasy();
        t.colors.borderFocus = Color(205, 55, 60, 255);
        t.colors.textAccent = Color(235, 95, 100, 255);
        t.colors.focusGlow = Color(205, 55, 60, 90);
        t.colors.selectionFill = Color(205, 55, 60, 40);
        return t;
    }
    if (name == "Dawnguard Amber") {
        auto t = NordicFantasy();
        t.colors.borderFocus = Color(225, 145, 45, 255);
        t.colors.textAccent = Color(245, 170, 70, 255);
        t.colors.focusGlow = Color(225, 145, 45, 90);
        t.colors.selectionFill = Color(225, 145, 45, 40);
        return t;
    }
    if (name == "Winterhold Frost") {
        auto t = NordicFantasy();
        t.colors.borderFocus = Color(85, 175, 235, 255);
        t.colors.textAccent = Color(130, 210, 255, 255);
        t.colors.focusGlow = Color(85, 175, 235, 90);
        t.colors.selectionFill = Color(85, 175, 235, 40);
        return t;
    }
    const auto& custom = GetCustomThemes();
    auto it = custom.find(name);
    if (it != custom.end()) {
        return it->second;
    }
    return LiquidGlass();
}

std::vector<std::string> Theme::GetAvailableThemes() {
    std::vector<std::string> themes = {
        "Liquid Glass",
        "Nordic Slate",
        "Nordic Gold",
        "Dark Fantasy",
        "Vanilla Skyrim",
        "Imperial Ruby",
        "Dawnguard Amber",
        "Winterhold Frost"
    };
    for (const auto& [name, _] : GetCustomThemes()) {
        if (std::find(themes.begin(), themes.end(), name) == themes.end()) {
            themes.push_back(name);
        }
    }
    return themes;
}

void Theme::ApplyPreset(const std::string& presetName) {
    if (HasTheme(presetName)) {
        SetCurrent(GetTheme(presetName));
        return;
    }
    SetCurrent(NordicFantasy());
}

static Color ParseColorAttributeOrNode(pugi::xml_node parent, const char* name, Color defaultColor) {
    if (!parent) return defaultColor;

    if (auto attr = parent.attribute(name)) {
        std::string_view val = attr.as_string();
        if (!val.empty()) {
            return Color::FromHex(val, defaultColor);
        }
    }

    pugi::xml_node child = parent.child(name);
    if (child) {
        if (auto hexAttr = child.attribute("hex")) {
            return Color::FromHex(hexAttr.as_string(), defaultColor);
        }
        if (auto colorAttr = child.attribute("color")) {
            return Color::FromHex(colorAttr.as_string(), defaultColor);
        }
        if (auto valAttr = child.attribute("value")) {
            return Color::FromHex(valAttr.as_string(), defaultColor);
        }
        if (child.attribute("r") || child.attribute("g") || child.attribute("b")) {
            uint8_t r = static_cast<uint8_t>(child.attribute("r").as_uint(defaultColor.r));
            uint8_t g = static_cast<uint8_t>(child.attribute("g").as_uint(defaultColor.g));
            uint8_t b = static_cast<uint8_t>(child.attribute("b").as_uint(defaultColor.b));
            uint8_t a = static_cast<uint8_t>(child.attribute("a").as_uint(defaultColor.a));
            return Color(r, g, b, a);
        }
        std::string_view text = child.text().as_string();
        if (!text.empty()) {
            return Color::FromHex(text, defaultColor);
        }
    }

    return defaultColor;
}

static float ParseFloatAttributeOrNode(pugi::xml_node parent, const char* name, float defaultVal) {
    if (!parent) return defaultVal;

    if (auto attr = parent.attribute(name)) {
        return attr.as_float(defaultVal);
    }
    pugi::xml_node child = parent.child(name);
    if (child) {
        if (auto val = child.attribute("value")) return val.as_float(defaultVal);
        return child.text().as_float(defaultVal);
    }
    return defaultVal;
}

static void RefreshDefaultComponentStyles(Theme& t) {
    t.components.button.box.normal = BoxStyle{}.bg(t.colors.surfaceElevated).border(t.colors.borderSubtle, 1.0f).radius(t.metrics.radiusSmall);
    t.components.button.box.hover = BoxStyle{}.bg(t.colors.surfaceActive).border(t.colors.borderFocus, 1.5f).radius(t.metrics.radiusSmall);
    t.components.button.box.pressed = BoxStyle{}.bg(t.colors.surfaceBase).border(t.colors.borderFocus, 1.0f).radius(t.metrics.radiusSmall);
    t.components.button.box.focused = BoxStyle{}.border(t.colors.borderFocus, 1.0f);
    t.components.button.text.normal = TextStyleOverride{}.col(t.colors.textPrimary).size(t.typography.fontBody);
    t.components.button.text.hover = TextStyleOverride{}.col(Color::White()).size(t.typography.fontBody);

    t.components.panel.box.normal = BoxStyle{};

    t.components.switchToggle.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).radius(12.0f);
    t.components.switchToggle.box.selected = BoxStyle{}.bg(t.colors.borderFocus).radius(12.0f);

    t.components.divider.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).border(t.colors.borderSubtle, 1.0f);
}

bool Theme::LoadFromXml(std::string_view xml, std::string* outName, bool applyIfRequested, Theme* outTheme) {
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_buffer(xml.data(), xml.size());
    if (!res) {
        return false;
    }

    pugi::xml_node themeNode = doc.child("Theme");
    if (!themeNode) {
        pugi::xml_node root = doc.first_child();
        if (root && std::string_view(root.name()) == "Theme") {
            themeNode = root;
        } else if (root && std::string_view(root.name()) == "UI") {
            themeNode = root.child("Theme");
        }
    }

    if (!themeNode) {
        return false;
    }

    std::string name = themeNode.attribute("name").as_string("Custom Theme");
    std::string extendsBase = themeNode.attribute("extends").as_string("Nordic Gold");
    bool apply = themeNode.attribute("apply").as_bool(false);

    Theme t = GetTheme(extendsBase);

    // Parse <Colors>
    pugi::xml_node colorsNode = themeNode.child("Colors");
    if (colorsNode) {
        t.colors.surfaceBase = ParseColorAttributeOrNode(colorsNode, "surfaceBase", t.colors.surfaceBase);
        t.colors.surfaceElevated = ParseColorAttributeOrNode(colorsNode, "surfaceElevated", t.colors.surfaceElevated);
        t.colors.surfaceActive = ParseColorAttributeOrNode(colorsNode, "surfaceActive", t.colors.surfaceActive);
        t.colors.borderSubtle = ParseColorAttributeOrNode(colorsNode, "borderSubtle", t.colors.borderSubtle);
        t.colors.borderStrong = ParseColorAttributeOrNode(colorsNode, "borderStrong", t.colors.borderStrong);
        t.colors.borderFocus = ParseColorAttributeOrNode(colorsNode, "borderFocus", t.colors.borderFocus);
        t.colors.textPrimary = ParseColorAttributeOrNode(colorsNode, "textPrimary", t.colors.textPrimary);
        t.colors.textSecondary = ParseColorAttributeOrNode(colorsNode, "textSecondary", t.colors.textSecondary);
        t.colors.textAccent = ParseColorAttributeOrNode(colorsNode, "textAccent", t.colors.textAccent);
        t.colors.textSuccess = ParseColorAttributeOrNode(colorsNode, "textSuccess", t.colors.textSuccess);
        t.colors.textDisabled = ParseColorAttributeOrNode(colorsNode, "textDisabled", t.colors.textDisabled);
        t.colors.focusGlow = ParseColorAttributeOrNode(colorsNode, "focusGlow", t.colors.focusGlow);
        t.colors.selectionFill = ParseColorAttributeOrNode(colorsNode, "selectionFill", t.colors.selectionFill);
    }

    // Parse <Metrics>
    pugi::xml_node metricsNode = themeNode.child("Metrics");
    if (metricsNode) {
        if (auto radAttr = metricsNode.attribute("radius")) {
            float r = radAttr.as_float();
            t.metrics.radiusSmall = (std::max)(0.0f, r * 0.5f);
            t.metrics.radiusMedium = r;
            t.metrics.radiusLarge = r * 1.5f;
        }
        if (auto spaceAttr = metricsNode.attribute("spacing")) {
            float s = spaceAttr.as_float();
            t.metrics.spacingXS = (std::max)(0.0f, s * 0.5f);
            t.metrics.spacingSM = s;
            t.metrics.spacingMD = s * 2.0f;
            t.metrics.spacingLG = s * 3.0f;
            t.metrics.spacingXL = s * 4.0f;
        }
        t.metrics.radiusSmall = ParseFloatAttributeOrNode(metricsNode, "radiusSmall", t.metrics.radiusSmall);
        t.metrics.radiusMedium = ParseFloatAttributeOrNode(metricsNode, "radiusMedium", t.metrics.radiusMedium);
        t.metrics.radiusLarge = ParseFloatAttributeOrNode(metricsNode, "radiusLarge", t.metrics.radiusLarge);

        t.metrics.spacingXS = ParseFloatAttributeOrNode(metricsNode, "spacingXS", t.metrics.spacingXS);
        t.metrics.spacingSM = ParseFloatAttributeOrNode(metricsNode, "spacingSM", t.metrics.spacingSM);
        t.metrics.spacingMD = ParseFloatAttributeOrNode(metricsNode, "spacingMD", t.metrics.spacingMD);
        t.metrics.spacingLG = ParseFloatAttributeOrNode(metricsNode, "spacingLG", t.metrics.spacingLG);
        t.metrics.spacingXL = ParseFloatAttributeOrNode(metricsNode, "spacingXL", t.metrics.spacingXL);
    }

    // Parse <Typography>
    pugi::xml_node typoNode = themeNode.child("Typography");
    if (typoNode) {
        t.typography.fontSmall = ParseFloatAttributeOrNode(typoNode, "fontSmall", t.typography.fontSmall);
        t.typography.fontBody = ParseFloatAttributeOrNode(typoNode, "fontBody", t.typography.fontBody);
        t.typography.fontMedium = ParseFloatAttributeOrNode(typoNode, "fontMedium", t.typography.fontMedium);
        t.typography.fontTitle = ParseFloatAttributeOrNode(typoNode, "fontTitle", t.typography.fontTitle);
        t.typography.fontHeader = ParseFloatAttributeOrNode(typoNode, "fontHeader", t.typography.fontHeader);
    }

    // Recompute component defaults from new colors & metrics
    RefreshDefaultComponentStyles(t);

    // Parse <Components> overrides if any
    pugi::xml_node compNode = themeNode.child("Components");
    if (compNode) {
        pugi::xml_node btnNode = compNode.child("Button");
        if (btnNode) {
            Color normalBg = ParseColorAttributeOrNode(btnNode, "normalBg", t.colors.surfaceElevated);
            Color hoverBg = ParseColorAttributeOrNode(btnNode, "hoverBg", t.colors.surfaceActive);
            Color borderCol = ParseColorAttributeOrNode(btnNode, "border", t.colors.borderSubtle);
            float rad = ParseFloatAttributeOrNode(btnNode, "radius", t.metrics.radiusSmall);
            t.components.button.box.normal = BoxStyle{}.bg(normalBg).border(borderCol, 1.0f).radius(rad);
            t.components.button.box.hover = BoxStyle{}.bg(hoverBg).border(t.colors.borderFocus, 1.5f).radius(rad);
        }
        pugi::xml_node panelNode = compNode.child("Panel");
        if (panelNode) {
            Color bg = ParseColorAttributeOrNode(panelNode, "bg", t.colors.surfaceBase);
            Color borderCol = ParseColorAttributeOrNode(panelNode, "border", t.colors.borderSubtle);
            float rad = ParseFloatAttributeOrNode(panelNode, "radius", t.metrics.radiusMedium);
            t.components.panel.box.normal = BoxStyle{}.bg(bg).border(borderCol, 1.0f).radius(rad);
        }
    }

    RegisterTheme(name, t);

    if (applyIfRequested && apply) {
        SetCurrent(t);
    }

    if (outName) *outName = name;
    if (outTheme) *outTheme = t;

    return true;
}

bool Theme::LoadFromFile(const std::filesystem::path& filepath, std::string* outName, bool applyIfRequested, Theme* outTheme) {
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs) return false;
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return LoadFromXml(content, outName, applyIfRequested, outTheme);
}

void Theme::LoadThemesFromFolder(const std::filesystem::path& folder) {
    std::error_code ec;
    if (!std::filesystem::exists(folder, ec) || !std::filesystem::is_directory(folder, ec)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".xml") {
            LoadFromFile(entry.path());
        }
    }
}

} // namespace PerfUI
