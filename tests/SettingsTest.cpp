#include "PerfUI/SettingsStore.h"
#include "PerfUI/SettingsView.h"
#include "PerfUI/SettingsSchema.h"
#include "PerfUI/ModSettingsManager.h"
#include "PerfUI/SettingsWindow.h"
#include "PerfUI/PerfUIApi.h"
#include "PerfUI/Localization.h"
#include "MockRenderBackend.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include "SimpleJson.h"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << (msg) << " (" << #cond << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)

using namespace PerfUI;

void TestTypesAndConversions() {
    std::cout << "[Test] Running TestTypesAndConversions..." << std::endl;

    // Bool conversion
    TEST_ASSERT(SettingValueToBool(true) == true, "bool true");
    TEST_ASSERT(SettingValueToBool(false) == false, "bool false");
    TEST_ASSERT(SettingValueToBool(1) == true, "int 1 to bool");
    TEST_ASSERT(SettingValueToBool(0) == false, "int 0 to bool");
    TEST_ASSERT(SettingValueToBool(std::string("true")) == true, "string 'true' to bool");
    TEST_ASSERT(SettingValueToBool(std::string("false")) == false, "string 'false' to bool");

    // Int conversion
    TEST_ASSERT(SettingValueToInt(42) == 42, "int 42");
    TEST_ASSERT(SettingValueToInt(true) == 1, "bool true to int");
    TEST_ASSERT(SettingValueToInt(false) == 0, "bool false to int");
    TEST_ASSERT(SettingValueToInt(42.7f) == 43, "float 42.7 to int (rounded)");
    TEST_ASSERT(SettingValueToInt(std::string("123")) == 123, "string '123' to int");

    // Float conversion
    TEST_ASSERT(std::abs(SettingValueToFloat(3.14f) - 3.14f) < 1e-4f, "float 3.14");
    TEST_ASSERT(std::abs(SettingValueToFloat(10) - 10.0f) < 1e-4f, "int 10 to float");
    TEST_ASSERT(std::abs(SettingValueToFloat(std::string("2.5")) - 2.5f) < 1e-4f, "string '2.5' to float");

    // String conversion
    TEST_ASSERT(SettingValueToString(std::string("Hello")) == "Hello", "string identity");
    TEST_ASSERT(SettingValueToString(true) == "true", "bool true to string");
    TEST_ASSERT(SettingValueToString(false) == "false", "bool false to string");
    TEST_ASSERT(SettingValueToString(100) == "100", "int 100 to string");

    // Color conversion
    Color c(255, 128, 64, 200);
    std::string hexStr = SettingValueToString(c);
    Color parsed = SettingValueToColor(hexStr);
    TEST_ASSERT(parsed == c, "Color to hex string and back");

    // KeyCode conversion
    uint32_t key = 0x11; // W key
    TEST_ASSERT(SettingValueToKeyCode(key) == key, "KeyCode uint32");
    TEST_ASSERT(SettingValueToKeyCode(static_cast<int32_t>(key)) == key, "KeyCode from int");

    std::cout << "  -> Passed TestTypesAndConversions" << std::endl;
}

void TestValidationAndClamping() {
    std::cout << "[Test] Running TestValidationAndClamping..." << std::endl;

    SettingDef intDef;
    intDef.key = "testInt";
    intDef.type = SettingType::Int;
    intDef.defaultValue = 10;
    intDef.minValue = 0.0f;
    intDef.maxValue = 100.0f;
    intDef.step = 5.0f;

    // Clamping min / max
    TEST_ASSERT(SettingValueToInt(intDef.validateAndClamp(-20)) == 0, "Clamped to min 0");
    TEST_ASSERT(SettingValueToInt(intDef.validateAndClamp(150)) == 100, "Clamped to max 100");
    // Stepping snap
    TEST_ASSERT(SettingValueToInt(intDef.validateAndClamp(12)) == 10, "Snapped 12 to 10 (step 5)");
    TEST_ASSERT(SettingValueToInt(intDef.validateAndClamp(13)) == 15, "Snapped 13 to 15 (step 5)");

    SettingDef floatDef;
    floatDef.key = "testFloat";
    floatDef.type = SettingType::Float;
    floatDef.defaultValue = 1.0f;
    floatDef.minValue = 0.0f;
    floatDef.maxValue = 2.0f;
    floatDef.step = 0.25f;

    TEST_ASSERT(std::abs(SettingValueToFloat(floatDef.validateAndClamp(-0.5f)) - 0.0f) < 1e-4f, "Clamped float min");
    TEST_ASSERT(std::abs(SettingValueToFloat(floatDef.validateAndClamp(3.0f)) - 2.0f) < 1e-4f, "Clamped float max");
    TEST_ASSERT(std::abs(SettingValueToFloat(floatDef.validateAndClamp(0.6f)) - 0.5f) < 1e-4f, "Snapped float to 0.5");
    TEST_ASSERT(std::abs(SettingValueToFloat(floatDef.validateAndClamp(0.7f)) - 0.75f) < 1e-4f, "Snapped float to 0.75");

    SettingDef choiceDef;
    choiceDef.key = "quality";
    choiceDef.type = SettingType::String;
    choiceDef.defaultValue = std::string("medium");
    choiceDef.options = { { "low", "Low" }, { "medium", "Medium" }, { "high", "High" } };

    TEST_ASSERT(SettingValueToString(choiceDef.validateAndClamp("high")) == "high", "Valid choice high");
    TEST_ASSERT(SettingValueToString(choiceDef.validateAndClamp("ultra")) == "medium", "Invalid choice falls back to default");

    std::cout << "  -> Passed TestValidationAndClamping" << std::endl;
}

void TestRegistrationAndStoreOperations() {
    std::cout << "[Test] Running TestRegistrationAndStoreOperations..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();

    SettingDef dToggle{ "general.enableFeature", SettingType::Bool, true };
    SettingDef dSlider{ "general.volume", SettingType::Float, 0.75f, 0.0f, 1.0f, 0.05f };
    SettingDef dChoice{ "display.mode", SettingType::String, std::string("windowed"), std::nullopt, std::nullopt, std::nullopt, "", { { "fullscreen", "Full" }, { "windowed", "Win" } } };

    store.registerSetting("MyMod", dToggle);
    store.registerSetting("MyMod", dSlider);
    store.registerSetting("MyMod", dChoice);

    TEST_ASSERT(store.hasSetting("MyMod", "general.enableFeature"), "Has toggle setting");
    TEST_ASSERT(store.hasSetting("MyMod", "general.volume"), "Has slider setting");
    TEST_ASSERT(store.getBool("MyMod", "general.enableFeature") == true, "Initial bool default");
    TEST_ASSERT(std::abs(store.getFloat("MyMod", "general.volume") - 0.75f) < 1e-4f, "Initial float default");
    TEST_ASSERT(store.getString("MyMod", "display.mode") == "windowed", "Initial choice default");
    TEST_ASSERT(store.isDefault("MyMod", "general.volume"), "Is default initially");

    // Modify values
    store.setBool("MyMod", "general.enableFeature", false);
    TEST_ASSERT(store.getBool("MyMod", "general.enableFeature") == false, "Updated bool");
    TEST_ASSERT(!store.isDefault("MyMod", "general.enableFeature"), "Not default after change");

    store.setFloat("MyMod", "general.volume", 0.5f);
    TEST_ASSERT(std::abs(store.getFloat("MyMod", "general.volume") - 0.5f) < 1e-4f, "Updated float");

    // Reset single setting
    store.reset("MyMod", "general.enableFeature");
    TEST_ASSERT(store.getBool("MyMod", "general.enableFeature") == true, "Reset single to default");
    TEST_ASSERT(store.isDefault("MyMod", "general.enableFeature"), "Is default after reset");

    // Reset all
    store.resetAll("MyMod");
    TEST_ASSERT(std::abs(store.getFloat("MyMod", "general.volume") - 0.75f) < 1e-4f, "ResetAll restored volume");
    TEST_ASSERT(store.isDefault("MyMod", "general.volume"), "Is default after resetAll");

    std::cout << "  -> Passed TestRegistrationAndStoreOperations" << std::endl;
}

void TestSubscriptionsAndReentrancy() {
    std::cout << "[Test] Running TestSubscriptionsAndReentrancy..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();

    SettingDef s1{ "speed", SettingType::Float, 10.0f, 0.0f, 100.0f };
    store.registerSetting("PhysicsMod", s1);

    int callCount = 0;
    float lastVal = 0.0f;

    auto subId = store.onChanged("PhysicsMod", "speed", [&](const std::string& /*modId*/, const std::string& /*key*/, const SettingValue& val) {
        ++callCount;
        lastVal = SettingValueToFloat(val);
        // Test re-entrancy: calling set() on the SAME key during notification must NOT cause recursion!
        store.setFloat("PhysicsMod", "speed", lastVal);
    });

    store.setFloat("PhysicsMod", "speed", 25.0f);
    TEST_ASSERT(callCount == 1, "Callback called exactly once without recursive looping");
    TEST_ASSERT(std::abs(lastVal - 25.0f) < 1e-4f, "Correct value received in callback");

    // Setting same value again should not trigger callback
    store.setFloat("PhysicsMod", "speed", 25.0f);
    TEST_ASSERT(callCount == 1, "No callback when value didn't change");

    // Mod-level subscription
    int modCallCount = 0;
    auto modSubId = store.onModChanged("PhysicsMod", [&](const std::string& /*modId*/, const std::string& /*key*/, const SettingValue& /*val*/) {
        ++modCallCount;
    });

    store.setFloat("PhysicsMod", "speed", 30.0f);
    TEST_ASSERT(callCount == 2, "Specific key callback invoked");
    TEST_ASSERT(modCallCount == 1, "Mod-level callback invoked");

    // Unsubscribe
    store.unsubscribe(subId);
    store.setFloat("PhysicsMod", "speed", 35.0f);
    TEST_ASSERT(callCount == 2, "Unsubscribed callback not invoked");
    TEST_ASSERT(modCallCount == 2, "Mod callback still invoked");

    store.unsubscribe(modSubId);
    std::cout << "  -> Passed TestSubscriptionsAndReentrancy" << std::endl;
}

