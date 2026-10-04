/**
 * @file main.cpp
 * @brief Example: Declarative XML UI with live hot-reloading in PerfUI.
 */

#include "PerfUI/PerfUI.h"
#include "PerfUI/Markup/MarkupLoader.h"
#include <iostream>

void SetupMarkupSettings(PerfUI::UIContext& context) {
    // 1. Create loader instance bound to UIContext
    static PerfUI::MarkupLoader loader(context);

    // 2. Bind event callbacks by name
    loader.bindCallback("cancelAction", [&context]() {
        std::cout << "[Event] Cancel clicked!\n";
        context.showToast("Settings", "Changes discarded.", PerfUI::ToastType::Info);
    });

    loader.bindCallback("applyAction", [&context, &loader]() {
        std::cout << "[Event] Apply clicked!\n";

        // Query widgets dynamically by name
        auto* masterSlider = dynamic_cast<PerfUI::Slider*>(loader.findByName("MasterVolume"));
        auto* compassCheck = dynamic_cast<PerfUI::Checkbox*>(loader.findByName("EnableCompass"));

        float volume = masterSlider ? masterSlider->value() : 0.0f;
        bool compass = compassCheck ? compassCheck->isChecked() : false;

        std::cout << "  * Volume: " << volume << "%\n";
        std::cout << "  * Compass: " << (compass ? "Enabled" : "Disabled") << "\n";

        context.showToast("Settings", "Configuration applied successfully!", PerfUI::ToastType::Success);
    });

    // 3. Enable instant hot-reloading
    loader.enableHotReload(true);

    // 4. Load XML file into UI hierarchy
    PerfUI::UIElement* menu = loader.loadFile("settings.xml");
    if (!menu) {
        std::cerr << "Failed to load settings.xml!\n";
        for (const auto& err : loader.lastResult().errors) {
            std::cerr << "  Error: " << err << "\n";
        }
        return;
    }

    std::cout << "Successfully loaded settings menu with "
              << menu->children().size() << " top-level sections.\n";
}

int main() {
    PerfUI::UIContext context;
    context.setViewportSize({ 1920.0f, 1080.0f });

    SetupMarkupSettings(context);

    std::cout << "Markup settings demo initialized. Ready for hot-reload polling loop.\n";
    return 0;
}
