#pragma once

#include "Types.h"
#include "Style.h"
#include <string>

namespace PerfUI {

struct ThemeMetrics {
    float radiusSmall{ 4.0f };
    float radiusMedium{ 8.0f };
    float radiusLarge{ 12.0f };

    float spacingXS{ 4.0f };
    float spacingSM{ 8.0f };
    float spacingMD{ 16.0f };
    float spacingLG{ 24.0f };
    float spacingXL{ 32.0f };
};

struct ThemeTypography {
    float fontSmall{ 12.0f };
    float fontBody{ 14.0f };
    float fontMedium{ 16.0f };
    float fontTitle{ 20.0f };
    float fontHeader{ 26.0f };
};

struct ThemeColors {
    Color surfaceBase{ 13, 17, 23, 235 };
    Color surfaceElevated{ 22, 27, 34, 220 };
    Color surfaceActive{ 33, 38, 45, 245 };

    Color borderSubtle{ 48, 54, 61, 155 };
    Color borderStrong{ 72, 79, 88, 205 };
    Color borderFocus{ 212, 175, 55, 255 }; // Nordic Gold

    Color textPrimary{ 240, 246, 252, 255 };
    Color textSecondary{ 139, 148, 158, 255 };
    Color textAccent{ 229, 192, 123, 255 };
    Color textSuccess{ 126, 231, 135, 255 };
    Color textDisabled{ 72, 79, 88, 255 };

    Color focusGlow{ 212, 175, 55, 90 };
    Color selectionFill{ 212, 175, 55, 40 };
};

struct ComponentThemes {
    WidgetStyle button;
    WidgetStyle panel;
    WidgetStyle checkbox;
    WidgetStyle switchToggle;
    WidgetStyle slider;
    WidgetStyle radio;
    WidgetStyle text;
    WidgetStyle progressBar;
    WidgetStyle divider;
};

class PERFUI_API Theme {
public:
    static Theme& MutableCurrent();
    static const Theme& Current();
    static void SetCurrent(const Theme& theme);
    static void ApplyPreset(const std::string& presetName);

    // Theme Presets
    static Theme NordicFantasy();
    static Theme DarkFantasy();
    static Theme Minimalist();
    static Theme VanillaSkyrim();
    static Theme Futuristic();

    ThemeColors colors;
    ThemeMetrics metrics;
    ThemeTypography typography;
    ComponentThemes components;
};

} // namespace PerfUI