void TestPersistenceAndCorruptFile() {
    std::cout << "[Test] Running TestPersistenceAndCorruptFile..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();

    auto tempDir = std::filesystem::temp_directory_path() / "PerfUI_Settings_Test";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    store.setSettingsDirectory(tempDir);

    SettingDef sBool{ "flag", SettingType::Bool, false };
    SettingDef sInt{ "count", SettingType::Int, 1 };
    SettingDef sColor{ "tint", SettingType::Color, Color(10, 20, 30, 255) };

    store.registerSetting("SaveMod", sBool);
    store.registerSetting("SaveMod", sInt);
    store.registerSetting("SaveMod", sColor);

    store.setBool("SaveMod", "flag", true);
    store.setInt("SaveMod", "count", 42);
    store.setColor("SaveMod", "tint", Color(100, 200, 50, 255));

    // Immediate save
    bool saved = store.saveToFile("SaveMod", true);
    TEST_ASSERT(saved, "Save to file succeeded");

    auto filePath = store.getSettingsFilePath("SaveMod");
    TEST_ASSERT(std::filesystem::exists(filePath), "JSON file exists on disk");

    // Clear and reload
    store.clearAll();
    store.setSettingsDirectory(tempDir);
    store.registerSetting("SaveMod", sBool);
    store.registerSetting("SaveMod", sInt);
    store.registerSetting("SaveMod", sColor);

    TEST_ASSERT(store.getBool("SaveMod", "flag") == true, "Reloaded bool value");
    TEST_ASSERT(store.getInt("SaveMod", "count") == 42, "Reloaded int value");
    TEST_ASSERT(store.getColor("SaveMod", "tint") == Color(100, 200, 50, 255), "Reloaded color value");

    // Test Corrupt File handling
    {
        std::ofstream corruptFile(filePath, std::ios::trunc);
        corruptFile << "{ \"version\": 1, invalid_json_syntax [[[ ...";
        corruptFile.close();
    }

    store.clearAll();
    store.setSettingsDirectory(tempDir);
    store.registerSetting("SaveMod", sBool);
    store.registerSetting("SaveMod", sInt);

    // Loading corrupt file must not crash, should create .bak and return defaults
    auto bakPath = filePath;
    bakPath.replace_extension(".json.bak");
    TEST_ASSERT(std::filesystem::exists(bakPath), "Corrupt file backed up to .bak");
    TEST_ASSERT(store.getBool("SaveMod", "flag") == false, "Fallback to default bool on corrupt file");
    TEST_ASSERT(store.getInt("SaveMod", "count") == 1, "Fallback to default int on corrupt file");

    std::filesystem::remove_all(tempDir);
    std::cout << "  -> Passed TestPersistenceAndCorruptFile" << std::endl;
}

void TestDebounceSave() {
    std::cout << "[Test] Running TestDebounceSave..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();

    auto tempDir = std::filesystem::temp_directory_path() / "PerfUI_Debounce_Test";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    store.setSettingsDirectory(tempDir);

    SettingDef sNum{ "value", SettingType::Int, 0 };
    store.registerSetting("DebounceMod", sNum);

    // Change setting (queued with 400ms debounce)
    store.setInt("DebounceMod", "value", 999);
    auto filePath = store.getSettingsFilePath("DebounceMod");
    TEST_ASSERT(!std::filesystem::exists(filePath), "File NOT saved immediately due to debounce");

    // Advance 0.2s (not enough)
    store.update(0.2f);
    TEST_ASSERT(!std::filesystem::exists(filePath), "File still not saved after 200ms");

    // Advance another 0.3s (total 0.5s > 0.4s)
    store.update(0.3f);
    store.flushAll();
    TEST_ASSERT(std::filesystem::exists(filePath), "File saved after debounce timer elapsed");

    std::filesystem::remove_all(tempDir);
    std::cout << "  -> Passed TestDebounceSave" << std::endl;
}

void TestConcurrency() {
    std::cout << "[Test] Running TestConcurrency..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();

    for (int i = 0; i < 20; ++i) {
        store.registerSetting("ThreadMod", SettingDef{ "key_" + std::to_string(i), SettingType::Int, 0 });
    }

    constexpr int kThreads = 8;
    constexpr int kIterations = 200;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&store, t]() {
            for (int it = 0; it < kIterations; ++it) {
                int keyIdx = (t + it) % 20;
                std::string k = "key_" + std::to_string(keyIdx);
                int oldVal = store.getInt("ThreadMod", k);
                store.setInt("ThreadMod", k, oldVal + 1, false);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    int total = 0;
    for (int i = 0; i < 20; ++i) {
        total += store.getInt("ThreadMod", "key_" + std::to_string(i));
    }

    std::cout << "  * Concurrent iterations total accumulated: " << total << std::endl;
    TEST_ASSERT(total > 0, "Concurrent mutations succeeded without race crash or deadlock");

    std::cout << "  -> Passed TestConcurrency" << std::endl;
}

void TestWidgetBindingAndTwoWaySync() {
    std::cout << "[Test] Running TestWidgetBindingAndTwoWaySync..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();

    // Register test definitions
    store.registerSetting("BindingMod", SettingDef{ "enableShadows", SettingType::Bool, true });
    store.registerSetting("BindingMod", SettingDef{ "shadowQuality", SettingType::Float, 50.0f, 0.0f, 100.0f, 1.0f });
    store.registerSetting("BindingMod", SettingDef{ "preset", SettingType::String, std::string("high"), std::nullopt, std::nullopt, std::nullopt, "", { { "low", "Low" }, { "high", "High" } } });
    store.registerSetting("BindingMod", SettingDef{ "profile", SettingType::String, std::string("DefaultUser") });
    store.registerSetting("BindingMod", SettingDef{ "rebindKey", SettingType::KeyCode, static_cast<uint32_t>(0x46) }); // F key
    store.registerSetting("BindingMod", SettingDef{ "accentColor", SettingType::Color, Color(255, 128, 0, 255) });

    // Create SettingsView
    SettingsView view("TestSettingsView", "BindingMod");
    TEST_ASSERT(view.hasClass("settings-page"), "SettingsView has 'settings-page' class");

    auto* sec = view.addSection("Graphics Settings");
    TEST_ASSERT(sec->hasClass("settings-section"), "SettingsSection has 'settings-section' class");

    auto* cb = sec->addToggle("Enable Shadows", "enableShadows", "Toggle shadow rendering");
    auto* sl = sec->addSlider("Shadow Quality", "shadowQuality", "Adjust shadow map resolution");
    auto* combo = sec->addChoice("Preset", "preset", "Select graphics preset");
    auto* input = sec->addTextInput("Profile", "profile", "Enter profile name");
    auto* keyBtn = sec->addKeybind("Menu Key", "rebindKey", "Open menu key");
    auto* colorBtn = sec->addColor("Accent Color", "accentColor", "UI accent color");

    auto* hdr = sec->addHeader("Advanced Options");
    TEST_ASSERT(hdr->hasClass("settings-header"), "Header has 'settings-header' class");

    auto* txt = sec->addText("Note: Restart may be required for some settings.");
    TEST_ASSERT(txt->hasClass("settings-help"), "Text has 'settings-help' class");

    auto* sep = sec->addSeparator();
    TEST_ASSERT(sep != nullptr, "Separator added");

    // 1. Check initial values from store
    TEST_ASSERT(cb->isChecked() == true, "Checkbox matches store default true");
    TEST_ASSERT(std::abs(sl->value() - 50.0f) < 1e-4f, "Slider matches store default 50.0");
    TEST_ASSERT(combo->selectedIndex() == 1, "ComboBox selected index 1 ('high')");
    TEST_ASSERT(input->text() == "DefaultUser", "TextInput matches store string");

    // Check PSS classes on controls
    TEST_ASSERT(cb->hasClass("settings-control"), "Checkbox has 'settings-control'");
    TEST_ASSERT(sl->hasClass("settings-control"), "Slider has 'settings-control'");
    TEST_ASSERT(combo->hasClass("settings-control"), "ComboBox has 'settings-control'");
    TEST_ASSERT(input->hasClass("settings-control"), "TextInput has 'settings-control'");
    TEST_ASSERT(keyBtn->hasClass("settings-control"), "KeybindButton has 'settings-control'");
    TEST_ASSERT(colorBtn->hasClass("settings-control"), "ColorButton has 'settings-control'");

    // Check modified status before changes
    TEST_ASSERT(store.isDefault("BindingMod", "enableShadows"), "Initially default");

    // 2. UI -> Store mutation
    // Toggle checkbox in UI
    cb->onAction(NavDirection::Up); // triggers onToggle
    TEST_ASSERT(store.getBool("BindingMod", "enableShadows") == false, "Store updated from UI checkbox toggle");
    TEST_ASSERT(!store.isDefault("BindingMod", "enableShadows"), "Setting marked as modified");

    // 3. Store -> UI mutation
    store.setFloat("BindingMod", "shadowQuality", 80.0f);
    TEST_ASSERT(std::abs(sl->value() - 80.0f) < 1e-4f, "Slider UI updated from store set");

    // 4. Conditional logic: visibleIf and enabledIf
    sec->setRowVisibleIf("shadowQuality", SettingCondition("enableShadows"));
    sec->setRowEnabledIf("preset", SettingCondition("enableShadows"));

    // When enableShadows is false:
    // shadowQuality row must be hidden
    // preset control must be disabled
    sec->updateConditions();

    auto hasSlider = [](UIElement* el) -> bool {
        for (const auto& c1 : el->children()) {
            if (dynamic_cast<Slider*>(c1.get())) return true;
            for (const auto& c2 : c1->children()) {
                if (dynamic_cast<Slider*>(c2.get())) return true;
            }
        }
        return false;
    };
    auto hasCombo = [](UIElement* el) -> bool {
        for (const auto& c1 : el->children()) {
            if (dynamic_cast<ComboBox*>(c1.get())) return true;
            for (const auto& c2 : c1->children()) {
                if (dynamic_cast<ComboBox*>(c2.get())) return true;
            }
        }
        return false;
    };

    // Verify row for shadowQuality is hidden
    bool foundQualityRow = false;
    bool foundPresetRow = false;
    for (const auto& rowChild : sec->itemsContainer()->children()) {
        if (!rowChild) continue;
        if (hasSlider(rowChild.get())) {
            foundQualityRow = true;
            TEST_ASSERT(rowChild->isVisible() == false, "Shadow quality row hidden when enableShadows is false");
        }
        if (hasCombo(rowChild.get())) {
            foundPresetRow = true;
            TEST_ASSERT(rowChild->hasClass("settings-disabled"), "Preset row has 'settings-disabled' when condition false");
            TEST_ASSERT(combo->isEnabled() == false, "Preset ComboBox disabled when condition false");
        }
    }
    TEST_ASSERT(foundQualityRow, "Found shadow quality row");
    TEST_ASSERT(foundPresetRow, "Found preset row");

    // Turn shadows back on in store
    store.setBool("BindingMod", "enableShadows", true);
    // UI checkbox updates, conditions recalculate
    TEST_ASSERT(cb->isChecked() == true, "Checkbox updated to true");
    for (const auto& rowChild : sec->itemsContainer()->children()) {
        if (!rowChild) continue;
        if (hasSlider(rowChild.get())) {
            TEST_ASSERT(rowChild->isVisible() == true, "Shadow quality row visible when enableShadows is true");
        }
        if (hasCombo(rowChild.get())) {
            TEST_ASSERT(!rowChild->hasClass("settings-disabled"), "Preset row no longer disabled");
            TEST_ASSERT(combo->isEnabled() == true, "Preset ComboBox enabled");
        }
    }

    // 5. Reset to defaults
    // Reset section
    sec->resetAllToDefaults();
    TEST_ASSERT(store.isDefault("BindingMod", "enableShadows"), "Reset section restored shadows default");
    TEST_ASSERT(store.isDefault("BindingMod", "shadowQuality"), "Reset section restored quality default");
    TEST_ASSERT(std::abs(sl->value() - 50.0f) < 1e-4f, "Slider restored to 50.0");

    // 6. Keybind Conflict Detection
    sec->setKeybindConflict("rebindKey", true);
    TEST_ASSERT(keyBtn->hasClass("settings-conflict"), "Keybind button has 'settings-conflict' class");
    sec->setKeybindConflict("rebindKey", false);
    TEST_ASSERT(!keyBtn->hasClass("settings-conflict"), "Keybind button removed 'settings-conflict' class");

    std::cout << "  -> Passed TestWidgetBindingAndTwoWaySync" << std::endl;
}

void TestDeclarativeSettingsSchema() {
    std::cout << "[Test] Running TestDeclarativeSettingsSchema..." << std::endl;

    auto& loc = LocalizationService::instance();
    loc.setTranslation("en", "MOD_TITLE", "Survival Mode Overhaul");
    loc.setTranslation("en", "PAGE_GENERAL", "General Features");
    loc.setTranslation("en", "PAGE_DIFFICULTY", "Difficulty Tuning");

    // 1. Valid multi-page schema with all types, localization, valueOptions
    std::string validJson = R"({
        "perfuiSchema": 1,
        "modName": "SurvivalMod",
        "displayName": "$MOD_TITLE",
        "pages": [
            {
                "pageDisplayName": "$PAGE_GENERAL",
                "content": [
                    { "type": "header", "text": "Core Mechanics" },
                    { "type": "toggle", "id": "enableHunger", "text": "Enable Hunger", "help": "Characters require food", "groupControl": true, "valueOptions": { "default": true } },
                    { "type": "slider", "id": "hungerRate", "text": "Hunger Rate", "groupCondition": true, "valueOptions": { "min": 0.1, "max": 5.0, "step": 0.1, "default": 1.5, "formatString": "%.1fx" } },
                    { "type": "separator" },
                    { "type": "color", "id": "hungerHudColor", "text": "Hunger Icon Color", "valueOptions": { "default": "#FFAA00" } },
                    { "type": "keymap", "id": "eatQuickKey", "text": "Quick Eat Hotkey", "valueOptions": { "default": 48 } }
                ]
            },
            {
                "pageDisplayName": "$PAGE_DIFFICULTY",
                "content": [
                    { "type": "text", "text": "Adjust penalty multipliers below." },
                    { "type": "stepper", "id": "damageMult", "text": "Exhaustion Damage", "valueOptions": { "min": 1, "max": 10, "step": 1, "default": 3 } },
                    { "type": "menu", "id": "preset", "text": "Presets", "valueOptions": {
                        "options": [
                            { "id": "easy", "text": "Casual" },
                            { "id": "normal", "text": "Standard" },
                            { "id": "hardcore", "text": "Hardcore" }
                        ],
                        "default": "normal"
                    }},
                    { "type": "input", "id": "customProfile", "text": "Profile Name", "valueOptions": { "default": "DefaultProfile", "placeholder": "Enter name..." } },
                    { "type": "empty", "height": 12.0 }
                ]
            }
        ]
    })";

    SettingsView view("TestView");
    auto res = SettingsSchemaLoader::loadFromString(validJson, "ValidSchema.json", &view);

    TEST_ASSERT(res.success, "Schema loaded successfully");
    TEST_ASSERT(res.modId == "SurvivalMod", "modId correctly extracted");
    TEST_ASSERT(res.displayName == "Survival Mode Overhaul", "Localized displayName resolved");
    TEST_ASSERT(res.pageTitles.size() == 2, "2 pages registered");
    TEST_ASSERT(res.pageTitles[0] == "General Features", "Page 1 localized title");
    TEST_ASSERT(res.pageTitles[1] == "Difficulty Tuning", "Page 2 localized title");

    // Verify settings registered in store with proper defaults
    auto& store = SettingsStore::GetSingleton();
    TEST_ASSERT(store.getBool("SurvivalMod", "enableHunger") == true, "Hunger default true");
    TEST_ASSERT(std::abs(store.getFloat("SurvivalMod", "hungerRate") - 1.5f) < 1e-4f, "Hunger rate default 1.5");
    Color expectedColor = Color::FromHex("#FFAA00");
    Color actualColor = store.getColor("SurvivalMod", "hungerHudColor");
    TEST_ASSERT(actualColor.r == expectedColor.r && actualColor.g == expectedColor.g && actualColor.b == expectedColor.b, "Color default #FFAA00");
    TEST_ASSERT(store.getKeyCode("SurvivalMod", "eatQuickKey") == 48, "Hotkey default 48");
    TEST_ASSERT(store.getInt("SurvivalMod", "damageMult") == 3, "Stepper default 3");
    TEST_ASSERT(store.getString("SurvivalMod", "preset") == "normal", "Menu default normal");
    TEST_ASSERT(store.getString("SurvivalMod", "customProfile") == "DefaultProfile", "Input default DefaultProfile");

    // 2. Corrupt / invalid JSON syntax
    std::string corruptJson = "{ \"modName\": \"BadJson\", \"pages\": [ { unquoted_key } ";
    auto corruptRes = SettingsSchemaLoader::loadFromString(corruptJson, "Corrupt.json", &view);
    TEST_ASSERT(!corruptRes.success, "Corrupt JSON reports failure");
    TEST_ASSERT(!corruptRes.errors.empty(), "Corrupt JSON records errors");

    // 3. Unknown types and missing ID handling (resilience)
    std::string resilientJson = R"({
        "perfuiSchema": 1,
        "modName": "ResilientMod",
        "displayName": "Resilience Test",
        "content": [
            { "type": "header", "text": "Valid Header" },
            { "type": "unknown_future_widget", "text": "Should Skip" },
            { "type": "toggle", "text": "Missing ID toggle" },
            { "type": "toggle", "id": "validToggle", "text": "Valid Toggle", "valueOptions": { "default": true } }
        ]
    })";

    SettingsView resilientView("ResilientView");
    auto resilientRes = SettingsSchemaLoader::loadFromString(resilientJson, "Resilient.json", &resilientView);
    TEST_ASSERT(!resilientRes.warnings.empty(), "Recorded warning for unknown type");
    TEST_ASSERT(!resilientRes.errors.empty(), "Recorded error for missing id on toggle");
    TEST_ASSERT(store.hasSetting("ResilientMod", "validToggle"), "Valid toggle was still registered and created despite errors");
    TEST_ASSERT(store.getBool("ResilientMod", "validToggle") == true, "Valid toggle value set");

    // 4. File round-trip test
    auto tempFile = std::filesystem::temp_directory_path() / "PerfUI_Test_Schema.json";
    {
        std::ofstream out(tempFile, std::ios::trunc);
        out << validJson;
    }
    SettingsView fileView("FileView");
    auto fileRes = SettingsSchemaLoader::loadFromFile(tempFile, &fileView);
    TEST_ASSERT(fileRes.success, "loadFromFile succeeded");
    TEST_ASSERT(fileRes.modId == "SurvivalMod", "loadFromFile modId matches");
    std::filesystem::remove(tempFile);

    std::cout << "  -> Passed TestDeclarativeSettingsSchema" << std::endl;
}

