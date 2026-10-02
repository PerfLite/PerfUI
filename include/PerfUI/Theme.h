#pragma once

#include "Types.h"

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

class Theme {
public:
    static Theme& MutableCurrent() {
        static Theme s_current;
        return s_current;
    }

    static const Theme& Current() {
        return MutableCurrent();
    }

    static void ApplyPreset(const std::string& presetName) {
        auto& t = MutableCurrent();
        if (presetName == "Imperial Ruby") {
            t.colors.borderFocus = Color(205, 55, 60, 255);
            t.colors.textAccent = Color(235, 95, 100, 255);
            t.colors.focusGlow = Color(205, 55, 60, 90);
            t.colors.selectionFill = Color(205, 55, 60, 40);
        } else if (presetName == "Dawnguard Amber") {
            t.colors.borderFocus = Color(225, 145, 45, 255);
            t.colors.textAccent = Color(245, 170, 70, 255);
            t.colors.focusGlow = Color(225, 145, 45, 90);
            t.colors.selectionFill = Color(225, 145, 45, 40);
        } else if (presetName == "Winterhold Frost") {
            t.colors.borderFocus = Color(85, 175, 235, 255);
            t.colors.textAccent = Color(130, 210, 255, 255);
            t.colors.focusGlow = Color(85, 175, 235, 90);
            t.colors.selectionFill = Color(85, 175, 235, 40);
        } else { // "Nordic Gold" (default)
            t.colors.borderFocus = Color(212, 175, 55, 255);
            t.colors.textAccent = Color(229, 192, 123, 255);
            t.colors.focusGlow = Color(212, 175, 55, 90);
            t.colors.selectionFill = Color(212, 175, 55, 40);
        }
    }

    static Theme NordicFantasy() {
        return Theme();
    }

    ThemeColors colors;
    ThemeMetrics metrics;
    ThemeTypography typography;
};

} // namespace PerfUI
