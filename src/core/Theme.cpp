#include "PerfUI/Theme.h"

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

    t.components.panel.box.normal = BoxStyle{}.bg(t.colors.surfaceElevated).border(t.colors.borderSubtle, 1.0f).radius(8.0f)
                                     .dropShadow(Color(0, 0, 0, 160), 16.0f, { 0.0f, 6.0f });

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

    t.components.panel.box.normal = BoxStyle{}.bg(t.colors.surfaceElevated).border(t.colors.borderSubtle, 1.0f).radius(6.0f)
                                     .dropShadow(Color(0, 0, 0, 200), 20.0f, { 0.0f, 6.0f });

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

    t.components.panel.box.normal = BoxStyle{}.bg(t.colors.surfaceElevated).border(t.colors.borderSubtle, 1.0f).radius(4.0f)
                                     .dropShadow(Color(0, 0, 0, 120), 10.0f, { 0.0f, 4.0f });

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

    t.components.panel.box.normal = BoxStyle{}.bg(t.colors.surfaceElevated).border(t.colors.borderSubtle, 1.0f).radius(2.0f)
                                     .dropShadow(Color(0, 0, 0, 180), 14.0f, { 0.0f, 5.0f });

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

    t.components.panel.box.normal = BoxStyle{}.bg(t.colors.surfaceElevated).border(t.colors.borderSubtle, 1.0f).radius(3.0f)
                                     .dropShadow(Color(0, 180, 255, 80), 16.0f, { 0.0f, 4.0f });

    t.components.switchToggle.box.normal = BoxStyle{}.bg(Color(25, 45, 70, 200)).radius(12.0f);
    t.components.switchToggle.box.selected = BoxStyle{}.bg(t.colors.borderFocus).radius(12.0f);

    t.components.divider.box.normal = BoxStyle{}.bg(t.colors.borderSubtle).border(t.colors.borderSubtle, 1.0f);

    return t;
}

void Theme::ApplyPreset(const std::string& presetName) {
    if (presetName == "Dark Fantasy") {
        SetCurrent(DarkFantasy());
    } else if (presetName == "Minimalist") {
        SetCurrent(Minimalist());
    } else if (presetName == "Vanilla Skyrim") {
        SetCurrent(VanillaSkyrim());
    } else if (presetName == "Futuristic" || presetName == "SciFi") {
        SetCurrent(Futuristic());
    } else if (presetName == "Imperial Ruby") {
        auto t = NordicFantasy();
        t.colors.borderFocus = Color(205, 55, 60, 255);
        t.colors.textAccent = Color(235, 95, 100, 255);
        t.colors.focusGlow = Color(205, 55, 60, 90);
        t.colors.selectionFill = Color(205, 55, 60, 40);
        SetCurrent(t);
    } else if (presetName == "Dawnguard Amber") {
        auto t = NordicFantasy();
        t.colors.borderFocus = Color(225, 145, 45, 255);
        t.colors.textAccent = Color(245, 170, 70, 255);
        t.colors.focusGlow = Color(225, 145, 45, 90);
        t.colors.selectionFill = Color(225, 145, 45, 40);
        SetCurrent(t);
    } else if (presetName == "Winterhold Frost") {
        auto t = NordicFantasy();
        t.colors.borderFocus = Color(85, 175, 235, 255);
        t.colors.textAccent = Color(130, 210, 255, 255);
        t.colors.focusGlow = Color(85, 175, 235, 90);
        t.colors.selectionFill = Color(85, 175, 235, 40);
        SetCurrent(t);
    } else { // "Nordic Gold" / "Nordic Fantasy"
        SetCurrent(NordicFantasy());
    }
}

} // namespace PerfUI