void TestModSettingsManagerAndApiV4() {
    std::cout << "[Test] Running TestModSettingsManagerAndApiV4..." << std::endl;

    // 1. ABI Layout & Sizes
#if defined(_MSC_VER) && defined(_M_X64)
    TEST_ASSERT(sizeof(PerfUI::API::IPerfUI_v1) == 168, "IPerfUI_v1 size is 168");
    TEST_ASSERT(sizeof(PerfUI::API::IPerfUI_v2) == 176, "IPerfUI_v2 size is 176");
    TEST_ASSERT(sizeof(PerfUI::API::IPerfUI_v3) == 232, "IPerfUI_v3 size is 232");
    TEST_ASSERT(sizeof(PerfUI::API::IPerfUI_v4) == 376, "IPerfUI_v4 size is 376");
    TEST_ASSERT(offsetof(PerfUI::API::IPerfUI_v2, v1) == 0, "IPerfUI_v2 offset v1 == 0");
    TEST_ASSERT(offsetof(PerfUI::API::IPerfUI_v3, v2) == 0, "IPerfUI_v3 offset v2 == 0");
    TEST_ASSERT(offsetof(PerfUI::API::IPerfUI_v4, v3) == 0, "IPerfUI_v4 offset v3 == 0");
#endif

    // 2. Populate API v4
    PerfUI::API::IPerfUI_v4 api{};
    PerfUI::API::PopulateApiV4(api);

    auto& mgr = PerfUI::ModSettingsManager::GetSingleton();
    mgr.clear();

    // 3. Register mods via API v4
    // Mod 1: via JSON
    auto tempJson = std::filesystem::temp_directory_path() / "ModAlpha.json";
    {
        std::ofstream out(tempJson, std::ios::trunc);
        out << R"({
            "perfuiSchema": 1,
            "modName": "ModAlpha",
            "displayName": "Alpha Mod",
            "content": [
                { "type": "toggle", "id": "alphaOption", "text": "Alpha Option", "valueOptions": { "default": true } }
            ]
        })";
    }

    bool rJson = api.RegisterModSettingsJson("ModAlpha", "Alpha Mod", tempJson.string().c_str());
    TEST_ASSERT(rJson, "RegisterModSettingsJson succeeded");

    // Mod 2: via C++ page builder
    auto pageBuilderFn = [](void* /*userData*/, PerfUI::API::SettingsPageBuilder* b) {
        b->AddHeader(b, "Beta Mod Controls");
        b->AddToggle(b, "Beta Toggle", "betaToggle", "Toggle something", false);
        b->AddSlider(b, "Beta Slider", "betaSlider", "Slide something", 0.0f, 10.0f, 0.5f, 5.0f, "%.1f");
    };

    bool rPage = api.RegisterModSettingsPage("ModBeta", "Beta Mod", pageBuilderFn, nullptr);
    TEST_ASSERT(rPage, "RegisterModSettingsPage succeeded");
    TEST_ASSERT(mgr.getRegisteredMods().size() == 2, "2 mods registered");

    // 4. Repeated registration with same modId updates rather than duplicates
    bool rUpdate = api.RegisterModSettingsJson("ModAlpha", "Alpha Mod Renamed", tempJson.string().c_str());
    TEST_ASSERT(rUpdate, "Repeated registration succeeded");
    TEST_ASSERT(mgr.getRegisteredMods().size() == 2, "Still 2 mods (no duplicate)");
    TEST_ASSERT(mgr.getMod("ModAlpha")->displayName == "Alpha Mod Renamed", "DisplayName updated");

    // 5. Attach to SettingsWindow
    SettingsWindow window("TestSettingsWindow");
    mgr.attachToWindow(&window);
    auto* betaView = window.getOrCreateCategory("mod_ModBeta", "Beta Mod");
    TEST_ASSERT(betaView != nullptr, "Category created for ModBeta");

    // 6. Get / Set API operations
    api.SetSettingBool("ModBeta", "betaToggle", true);
    TEST_ASSERT(api.GetSettingBool("ModBeta", "betaToggle", false) == true, "api.GetSettingBool returns true");

    api.SetSettingInt("ModBeta", "counter", 100);
    TEST_ASSERT(api.GetSettingInt("ModBeta", "counter", 0) == 100, "api.GetSettingInt returns 100");

    api.SetSettingFloat("ModBeta", "betaSlider", 7.5f);
    TEST_ASSERT(std::abs(api.GetSettingFloat("ModBeta", "betaSlider", 0.0f) - 7.5f) < 1e-4f, "api.GetSettingFloat returns 7.5");

    api.SetSettingString("ModBeta", "greeting", "Hello Skyrim");
    TEST_ASSERT(std::string(api.GetSettingString("ModBeta", "greeting", "")) == "Hello Skyrim", "api.GetSettingString returns Hello Skyrim");

    api.SetSettingColor("ModBeta", "tint", 0x11223344);
    TEST_ASSERT(api.GetSettingColor("ModBeta", "tint", 0) == 0x11223344, "api.GetSettingColor returns 0x11223344");

    api.SetSettingKey("ModBeta", "menuKey", 64);
    TEST_ASSERT(api.GetSettingKey("ModBeta", "menuKey", 0) == 64, "api.GetSettingKey returns 64");

    // 7. Subscriptions through API v4
    struct SubContext {
        bool called{ false };
        std::string mod;
        std::string key;
    } subCtx;

    uint64_t subId = api.SubscribeSettingChanged("ModBeta", "betaSlider", [](void* ud, const char* m, const char* k) {
        auto* ctx = static_cast<SubContext*>(ud);
        ctx->called = true;
        ctx->mod = m ? m : "";
        ctx->key = k ? k : "";
    }, &subCtx);

    TEST_ASSERT(subId != 0, "Subscription created");
    api.SetSettingFloat("ModBeta", "betaSlider", 9.0f);
    TEST_ASSERT(subCtx.called == true, "Subscription callback fired");
    TEST_ASSERT(subCtx.mod == "ModBeta", "Callback received correct mod");
    TEST_ASSERT(subCtx.key == "betaSlider", "Callback received correct key");

    subCtx.called = false;
    api.UnsubscribeSettingChanged(subId);
    api.SetSettingFloat("ModBeta", "betaSlider", 1.0f);
    TEST_ASSERT(subCtx.called == false, "Callback not fired after unsubscribe");

    // 8. Open & Close Settings
    mgr.setSettingsWindow(&window);
    api.OpenSettings("ModBeta");
    TEST_ASSERT(window.isVisible() == true, "Window opened via api.OpenSettings");
    api.CloseSettings();
    TEST_ASSERT(window.isVisible() == false, "Window closed via api.CloseSettings");

    // 9. Client wrappers without DLL loaded (fallback safety)
    TEST_ASSERT(!PerfUI::Client::RegisterModSettingsJson("Test", "Test", "path.json"), "Client wrapper returns false when DLL not present");
    TEST_ASSERT(!PerfUI::Client::RegisterModSettingsPage("Test", "Test", nullptr, nullptr), "Client wrapper returns false when DLL not present");
    PerfUI::Client::OpenSettings("Test"); // Should not crash
    PerfUI::Client::CloseSettings();      // Should not crash

    std::filesystem::remove(tempJson);
    std::cout << "  -> Passed TestModSettingsManagerAndApiV4" << std::endl;
}

