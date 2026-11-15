#pragma once

#include "PerfUI/Types.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <filesystem>
#include <cstdint>

namespace PerfUI {

enum class SettingType : uint8_t {
    Bool,
    Int,
    Float,
    String,
    Color,
    KeyCode
};

using SettingValue = std::variant<bool, int32_t, float, std::string, Color, uint32_t>;

PERFUI_API bool SettingValueToBool(const SettingValue& v, bool fallback = false);
PERFUI_API int32_t SettingValueToInt(const SettingValue& v, int32_t fallback = 0);
PERFUI_API float SettingValueToFloat(const SettingValue& v, float fallback = 0.0f);
PERFUI_API std::string SettingValueToString(const SettingValue& v, const std::string& fallback = "");
PERFUI_API Color SettingValueToColor(const SettingValue& v, Color fallback = Color::White());
PERFUI_API uint32_t SettingValueToKeyCode(const SettingValue& v, uint32_t fallback = 0);
PERFUI_API bool SettingValuesEqual(const SettingValue& a, const SettingValue& b);

struct ChoiceOption {
    std::string id;
    std::string label;
};

struct SettingDef {
    std::string key;
    SettingType type{ SettingType::Bool };
    SettingValue defaultValue{ false };

    // Range & Step constraints (for numeric types)
    std::optional<float> minValue;
    std::optional<float> maxValue;
    std::optional<float> step;

    // Formatting & metadata
    std::string formatString; // e.g. "%.1f", "{0} %"
    std::vector<ChoiceOption> options; // for enum / choice
    std::string displayName;
    std::string tooltip;

    PERFUI_API SettingValue validateAndClamp(const SettingValue& val) const;
};

class PERFUI_API SettingsStore {
public:
    using ChangeCallback = std::function<void(const std::string& modId, const std::string& key, const SettingValue& val)>;

    static SettingsStore& GetSingleton();

    SettingsStore();
    ~SettingsStore();

    // Prevent copies
    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;

    // Registration of setting definitions
    void registerSetting(const std::string& modId, const SettingDef& def);
    void registerSettings(const std::string& modId, const std::vector<SettingDef>& defs);
    bool hasSetting(const std::string& modId, const std::string& key) const;
    const SettingDef* getDefinition(const std::string& modId, const std::string& key) const;
    std::vector<SettingDef> getDefinitions(const std::string& modId) const;
    std::vector<std::string> getRegisteredMods() const;

    // Value accessors
    SettingValue get(const std::string& modId, const std::string& key) const;
    bool getBool(const std::string& modId, const std::string& key, bool fallback = false) const;
    int32_t getInt(const std::string& modId, const std::string& key, int32_t fallback = 0) const;
    float getFloat(const std::string& modId, const std::string& key, float fallback = 0.0f) const;
    std::string getString(const std::string& modId, const std::string& key, const std::string& fallback = "") const;
    Color getColor(const std::string& modId, const std::string& key, Color fallback = Color::White()) const;
    uint32_t getKeyCode(const std::string& modId, const std::string& key, uint32_t fallback = 0) const;

    // Value mutators
    bool set(const std::string& modId, const std::string& key, const SettingValue& val, bool notify = true);
    bool setBool(const std::string& modId, const std::string& key, bool val, bool notify = true);
    bool setInt(const std::string& modId, const std::string& key, int32_t val, bool notify = true);
    bool setFloat(const std::string& modId, const std::string& key, float val, bool notify = true);
    bool setString(const std::string& modId, const std::string& key, const std::string& val, bool notify = true);
    bool setColor(const std::string& modId, const std::string& key, Color val, bool notify = true);
    bool setKeyCode(const std::string& modId, const std::string& key, uint32_t val, bool notify = true);

    // Reset operations
    void reset(const std::string& modId, const std::string& key);
    void resetAll(const std::string& modId);
    bool isDefault(const std::string& modId, const std::string& key) const;

    // Snapshot of current state for a mod
    std::unordered_map<std::string, SettingValue> getSnapshot(const std::string& modId) const;

    // Subscriptions
    uint64_t onChanged(const std::string& modId, const std::string& key, ChangeCallback callback);
    uint64_t onModChanged(const std::string& modId, ChangeCallback callback);
    uint64_t subscribe(const std::string& modId, const std::string& key, ChangeCallback callback) {
        return onChanged(modId, key, std::move(callback));
    }
    void unsubscribe(uint64_t subscriptionId);

    // Persistence configuration
    void setSettingsDirectory(const std::filesystem::path& dir);
    std::filesystem::path getSettingsDirectory() const;
    std::filesystem::path getSettingsFilePath(const std::string& modId) const;

    // File IO
    bool loadFromFile(const std::string& modId);
    bool saveToFile(const std::string& modId, bool immediate = false);
    void flushAll();
    void update(float dt);

    // Test / debug utilities
    void clearAll();

private:
    struct ModData {
        std::unordered_map<std::string, SettingDef> definitions;
        std::unordered_map<std::string, SettingValue> values;
        std::unordered_map<std::string, std::string> unknownRawValues; // preserves unknown keys from JSON
        float debounceTimer{ -1.0f };
        bool dirty{ false };
        bool loaded{ false };
    };

    struct Subscription {
        uint64_t id{ 0 };
        std::string modId;
        std::string key; // empty if mod-level
        ChangeCallback callback;
    };

    mutable std::recursive_mutex m_mutex;
    std::filesystem::path m_settingsDir{ "Data/SKSE/Plugins/PerfUI/Settings" };
    std::unordered_map<std::string, ModData> m_mods;
    std::unordered_map<uint64_t, Subscription> m_subscriptions;
    uint64_t m_nextSubscriptionId{ 1 };

    // Re-entrancy guard set: key format "modId::key"
    std::unordered_map<std::string, bool> m_notifyingKeys;

    void notifyListeners(const std::string& modId, const std::string& key, const SettingValue& val);
    std::string makeKeyTag(const std::string& modId, const std::string& key) const;

    // Background asynchronous writer thread (Phase 4 fix)
    struct SaveTask {
        std::string modId;
        std::filesystem::path finalPath;
        std::unordered_map<std::string, SettingValue> values;
        std::unordered_map<std::string, std::string> unknownRawValues;
        int retryCount{ 0 };
    };

    mutable std::mutex m_queueMutex;
    std::condition_variable m_saveCv;
    std::condition_variable m_flushCv;
    std::unordered_map<std::string, SaveTask> m_pendingSaves;
    size_t m_activeSavesCount{ 0 };
    std::thread m_workerThread;
    std::atomic<bool> m_stopWorker{ false };
    std::atomic<bool> m_workerStarted{ false };

    void ensureWorkerRunning();
    void enqueueSaveSnapshot(const std::string& modId);
    static bool executeDiskSave(const SaveTask& task);
    void workerLoop();
};

} // namespace PerfUI
