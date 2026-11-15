#pragma once

#include "Export.h"
#include "SettingsStore.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace PerfUI {

class SettingsWindow;
class SettingsView;
class SettingsSection;

namespace API {
struct SettingsPageBuilder;
struct IPerfUI_v4;
PERFUI_API void PopulateApiV4(IPerfUI_v4& api);
}

struct ModSettingsEntry {
    std::string modId;
    std::string displayName;
    std::string jsonPath;
    void (*pageBuilder)(void* userData, API::SettingsPageBuilder* builder){ nullptr };
    void* builderUserData{ nullptr };
    bool isCustomBuilder{ false };
};

class PERFUI_API ModSettingsManager {
public:
    static ModSettingsManager& GetSingleton();

    bool registerJson(const std::string& modId, const std::string& displayName, const std::string& jsonPath);
    bool registerPage(const std::string& modId, const std::string& displayName, void (*build)(void*, API::SettingsPageBuilder*), void* userData);

    // Auto-discover and register mods containing perfui.settings.json
    size_t scanAndRegisterSettings(const std::filesystem::path& pluginsRoot);

    const std::vector<ModSettingsEntry>& getRegisteredMods() const { return m_mods; }
    const ModSettingsEntry* getMod(const std::string& modId) const;

    // Attach registered mods to a SettingsWindow
    void attachToWindow(SettingsWindow* window);

    // Window visibility & management
    void openSettings(const std::string& targetModId = "");
    void closeSettings();
    bool isSettingsOpen() const;

    void setSettingsWindow(SettingsWindow* win);
    SettingsWindow* getSettingsWindow() const { return m_window; }

    uint32_t getHotkey() const { return m_hotkey; }
    void setHotkey(uint32_t key) { m_hotkey = key; }

    // Conflict detection & unbinding (Phase 5)
    struct KeyConflict {
        std::string modId;
        std::string key;
    };
    std::vector<KeyConflict> findKeybindConflicts(const std::string& currentModId, const std::string& currentKey, uint32_t keyCode) const;
    void unbindConflictingKeybinds(const std::string& currentModId, const std::string& currentKey, uint32_t keyCode);

    void clear();

private:
    ModSettingsManager() = default;
    ~ModSettingsManager() = default;

    std::vector<ModSettingsEntry> m_mods;
    std::unordered_map<std::string, size_t> m_modIndex;
    SettingsWindow* m_window{ nullptr };
    uint32_t m_hotkey{ 0x58 }; // Default F11 (DX scancode 0x58 = 88)
};

} // namespace PerfUI