void TestHotkeysAndConflictResolution() {
    std::cout << "[Test] Running TestHotkeysAndConflictResolution..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    auto& mgr = ModSettingsManager::GetSingleton();
    mgr.clear();

    // 1. Hotkey persistence & reactive update
    UIContext ctx;
    bool hotkeyTriggered = false;
    ctx.registerHotkey("TestMod:menuKey", 0x48, [&hotkeyTriggered]() {
        hotkeyTriggered = true;
    });

    TEST_ASSERT(ctx.hasHotkey(0x48), "Context initially has hotkey 0x48");
    ctx.triggerHotkey(0x48);
    TEST_ASSERT(hotkeyTriggered == true, "Hotkey 0x48 triggered callback");

    // Change key in store
    hotkeyTriggered = false;
    store.setKeyCode("TestMod", "menuKey", 0x49);
    TEST_ASSERT(!ctx.hasHotkey(0x48), "Context no longer has hotkey 0x48 after store update");
    TEST_ASSERT(ctx.hasHotkey(0x49), "Context now has hotkey 0x49");

    ctx.triggerHotkey(0x49);
    TEST_ASSERT(hotkeyTriggered == true, "Hotkey 0x49 triggered callback");

    // Re-register (e.g. game reload) with old default 0x48
    UIContext freshCtx;
    freshCtx.registerHotkey("TestMod:menuKey", 0x48, [&hotkeyTriggered]() {
        hotkeyTriggered = true;
    });
    TEST_ASSERT(freshCtx.hasHotkey(0x49), "Fresh context used persisted user override 0x49 instead of default 0x48");

    // 2. Conflict detection & unbinding
    mgr.registerPage("ModA", "Mod A", [](void*, API::SettingsPageBuilder* b) {
        b->AddKeybind(b, "Quick Save", "quickSave", "", 57);
    }, nullptr);

    mgr.registerPage("ModB", "Mod B", [](void*, API::SettingsPageBuilder* b) {
        b->AddKeybind(b, "Jump", "jumpKey", "", 57);
    }, nullptr);

    SettingsWindow win("Win");
    mgr.attachToWindow(&win);

    auto conflicts = mgr.findKeybindConflicts("ModA", "quickSave", 57);
    TEST_ASSERT(conflicts.size() == 1, "Found 1 conflict for key 57");
    TEST_ASSERT(conflicts[0].modId == "ModB" && conflicts[0].key == "jumpKey", "Conflict is ModB::jumpKey");

    // Unbind conflicting keys
    mgr.unbindConflictingKeybinds("ModA", "quickSave", 57);
    TEST_ASSERT(store.getKeyCode("ModB", "jumpKey") == 0, "ModB jumpKey successfully unbound to 0");

    auto conflictsAfter = mgr.findKeybindConflicts("ModA", "quickSave", 57);
    TEST_ASSERT(conflictsAfter.empty(), "No more conflicts after unbind");

    std::cout << "  -> Passed TestHotkeysAndConflictResolution" << std::endl;
}

void TestAutoLoaderAndSchemaMarker() {
    std::cout << "[Test] Running TestAutoLoaderAndSchemaMarker..." << std::endl;

    auto& mgr = ModSettingsManager::GetSingleton();
    mgr.clear();

    // 1. Explicit schema validation of perfuiSchema marker
    std::string missingMarkerJson = R"({
        "modName": "MissingMarkerMod",
        "displayName": "No Marker",
        "content": []
    })";
    SettingsView view("TestView");
    auto resMissing = SettingsSchemaLoader::loadFromString(missingMarkerJson, "MissingMarker.json", &view);
    TEST_ASSERT(!resMissing.success, "Schema missing 'perfuiSchema' must fail");
    TEST_ASSERT(!resMissing.errors.empty(), "Error message recorded for missing 'perfuiSchema'");

    std::string zeroMarkerJson = R"({
        "perfuiSchema": 0,
        "modName": "ZeroMarkerMod",
        "displayName": "Zero Marker",
        "content": []
    })";
    auto resZero = SettingsSchemaLoader::loadFromString(zeroMarkerJson, "ZeroMarker.json", &view);
    TEST_ASSERT(!resZero.success, "Schema with 'perfuiSchema: 0' must fail");

    std::string validMarkerJson = R"({
        "perfuiSchema": 1,
        "modName": "ValidMarkerMod",
        "displayName": "Valid Marker",
        "content": []
    })";
    auto resValid = SettingsSchemaLoader::loadFromString(validMarkerJson, "ValidMarker.json", &view);
    TEST_ASSERT(resValid.success, "Schema with 'perfuiSchema: 1' must succeed");

    // 2. Directory scan with alien config.json, unmarked files, broken files, and valid files
    auto tempScanDir = std::filesystem::temp_directory_path() / "PerfUI_AutoLoad_Test";
    std::filesystem::remove_all(tempScanDir);
    std::filesystem::create_directories(tempScanDir);

    // Folder 1: Alien mod with config.json -> Must be completely ignored
    auto alienDir = tempScanDir / "AlienMod";
    std::filesystem::create_directories(alienDir);
    {
        std::ofstream f(alienDir / "config.json");
        f << "{ \"alienProperty\": 42, \"anotherMod\": true }";
    }

    // Folder 2: Unmarked perfui.settings.json -> Must be skipped
    auto unmarkedDir = tempScanDir / "UnmarkedMod";
    std::filesystem::create_directories(unmarkedDir);
    {
        std::ofstream f(unmarkedDir / "perfui.settings.json");
        f << "{ \"modName\": \"UnmarkedMod\", \"content\": [] }";
    }

    // Folder 3: Valid Mod Alpha with perfui.settings.json
    auto modADir = tempScanDir / "ModA";
    std::filesystem::create_directories(modADir);
    {
        std::ofstream f(modADir / "perfui.settings.json");
        f << "{ \"perfuiSchema\": 1, \"modName\": \"ModA\", \"displayName\": \"Mod A Title\", \"content\": [] }";
    }

    // Folder 4: Corrupted JSON file -> Must log and continue, NOT blocking ModB
    auto brokenDir = tempScanDir / "BrokenMod";
    std::filesystem::create_directories(brokenDir);
    {
        std::ofstream f(brokenDir / "perfui.settings.json");
        f << "{ \"perfuiSchema\": 1, unquoted_garbage_here ";
    }

    // Folder 5: Valid Mod Beta with perfui.settings.json
    auto modBDir = tempScanDir / "ModB";
    std::filesystem::create_directories(modBDir);
    {
        std::ofstream f(modBDir / "perfui.settings.json");
        f << "{ \"perfuiSchema\": 1, \"modName\": \"ModB\", \"displayName\": \"Mod B Title\", \"content\": [] }";
    }

    size_t loadedCount = mgr.scanAndRegisterSettings(tempScanDir);
    TEST_ASSERT(loadedCount == 2, "Scanned 2 valid mods (ModA and ModB)");
    TEST_ASSERT(mgr.getMod("ModA") != nullptr, "ModA was registered");
    TEST_ASSERT(mgr.getMod("ModB") != nullptr, "ModB was registered despite BrokenMod error");
    TEST_ASSERT(mgr.getMod("AlienMod") == nullptr, "Alien config.json was not registered");
    TEST_ASSERT(mgr.getMod("UnmarkedMod") == nullptr, "Unmarked file was not registered");
    TEST_ASSERT(mgr.getMod("BrokenMod") == nullptr, "Broken file was not registered");

    // Clean up temp directory
    std::error_code ec;
    std::filesystem::remove_all(tempScanDir, ec);

    std::cout << "  -> Passed TestAutoLoaderAndSchemaMarker" << std::endl;
}

