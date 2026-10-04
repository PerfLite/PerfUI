#include "PerfUI/Theme.h"

namespace PerfUI {

Theme& Theme::MutableCurrent() {
    static Theme s_current;
    return s_current;
}

const Theme& Theme::Current() {
    return MutableCurrent();
}

void Theme::ApplyPreset(const std::string& presetName) {
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

} // namespace PerfUI
