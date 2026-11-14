#include "PerfUI/SettingsStore.h"
#include "PerfUI/SettingsView.h"
#include "MockRenderBackend.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <filesystem>
#include <fstream>

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

    std::cout << "=== ALL SETTINGS TESTS PASSED ===" << std::endl;
    return 0;
}