void TestSafeStringRetrievalAndApiV5() {
    std::cout << "[Test] Running TestSafeStringRetrievalAndApiV5..." << std::endl;

    // 1. ABI Verification for v5
    static_assert(sizeof(PerfUI::API::IPerfUI_v5) == 384, "IPerfUI_v5 size mismatch");
    static_assert(offsetof(PerfUI::API::IPerfUI_v5, v4) == 0, "IPerfUI_v5::v4 offset mismatch");

    PerfUI::API::IPerfUI_v5 api5{};
    PerfUI::API::PopulateApiV5(api5);

    auto& store = SettingsStore::GetSingleton();
    std::string mod = "StringMod";

    // 2. Unknown key returns default
    const char* strDef = api5.v4.GetSettingString(mod.c_str(), "missingKey", "fallback_default");
    TEST_ASSERT(std::string(strDef) == "fallback_default", "GetSettingString returns fallback for missing key");

    char buf[128];
    size_t written = api5.GetSettingStringCopy(mod.c_str(), "missingKey", buf, sizeof(buf), "copy_fallback");
    TEST_ASSERT(std::string(buf) == "copy_fallback", "GetSettingStringCopy returns copy_fallback for missing key");
    TEST_ASSERT(written == std::string("copy_fallback").size(), "GetSettingStringCopy returns correct copied length");

    // 3. Empty string test
    api5.v4.SetSettingString(mod.c_str(), "emptyStr", "");
    TEST_ASSERT(std::string(api5.v4.GetSettingString(mod.c_str(), "emptyStr", "def")) == "", "Empty string retrieved correctly");
    written = api5.GetSettingStringCopy(mod.c_str(), "emptyStr", buf, sizeof(buf), "def");
    TEST_ASSERT(written == 0, "Empty string returns length 0");
    TEST_ASSERT(buf[0] == '\0', "Buffer is null terminated");

    // 4. Large string (> 4 KB) test
    std::string largeStr(5000, 'X');
    largeStr[2500] = 'Y';
    api5.v4.SetSettingString(mod.c_str(), "largeStr", largeStr.c_str());

    const char* retrievedLarge = api5.v4.GetSettingString(mod.c_str(), "largeStr", "");
    TEST_ASSERT(retrievedLarge != nullptr, "Retrieved large string is non-null");
    TEST_ASSERT(std::string(retrievedLarge) == largeStr, "Large string (>4KB) matches original content");

    // Buffer query (nullptr buffer) returns required size
    size_t reqSize = api5.GetSettingStringCopy(mod.c_str(), "largeStr", nullptr, 0, "");
    TEST_ASSERT(reqSize == largeStr.size() + 1, "Querying with null buffer returns full required size");

    // Truncated buffer test
    char smallBuf[10];
    written = api5.GetSettingStringCopy(mod.c_str(), "largeStr", smallBuf, sizeof(smallBuf), "");
    TEST_ASSERT(written == 9, "Truncated buffer writes sizeof-1 bytes");
    TEST_ASSERT(smallBuf[9] == '\0', "Truncated buffer is null terminated");
    TEST_ASSERT(std::string(smallBuf) == "XXXXXXXXX", "Truncated buffer contains first 9 chars");

    // 5. Multithreaded stress test: thread 1 writes, thread 2 reads
    std::atomic<bool> stopStress{ false };
    std::atomic<int> readCount{ 0 };

    std::thread writer([&]() {
        int counter = 0;
        while (!stopStress.load()) {
            std::string val = "dynamic_string_value_" + std::to_string(counter++);
            api5.v4.SetSettingString(mod.c_str(), "raceKey", val.c_str());
            std::this_thread::sleep_for(std::chrono::microseconds(20));
        }
    });

    std::thread reader([&]() {
        char localBuf[256];
        while (!stopStress.load()) {
            const char* s = api5.v4.GetSettingString(mod.c_str(), "raceKey", "def");
            TEST_ASSERT(s != nullptr, "s is non-null");
            TEST_ASSERT(std::string_view(s).rfind("dynamic_string_value_", 0) == 0 || std::string_view(s) == "def", "Valid string read");

            size_t copied = api5.GetSettingStringCopy(mod.c_str(), "raceKey", localBuf, sizeof(localBuf), "def");
            TEST_ASSERT(copied > 0, "Copied positive length");
            TEST_ASSERT(std::string_view(localBuf).rfind("dynamic_string_value_", 0) == 0 || std::string_view(localBuf) == "def", "Valid copy read");

            readCount.fetch_add(1);
            std::this_thread::yield();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stopStress.store(true);
    writer.join();
    reader.join();

    TEST_ASSERT(readCount.load() > 100, "Multithreaded stress test completed hundreds of reads without crash or corruption");

    std::cout << "  -> Passed TestSafeStringRetrievalAndApiV5 (" << readCount.load() << " concurrent reads verified)" << std::endl;
}

void TestStringLengthsAndUtf8Boundary() {
    std::cout << "[Test] Running TestStringLengthsAndUtf8Boundary..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    std::string mod = "StringLenMod";
    store.clearAll();

    static PerfUI::API::IPerfUI_v5 hostApiV5{};
    PerfUI::API::PopulateApiV5(hostApiV5);
    PerfUI::Client::Detail::s_customRequestFn.store([](unsigned long ver, const PerfUI::API::ClientABIInfo*) -> void* {
        if (ver <= 5) return &hostApiV5;
        return nullptr;
    });

    // 1. Exact lengths: 0, 1, 255, 256, 257, 4096, 100000
    std::vector<size_t> testSizes = { 0, 1, 255, 256, 257, 4096, 100000 };
    for (size_t sz : testSizes) {
        std::string expected(sz, static_cast<char>('A' + (sz % 26)));
        std::string key = "len_" + std::to_string(sz);
        store.setString(mod, key, expected);

        // a) Client::GetSettingString
        std::string retrieved = PerfUI::Client::GetSettingString(mod.c_str(), key.c_str(), "fallback");
        TEST_ASSERT(retrieved.size() == sz, "Client::GetSettingString retrieved full size for length " + std::to_string(sz));
        TEST_ASSERT(retrieved == expected, "Client::GetSettingString retrieved matching content for length " + std::to_string(sz));

        // b) Null buffer returns required size
        size_t needed = PerfUI::Client::GetSettingStringCopy(mod.c_str(), key.c_str(), nullptr, 0, "fallback");
        TEST_ASSERT(needed == sz + 1, "GetSettingStringCopy with null buffer returns sz + 1 for length " + std::to_string(sz));
    }

    // 2. UTF-8 multi-byte truncation by character boundary
    std::string cyrillic = "Привет"; // 6 characters = 12 bytes
    store.setString(mod, "cyrillic", cyrillic);

    char buf[16];
    // Buffer size 4: max 3 bytes to copy -> cuts inside 2nd char. Must truncate to 2 bytes ("П")
    size_t copied = PerfUI::Client::GetSettingStringCopy(mod.c_str(), "cyrillic", buf, 4, "");
    TEST_ASSERT(copied == 2, "Cyrillic truncation to 4-byte buffer yielded 2 bytes");
    TEST_ASSERT(std::string(buf) == "П", "Cyrillic truncation did not break UTF-8 sequence");

    // Buffer size 5: max 4 bytes to copy -> exactly 2 characters ("Пр")
    copied = PerfUI::Client::GetSettingStringCopy(mod.c_str(), "cyrillic", buf, 5, "");
    TEST_ASSERT(copied == 4, "Cyrillic truncation to 5-byte buffer yielded 4 bytes");
    TEST_ASSERT(std::string(buf) == "Пр", "Cyrillic truncation exact 2 characters");

    // Emoji: "A😀B" ('A' 1 byte, emoji 4 bytes, 'B' 1 byte -> total 6 bytes)
    std::string emoji = "A\xF0\x9F\x98\x80" "B";
    store.setString(mod, "emoji", emoji);

    // Buffer size 4: max 3 bytes to copy -> 'A' (1 byte) + partial emoji (2 bytes) -> must truncate to 1 byte ("A")
    copied = PerfUI::Client::GetSettingStringCopy(mod.c_str(), "emoji", buf, 4, "");
    TEST_ASSERT(copied == 1, "Emoji truncation to 4-byte buffer yielded 1 byte");
    TEST_ASSERT(std::string(buf) == "A", "Emoji truncation cleanly dropped partial emoji");

    // Buffer size 6: max 5 bytes to copy -> 'A' (1 byte) + full emoji (4 bytes) = 5 bytes ("A😀")
    copied = PerfUI::Client::GetSettingStringCopy(mod.c_str(), "emoji", buf, 6, "");
    TEST_ASSERT(copied == 5, "Emoji truncation to 6-byte buffer yielded 5 bytes");
    TEST_ASSERT(std::string(buf) == "A\xF0\x9F\x98\x80", "Emoji truncation kept full emoji");

    // 3. Multithreaded stress test during active string mutation
    std::atomic<bool> stopStress{ false };
    std::atomic<int> readCount{ 0 };

    std::thread writer([&]() {
        int iter = 0;
        while (!stopStress.load()) {
            size_t len = 50 + (iter % 3000);
            std::string s(len, static_cast<char>('a' + (iter % 26)));
            s += "Привет\xF0\x9F\x98\x80";
            store.setString(mod, "raceStr", s);
            ++iter;
            std::this_thread::sleep_for(std::chrono::microseconds(30));
        }
    });

    std::thread reader([&]() {
        while (!stopStress.load()) {
            std::string s = PerfUI::Client::GetSettingString(mod.c_str(), "raceStr", "def");
            TEST_ASSERT(!s.empty(), "String read is non-empty");
            readCount.fetch_add(1);
            std::this_thread::yield();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    stopStress.store(true);
    writer.join();
    reader.join();

    TEST_ASSERT(readCount.load() > 50, "Multithreaded dynamic string stress completed concurrent reads without crash");

    // 4. Microbenchmark: 100,000 short string queries
    store.setString(mod, "shortStr", "fast_short_string");
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; ++i) {
        std::string s = PerfUI::Client::GetSettingString(mod.c_str(), "shortStr", "def");
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "  * 100,000 Client::GetSettingString short queries: " << ms << " ms ("
              << (ms / 100.0) << " ns/query)" << std::endl;

    std::cout << "  -> Passed TestStringLengthsAndUtf8Boundary" << std::endl;
}

void TestApiResolutionRetryAndIsApiReady() {
    std::cout << "[Test] Running TestApiResolutionRetryAndIsApiReady..." << std::endl;

    // 1. Initial state: DLL not loaded / returns null
    PerfUI::Client::Detail::ResetCachedApisForTesting();
    PerfUI::Client::Detail::s_customHostLoaded.store(false);
    PerfUI::Client::Detail::s_customRequestFn.store([](unsigned long, const PerfUI::API::ClientABIInfo*) -> void* {
        return nullptr;
    });

    TEST_ASSERT(!PerfUI::Client::IsApiReady(1), "IsApiReady(1) false when DLL not present");
    TEST_ASSERT(!PerfUI::Client::IsApiReady(4), "IsApiReady(4) false when DLL not present");
    TEST_ASSERT(!PerfUI::Client::IsApiReady(5), "IsApiReady(5) false when DLL not present");
    TEST_ASSERT(PerfUI::Client::GetApiV4() == nullptr, "GetApiV4 returns null before load");
    TEST_ASSERT(PerfUI::Client::GetApiV5() == nullptr, "GetApiV5 returns null before load");

    // 2. DLL loads: host becomes ready
    static PerfUI::API::IPerfUI_v5 hostApiV5{};
    PerfUI::API::PopulateApiV5(hostApiV5);

    PerfUI::Client::Detail::s_customRequestFn.store([](unsigned long ver, const PerfUI::API::ClientABIInfo*) -> void* {
        if (ver <= 5) return &hostApiV5;
        return nullptr;
    });

    // 3. Retry succeeds: failure was NOT cached permanently!
    TEST_ASSERT(PerfUI::Client::IsApiReady(1), "IsApiReady(1) true after load (retry succeeded)");
    TEST_ASSERT(PerfUI::Client::IsApiReady(4), "IsApiReady(4) true after load (retry succeeded)");
    TEST_ASSERT(PerfUI::Client::IsApiReady(5), "IsApiReady(5) true after load (retry succeeded)");

    auto* api4 = PerfUI::Client::GetApiV4();
    TEST_ASSERT(api4 != nullptr, "GetApiV4 returns non-null after host load");

    auto* api5 = PerfUI::Client::GetApiV5();
    TEST_ASSERT(api5 != nullptr, "GetApiV5 returns non-null after host load");

    // 4. Distinction between "DLL not loaded" (retry) vs "DLL loaded but version unsupported" (permanent cache)
    PerfUI::Client::Detail::ResetCachedApisForTesting();
    PerfUI::Client::Detail::s_customHostLoaded.store(true); // DLL IS loaded!
    static std::atomic<int> s_unsupportedQueryCount{ 0 };
    s_unsupportedQueryCount.store(0);
    PerfUI::Client::Detail::s_customRequestFn.store([](unsigned long ver, const PerfUI::API::ClientABIInfo*) -> void* {
        s_unsupportedQueryCount++;
        if (ver <= 3) return &hostApiV5; // Old DLL: supports only v1..v3
        return nullptr; // Refuses v4 and v5
    });

    TEST_ASSERT(PerfUI::Client::IsApiReady(1), "IsApiReady(1) supported on older DLL");
    TEST_ASSERT(!PerfUI::Client::IsApiReady(4), "IsApiReady(4) unsupported on older DLL");
    TEST_ASSERT(!PerfUI::Client::IsApiReady(5), "IsApiReady(5) unsupported on older DLL");
    TEST_ASSERT(PerfUI::Client::GetApiV4() == nullptr, "GetApiV4 returns null on older DLL");
    TEST_ASSERT(PerfUI::Client::GetApiV5() == nullptr, "GetApiV5 returns null on older DLL");

    int queriesSoFar = s_unsupportedQueryCount.load();
    // Subsequent calls to GetApiV4 / GetApiV5 must NOT re-query host function (cached rejection)
    for (int i = 0; i < 10; ++i) {
        TEST_ASSERT(PerfUI::Client::GetApiV4() == nullptr, "Cached rejection v4");
        TEST_ASSERT(PerfUI::Client::GetApiV5() == nullptr, "Cached rejection v5");
    }
    TEST_ASSERT(s_unsupportedQueryCount.load() == queriesSoFar, "No repeated host queries sent after unsupported version cached");

    // Clean up hook
    PerfUI::Client::Detail::s_customRequestFn.store(nullptr);

    std::cout << "  -> Passed TestApiResolutionRetryAndIsApiReady" << std::endl;
}

void TestBackgroundAsyncSavingAndDebounce() {
    std::cout << "[Test] Running TestBackgroundAsyncSavingAndDebounce..." << std::endl;

    auto tempDir = std::filesystem::temp_directory_path() / "PerfUI_AsyncSave_Test";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();
    store.setSettingsDirectory(tempDir);

    std::string mod = "AsyncMod";
    SettingDef def;
    def.key = "sliderVal";
    def.type = SettingType::Float;
    def.defaultValue = 0.0f;
    store.registerSetting(mod, def);

    // 1. Rapid slider changes: 500 rapid sets must execute purely in-memory
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= 500; ++i) {
        store.setFloat(mod, "sliderVal", static_cast<float>(i));
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double rapidMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    TEST_ASSERT(rapidMs < 10.0, "500 rapid setFloat calls completed under 10ms (pure in-memory)");

    // In-memory value is immediately 500.0f
    TEST_ASSERT(store.getFloat(mod, "sliderVal") == 500.0f, "In-memory value is immediately 500.0f");

    // File not written yet due to debounce
    auto filePath = store.getSettingsFilePath(mod);
    TEST_ASSERT(!std::filesystem::exists(filePath), "File not written before debounce timeout");

    // 2. update(dt) in frame loop: when debounce expires, snapshot is enqueued to background thread without blocking frame!
    auto tFrameStart = std::chrono::high_resolution_clock::now();
    store.update(0.5f); // 0.5s > 0.4s debounce
    auto tFrameEnd = std::chrono::high_resolution_clock::now();
    double frameMs = std::chrono::duration<double, std::milli>(tFrameEnd - tFrameStart).count();
    TEST_ASSERT(frameMs < 1.0, "update() frame time < 1ms (non-blocking background enqueue)");

    // 3. flushAll waits for background writer to complete atomic write to disk
    store.flushAll();
    TEST_ASSERT(std::filesystem::exists(filePath), "Settings file exists after flushAll");

    // Verify written file contains the final value (500.0f)
    SettingsStore freshStore;
    freshStore.setSettingsDirectory(tempDir);
    freshStore.registerSetting(mod, def);
    TEST_ASSERT(freshStore.getFloat(mod, "sliderVal") == 500.0f, "Disk file holds final coalesced value 500.0f");

    // Clean up
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    std::cout << "  -> Passed TestBackgroundAsyncSavingAndDebounce (frame dispatch time: " << frameMs << " ms)" << std::endl;
}

void TestSubscriptionThreadingAndReentrantCalls() {
    std::cout << "[Test] Running TestSubscriptionThreadingAndReentrantCalls..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();

    PerfUI::API::IPerfUI_v4 api{};
    PerfUI::API::PopulateApiV4(api);

    std::string mod = "ReentrantMod";

    // 1. Calling SetSetting from inside a callback (cross-key) must not deadlock
    bool keyBCalled = false;
    uint64_t subB = api.SubscribeSettingChanged(mod.c_str(), "keyB", [](void* ud, const char*, const char*) {
        auto* flag = static_cast<bool*>(ud);
        *flag = true;
    }, &keyBCalled);

    struct ContextA {
        PerfUI::API::IPerfUI_v4* api;
        std::string mod;
    } ctxA{ &api, mod };

    uint64_t subA = api.SubscribeSettingChanged(mod.c_str(), "keyA", [](void* ud, const char*, const char*) {
        auto* ctx = static_cast<ContextA*>(ud);
        // Mutate keyB from within keyA callback
        ctx->api->SetSettingFloat(ctx->mod.c_str(), "keyB", 42.0f);
    }, &ctxA);

    api.SetSettingFloat(mod.c_str(), "keyA", 1.0f);
    TEST_ASSERT(keyBCalled == true, "Mutating keyB from keyA callback succeeded without deadlock");
    TEST_ASSERT(api.GetSettingFloat(mod.c_str(), "keyB", 0.0f) == 42.0f, "keyB updated to 42.0f");

    api.UnsubscribeSettingChanged(subA);
    api.UnsubscribeSettingChanged(subB);

    // 2. Unsubscribing self from inside own callback must not crash or deadlock
    struct SelfUnsubContext {
        PerfUI::API::IPerfUI_v4* api;
        uint64_t subId{ 0 };
        int callCount{ 0 };
    } selfCtx{ &api, 0, 0 };

    selfCtx.subId = api.SubscribeSettingChanged(mod.c_str(), "selfUnsubKey", [](void* ud, const char*, const char*) {
        auto* c = static_cast<SelfUnsubContext*>(ud);
        c->callCount++;
        c->api->UnsubscribeSettingChanged(c->subId);
    }, &selfCtx);

    api.SetSettingInt(mod.c_str(), "selfUnsubKey", 1);
    TEST_ASSERT(selfCtx.callCount == 1, "Self-unsubscribing callback called first time");

    // Setting again must not fire callback
    api.SetSettingInt(mod.c_str(), "selfUnsubKey", 2);
    TEST_ASSERT(selfCtx.callCount == 1, "Self-unsubscribed callback did not fire on subsequent change");

    // 3. Callback 1 unregisters Callback 2: Callback 2 must not fire
    struct MutualContext {
        PerfUI::API::IPerfUI_v4* api;
        uint64_t sub2Id{ 0 };
        bool cb2Called{ false };
    } mutCtx{ &api, 0, false };

    uint64_t sub1 = api.SubscribeSettingChanged(mod.c_str(), "mutualKey", [](void* ud, const char*, const char*) {
        auto* c = static_cast<MutualContext*>(ud);
        c->api->UnsubscribeSettingChanged(c->sub2Id);
    }, &mutCtx);

    mutCtx.sub2Id = api.SubscribeSettingChanged(mod.c_str(), "mutualKey", [](void* ud, const char*, const char*) {
        auto* c = static_cast<MutualContext*>(ud);
        c->cb2Called = true;
    }, &mutCtx);

    api.SetSettingInt(mod.c_str(), "mutualKey", 100);
    TEST_ASSERT(mutCtx.cb2Called == false, "Callback 2 was successfully aborted after Callback 1 unsubscribed it");

    api.UnsubscribeSettingChanged(sub1);

    // 4. Verify thread of execution matches caller thread
    std::thread::id expectedThreadId;
    std::thread::id actualThreadId;
    uint64_t threadSub = api.SubscribeSettingChanged(mod.c_str(), "threadKey", [](void* ud, const char*, const char*) {
        auto* idPtr = static_cast<std::thread::id*>(ud);
        *idPtr = std::this_thread::get_id();
    }, &actualThreadId);

    std::thread workerThread([&]() {
        expectedThreadId = std::this_thread::get_id();
        api.SetSettingInt(mod.c_str(), "threadKey", 777);
    });
    workerThread.join();

    TEST_ASSERT(actualThreadId == expectedThreadId, "Callback was invoked synchronously in the calling thread");
    api.UnsubscribeSettingChanged(threadSub);

    // 5. Unregistered mod / key query safety
    TEST_ASSERT(api.GetSettingBool("NonExistentMod", "noKey", true) == true, "Unregistered mod bool fallback");
    TEST_ASSERT(api.GetSettingInt("NonExistentMod", "noKey", 123) == 123, "Unregistered mod int fallback");
    TEST_ASSERT(api.GetSettingFloat("NonExistentMod", "noKey", 3.14f) == 3.14f, "Unregistered mod float fallback");
    TEST_ASSERT(std::string(api.GetSettingString("NonExistentMod", "noKey", "xyz")) == "xyz", "Unregistered mod string fallback");

    std::cout << "  -> Passed TestSubscriptionThreadingAndReentrantCalls" << std::endl;
}

void TestUIThreadWidgetUpdateAndThreadingModel() {
    std::cout << "[Test] Running TestUIThreadWidgetUpdateAndThreadingModel..." << std::endl;

    auto& store = SettingsStore::GetSingleton();
    store.clearAll();
    std::string mod = "UIThreadMod";

    UIContext ctx;
    SettingDef def;
    def.key = "speed";
    def.type = SettingType::Float;
    def.defaultValue = 10.0f;
    def.minValue = 0.0f;
    def.maxValue = 100.0f;
    store.registerSetting(mod, def);

    SettingsSection section("Vehicle Settings", mod);
    section.setContext(&ctx);
    auto* slider = section.addSlider("Speed", "speed");
    TEST_ASSERT(slider != nullptr, "Slider created");
    TEST_ASSERT(slider->value() == 10.0f, "Slider initialized to default 10.0f");

    // 1. Mutate from background thread: update must be queued and NOT touch widget immediately
    std::thread bgWorker([&]() {
        store.setFloat(mod, "speed", 75.0f);
    });
    bgWorker.join();

    TEST_ASSERT(slider->value() == 10.0f, "Slider value was NOT mutated concurrently on background thread");

    // 2. Main thread runs update(dt): queued store update applies safely
    section.update(0.016f);
    TEST_ASSERT(slider->value() == 75.0f, "Slider value updated to 75.0f on UI thread during section.update()");

    // 3. Mutate on UI thread directly: applies immediately
    store.setFloat(mod, "speed", 33.0f);
    TEST_ASSERT(slider->value() == 33.0f, "Direct UI-thread store mutation updates widget immediately");

    // 4. Multithreaded stress test: 5 background threads mutating store while UI thread runs update
    std::atomic<bool> stopStress{ false };
    std::vector<std::thread> workers;
    for (int t = 0; t < 5; ++t) {
        workers.emplace_back([&, t]() {
            int count = 0;
            while (!stopStress.load()) {
                float v = static_cast<float>((count * 7 + t * 13) % 100);
                store.setFloat(mod, "speed", v);
                ++count;
                std::this_thread::yield();
            }
        });
    }

    for (int frame = 0; frame < 100; ++frame) {
        section.update(0.016f);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    stopStress.store(true);
    for (auto& w : workers) {
        w.join();
    }

    // Final update on UI thread drains all remaining background updates
    section.update(0.016f);
    float finalStoreVal = store.getFloat(mod, "speed", 0.0f);
    TEST_ASSERT(slider->value() == finalStoreVal, "Final slider value matches store value after multithreaded stress");

    std::cout << "  -> Passed TestUIThreadWidgetUpdateAndThreadingModel" << std::endl;
}

void TestSimpleJsonHardeningAndFuzzing() {
    std::cout << "[Test] Running TestSimpleJsonHardeningAndFuzzing..." << std::endl;

    // 1. Recursion depth limit
    {
        // 50 nested arrays -> ok
        std::string nestedOk;
        for (int i = 0; i < 50; ++i) nestedOk += "[";
        nestedOk += "1";
        for (int i = 0; i < 50; ++i) nestedOk += "]";
        std::string err;
        auto valOk = JsonValue::tryParse(nestedOk, &err);
        TEST_ASSERT(valOk.has_value(), "50 nested arrays parse within limit");

        // 70 nested arrays -> exceed depth 64
        std::string nestedExceed;
        for (int i = 0; i < 70; ++i) nestedExceed += "[";
        nestedExceed += "1";
        for (int i = 0; i < 70; ++i) nestedExceed += "]";
        auto valExceed = JsonValue::tryParse(nestedExceed, &err);
        TEST_ASSERT(!valExceed.has_value(), "70 nested arrays rejected due to recursion depth limit");
        TEST_ASSERT(err.find("recursion depth") != std::string::npos, "Error mentions recursion depth");

        // 70 nested objects -> exceed depth 64
        std::string nestedObj;
        for (int i = 0; i < 70; ++i) nestedObj += "{\"a\":";
        nestedObj += "true";
        for (int i = 0; i < 70; ++i) nestedObj += "}";
        auto valObj = JsonValue::tryParse(nestedObj, &err);
        TEST_ASSERT(!valObj.has_value(), "70 nested objects rejected due to recursion depth limit");
        TEST_ASSERT(err.find("recursion depth") != std::string::npos, "Error mentions recursion depth");
    }

    // 2. Surrogate pairs in \uXXXX
    {
        // \uD83D\uDE00 -> grinning face emoji
        std::string grinning = "\"\\uD83D\\uDE00\"";
        std::string err;
        auto val = JsonValue::tryParse(grinning, &err);
        TEST_ASSERT(val.has_value(), "Surrogate pair parsed successfully");
        TEST_ASSERT(val->asString() == "\xF0\x9F\x98\x80", "Surrogate pair converted to 4-byte UTF-8 emoji");

        // Musical note \uD834\uDD1E (U+1D11E)
        std::string note = "\"\\uD834\\uDD1E\"";
        auto valNote = JsonValue::tryParse(note, &err);
        TEST_ASSERT(valNote.has_value(), "Musical note surrogate pair parsed");
        TEST_ASSERT(valNote->asString() == "\xF0\x9D\x84\x9E", "Musical note converted to UTF-8");

        // Unpaired high surrogate
        auto valUnpairedHigh = JsonValue::tryParse("\"\\uD83D abc\"", &err);
        TEST_ASSERT(!valUnpairedHigh.has_value(), "Unpaired high surrogate rejected");
        TEST_ASSERT(err.find("surrogate") != std::string::npos, "Error mentions surrogate");

        // Unpaired low surrogate
        auto valUnpairedLow = JsonValue::tryParse("\"\\uDE00\"", &err);
        TEST_ASSERT(!valUnpairedLow.has_value(), "Unpaired low surrogate rejected");
        TEST_ASSERT(err.find("surrogate") != std::string::npos, "Error mentions surrogate");

        // High surrogate followed by non-surrogate \u0041 ('A')
        auto valWrongPair = JsonValue::tryParse("\"\\uD83D\\u0041\"", &err);
        TEST_ASSERT(!valWrongPair.has_value(), "High surrogate followed by standard character rejected");
    }

    // 3. Strict numbers, NaN, Infinity, leading zeros
    {
        std::string err;
        // Leading zeros
        TEST_ASSERT(!JsonValue::tryParse("012", &err).has_value(), "Leading zero rejected (012)");
        TEST_ASSERT(!JsonValue::tryParse("-05", &err).has_value(), "Leading zero rejected (-05)");
        TEST_ASSERT(JsonValue::tryParse("0", &err).has_value(), "Single zero accepted (0)");
        TEST_ASSERT(JsonValue::tryParse("-0", &err).has_value(), "Negative zero accepted (-0)");
        TEST_ASSERT(JsonValue::tryParse("0.123", &err).has_value(), "Float with 0 integer part accepted (0.123)");

        // NaN and Infinity
        TEST_ASSERT(!JsonValue::tryParse("NaN", &err).has_value(), "NaN rejected");
        TEST_ASSERT(!JsonValue::tryParse("Infinity", &err).has_value(), "Infinity rejected");
        TEST_ASSERT(!JsonValue::tryParse("-Infinity", &err).has_value(), "-Infinity rejected");
        TEST_ASSERT(!JsonValue::tryParse("{\"val\": NaN}", &err).has_value(), "Object with NaN rejected");
        TEST_ASSERT(!JsonValue::tryParse("[Infinity]", &err).has_value(), "Array with Infinity rejected");

        // Incomplete numbers
        TEST_ASSERT(!JsonValue::tryParse("-", &err).has_value(), "Lone minus rejected");
        TEST_ASSERT(!JsonValue::tryParse("1.", &err).has_value(), "Trailing dot rejected");
        TEST_ASSERT(!JsonValue::tryParse(".5", &err).has_value(), "Leading dot without zero rejected");
        TEST_ASSERT(!JsonValue::tryParse("1e", &err).has_value(), "Incomplete exponent rejected");
        TEST_ASSERT(!JsonValue::tryParse("1e+", &err).has_value(), "Incomplete exponent with sign rejected");
    }

    // 4. UTF-8 BOM
    {
        std::string bomJson = "\xEF\xBB\xBF{\"bomKey\": \"bomValue\"}";
        std::string err;
        auto val = JsonValue::tryParse(bomJson, &err);
        TEST_ASSERT(val.has_value(), "JSON with UTF-8 BOM parsed successfully");
        TEST_ASSERT(val->get("bomKey").asString() == "bomValue", "BOM value matches");
    }

    // 5. Duplicate keys in objects ("last key wins")
    {
        std::string dupJson = "{\"dup\": 1, \"other\": \"abc\", \"dup\": 2, \"dup\": 99}";
        std::string err;
        auto val = JsonValue::tryParse(dupJson, &err);
        TEST_ASSERT(val.has_value(), "Object with duplicate keys parsed");
        TEST_ASSERT(val->size() == 2, "Object has 2 unique keys");
        TEST_ASSERT(val->get("dup").asInt() == 99, "Last key wins for duplicate key");
        TEST_ASSERT(val->get("other").asString() == "abc", "Non-duplicate key preserved");
    }

    // 6. Precise line and column in error messages
    {
        std::string multiLine = "{\n  \"valid\": 1,\n  \"invalid\": 099\n}";
        std::string err;
        auto val = JsonValue::tryParse(multiLine, &err);
        TEST_ASSERT(!val.has_value(), "Invalid multiline JSON rejected");
        TEST_ASSERT(err.find("line 3") != std::string::npos, "Error correctly points to line 3");
    }

    // 7. Fuzzing: 2,000 mutated / random inputs
    {
        std::mt19937_64 rng(1337);
        std::vector<std::string> seedCorpus = {
            "{}",
            "[]",
            "{\"a\": 1, \"b\": [true, false, null], \"c\": \"test\"}",
            "\"simple string\"",
            "12345",
            "-987.654e-2",
            "\"\\u0041\\u0042\\u0043\"",
            "{\"nested\": {\"deep\": {\"item\": 100}}}",
            "   \n\t// comment\n  {\"val\": 1} /* block */ \n"
        };

        static const char mutationChars[] = "\0\"'\\{}[],:0123456789+-eE. \t\r\n\xEF\xBB\xBF\xD8\x3D\xDE";
        static const size_t numMutationChars = sizeof(mutationChars) - 1;

        for (int i = 0; i < 2000; ++i) {
            std::string input;
            int mode = rng() % 4;
            if (mode == 0) {
                // Mutate a corpus item
                input = seedCorpus[rng() % seedCorpus.size()];
                int mutations = 1 + (rng() % 5);
                for (int m = 0; m < mutations; ++m) {
                    if (input.empty()) {
                        char c = mutationChars[rng() % numMutationChars];
                        input.push_back(c);
                    } else if (rng() % 3 == 0) {
                        size_t pos = rng() % (input.size() + 1);
                        char c = mutationChars[rng() % numMutationChars];
                        input.insert(input.begin() + pos, c);
                    } else if (rng() % 2 == 0) {
                        size_t pos = rng() % input.size();
                        char c = mutationChars[rng() % numMutationChars];
                        input[pos] = c;
                    } else {
                        size_t pos = rng() % input.size();
                        input.erase(input.begin() + pos);
                    }
                }
            } else if (mode == 1) {
                // Random string from mutation characters
                size_t len = rng() % 120;
                for (size_t l = 0; l < len; ++l) {
                    input.push_back(mutationChars[rng() % numMutationChars]);
                }
            } else if (mode == 2) {
                // Highly nested structures
                int depth = 1 + (rng() % 100);
                for (int d = 0; d < depth; ++d) input += (rng() % 2 == 0) ? "[" : "{\"k\":";
                input += "0";
                for (int d = 0; d < depth; ++d) input += (rng() % 2 == 0) ? "]" : "}";
            } else {
                // Extreme number formats
                if (rng() % 2 == 0) input += "-";
                size_t digits = 1 + (rng() % 350);
                for (size_t d = 0; d < digits; ++d) input += static_cast<char>('0' + (rng() % 10));
                if (rng() % 2 == 0) {
                    input += ".";
                    for (size_t d = 0; d < 20; ++d) input += static_cast<char>('0' + (rng() % 10));
                }
                if (rng() % 2 == 0) {
                    input += (rng() % 2 == 0) ? "e" : "e+";
                    for (size_t d = 0; d < 10; ++d) input += static_cast<char>('0' + (rng() % 10));
                }
            }

            std::string err;
            auto parsed = JsonValue::tryParse(input, &err);
            if (!parsed.has_value()) {
                TEST_ASSERT(!err.empty(), "Error message must be set when parsing fails");
            }
        }
    }

    std::cout << "  -> Passed TestSimpleJsonHardeningAndFuzzing (2,000 fuzz iterations)" << std::endl;
}

void TestBenchmarks() {
    std::cout << "[Benchmark] Running SettingsStore Performance Benchmarks..." << std::endl;

    auto tempDir = std::filesystem::temp_directory_path() / "PerfUI_Benchmark";
    std::filesystem::create_directories(tempDir);

    auto& store = SettingsStore::GetSingleton();
    store.setSettingsDirectory(tempDir);

    // 1. Benchmark: Register 50 mods x 100 settings (5,000 settings total)
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int m = 0; m < 50; ++m) {
        std::string modId = "BenchMod_" + std::to_string(m);
        for (int s = 0; s < 100; ++s) {
            SettingDef def;
            def.key = "setting_" + std::to_string(s);
            if (s % 4 == 0) {
                def.type = SettingType::Bool;
                def.defaultValue = (s % 2 == 0);
            } else if (s % 4 == 1) {
                def.type = SettingType::Float;
                def.defaultValue = static_cast<float>(s) * 1.5f;
                def.minValue = 0.0f;
                def.maxValue = 500.0f;
            } else if (s % 4 == 2) {
                def.type = SettingType::String;
                def.defaultValue = std::string("default_str_val");
            } else {
                def.type = SettingType::KeyCode;
                def.defaultValue = static_cast<uint32_t>(s);
            }
            store.registerSetting(modId, def);
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double regMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "  * Registered 5,000 settings (50 mods x 100 settings): " << regMs << " ms ("
              << (regMs / 50.0) << " ms/mod)" << std::endl;

    // 2. Benchmark: 100,000 read queries (cache hits)
    auto t2 = std::chrono::high_resolution_clock::now();
    double accum = 0.0;
    for (int i = 0; i < 100000; ++i) {
        std::string modId = "BenchMod_" + std::to_string(i % 50);
        std::string key = "setting_" + std::to_string(i % 100);
        accum += store.getFloat(modId, key, 0.0f);
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    double readMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "  * 100,000 read queries across 5,000 settings: " << readMs << " ms ("
              << (readMs / 100.0) << " ns/query, checksum=" << accum << ")" << std::endl;

    // 3. Benchmark: Save 50 mods to disk (atomic JSON serialization & flush)
    auto t4 = std::chrono::high_resolution_clock::now();
    for (int m = 0; m < 50; ++m) {
        std::string modId = "BenchMod_" + std::to_string(m);
        store.saveToFile(modId, true);
    }
    auto t5 = std::chrono::high_resolution_clock::now();
    double saveMs = std::chrono::duration<double, std::milli>(t5 - t4).count();
    std::cout << "  * Saved 50 mod JSON files to disk: " << saveMs << " ms ("
              << (saveMs / 50.0) << " ms/mod)" << std::endl;

    // 4. Benchmark: Load 50 mods from disk
    auto t6 = std::chrono::high_resolution_clock::now();
    for (int m = 0; m < 50; ++m) {
        std::string modId = "BenchMod_" + std::to_string(m);
        store.loadFromFile(modId);
    }
    auto t7 = std::chrono::high_resolution_clock::now();
    double loadMs = std::chrono::duration<double, std::milli>(t7 - t6).count();
    std::cout << "  * Loaded 50 mod JSON files from disk: " << loadMs << " ms ("
              << (loadMs / 50.0) << " ms/mod)" << std::endl;

    // Cleanup
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    std::cout << "  -> Passed TestBenchmarks" << std::endl;
}

void TestShutdownAndSynchronousFallback() {
    std::cout << "[Test] Running TestShutdownAndSynchronousFallback..." << std::endl;

    auto& store = PerfUI::SettingsStore::GetSingleton();
    auto tempDir = std::filesystem::temp_directory_path() / "PerfUI_Shutdown_Test";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    store.clearAll();
    store.setSettingsDirectory(tempDir);

    const std::string mod = "ShutdownTestMod";
    store.registerSetting(mod, PerfUI::SettingDef{ "Alpha", PerfUI::SettingType::Int, 10 });
    store.registerSetting(mod, PerfUI::SettingDef{ "Beta", PerfUI::SettingType::String, std::string("initial") });

    // 1. Double shutdown must be safe and idempotent
    TEST_ASSERT(!store.isShutdown(), "Store is active initially");
    store.shutdown();
    TEST_ASSERT(store.isShutdown(), "Store is marked as shutdown");

    // Second call to shutdown()
    auto t0 = std::chrono::high_resolution_clock::now();
    store.shutdown();
    auto t1 = std::chrono::high_resolution_clock::now();
    double doubleShutdownMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    TEST_ASSERT(doubleShutdownMs < 50.0, "Second shutdown returns immediately (<50ms)");
    TEST_ASSERT(store.isShutdown(), "Store remains shutdown");

    // 2. Post-shutdown set operates synchronously and persists directly
    store.setInt(mod, "Alpha", 42);
    store.setString(mod, "Beta", "persisted_post_shutdown");

    // Verify file on disk exists and contains post-shutdown values immediately without update(dt)
    auto filePath = store.getSettingsFilePath(mod);
    TEST_ASSERT(std::filesystem::exists(filePath), "Settings file exists after post-shutdown set");

    std::ifstream ifs(filePath);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();
    TEST_ASSERT(content.find("\"Alpha\": 42") != std::string::npos, "Disk file contains synchronous post-shutdown int");
    TEST_ASSERT(content.find("\"Beta\": \"persisted_post_shutdown\"") != std::string::npos, "Disk file contains synchronous post-shutdown string");

    // 3. Pending saves before shutdown are flushed
    store.clearAll();
    store.setSettingsDirectory(tempDir);
    TEST_ASSERT(!store.isShutdown(), "clearAll revives store");

    const std::string mod2 = "ShutdownFlushMod";
    store.registerSetting(mod2, PerfUI::SettingDef{ "Score", PerfUI::SettingType::Int, 0 });
    store.setInt(mod2, "Score", 999); // sets dirty = true, debounceTimer = 0.4s
    // Do NOT call update(dt) or sleep 400ms! Immediately call shutdown()
    store.shutdown();

    auto file2 = store.getSettingsFilePath(mod2);
    TEST_ASSERT(std::filesystem::exists(file2), "Settings file flushed to disk on shutdown");
    std::ifstream ifs2(file2);
    std::string content2((std::istreambuf_iterator<char>(ifs2)), std::istreambuf_iterator<char>());
    ifs2.close();
    TEST_ASSERT(content2.find("\"Score\": 999") != std::string::npos, "Dirty setting was written to disk during shutdown flush");

    // 4. Rapid series of sets and immediate shutdown
    store.clearAll();
    store.setSettingsDirectory(tempDir);
    const std::string mod3 = "RapidShutdownMod";
    for (int i = 0; i < 50; ++i) {
        std::string key = "Key_" + std::to_string(i);
        store.registerSetting(mod3, PerfUI::SettingDef{ key, PerfUI::SettingType::Int, 0 });
        store.setInt(mod3, key, i * 10);
    }
    // Immediate shutdown
    auto s0 = std::chrono::high_resolution_clock::now();
    store.shutdown();
    auto s1 = std::chrono::high_resolution_clock::now();
    double shutdownTimeMs = std::chrono::duration<double, std::milli>(s1 - s0).count();
    TEST_ASSERT(shutdownTimeMs < 2000.0, "Rapid sets followed by shutdown completed within timeout");

    auto file3 = store.getSettingsFilePath(mod3);
    TEST_ASSERT(std::filesystem::exists(file3), "Rapid set file persisted on disk");
    std::ifstream ifs3(file3);
    std::string content3((std::istreambuf_iterator<char>(ifs3)), std::istreambuf_iterator<char>());
    ifs3.close();
    TEST_ASSERT(content3.find("\"Key_49\": 490") != std::string::npos, "Last rapid key persisted on disk");

    // 5. Test DLL_PROCESS_DETACH simulation (process detaching flag prevents blocking join)
    store.clearAll();
    store.setSettingsDirectory(tempDir);
    PerfUI::SettingsStore::setProcessDetaching(true);
    TEST_ASSERT(PerfUI::SettingsStore::isProcessDetaching(), "Process detaching flag set");
    store.setInt("DetachMod", "Val", 1);
    auto d0 = std::chrono::high_resolution_clock::now();
    store.shutdown();
    auto d1 = std::chrono::high_resolution_clock::now();
    double detachMs = std::chrono::duration<double, std::milli>(d1 - d0).count();
    TEST_ASSERT(detachMs < 100.0, "Shutdown during process detaching is non-blocking (<100ms)");

    // Cleanup
    PerfUI::SettingsStore::setProcessDetaching(false);
    store.clearAll();
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    std::cout << "  -> Passed TestShutdownAndSynchronousFallback" << std::endl;
}

int main() {
    std::cout << "=== Running PerfUI SettingsStore Tests ===" << std::endl;

    TestTypesAndConversions();
    TestValidationAndClamping();
    TestRegistrationAndStoreOperations();
    TestSubscriptionsAndReentrancy();
    TestPersistenceAndCorruptFile();
    TestDebounceSave();
    TestConcurrency();
    TestWidgetBindingAndTwoWaySync();
    TestDeclarativeSettingsSchema();
    TestModSettingsManagerAndApiV4();
    TestHotkeysAndConflictResolution();
    TestAutoLoaderAndSchemaMarker();
    TestSafeStringRetrievalAndApiV5();
    TestStringLengthsAndUtf8Boundary();
    TestApiResolutionRetryAndIsApiReady();
    TestBackgroundAsyncSavingAndDebounce();
    TestShutdownAndSynchronousFallback();
    TestSubscriptionThreadingAndReentrantCalls();
    TestUIThreadWidgetUpdateAndThreadingModel();
    TestSimpleJsonHardeningAndFuzzing();
    TestBenchmarks();

    std::cout << "=== ALL SETTINGS TESTS PASSED ===" << std::endl;
    return 0;
}
