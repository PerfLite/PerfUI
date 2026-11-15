#include "PerfUI/SettingsStore.h"
#include "SimpleJson.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <cmath>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace PerfUI {

static Color ParseColorHex(std::string_view hexStr) {
    if (hexStr.empty()) return Color::White();
    if (hexStr.front() == '#') hexStr.remove_prefix(1);

    uint32_t val = 0;
    for (char c : hexStr) {
        val <<= 4;
        if (c >= '0' && c <= '9') val |= (c - '0');
        else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
    }

    if (hexStr.size() == 6) {
        uint8_t r = (val >> 16) & 0xFF;
        uint8_t g = (val >> 8) & 0xFF;
        uint8_t b = val & 0xFF;
        return Color(r, g, b, 255);
    } else if (hexStr.size() == 8) {
        uint8_t r = (val >> 24) & 0xFF;
        uint8_t g = (val >> 16) & 0xFF;
        uint8_t b = (val >> 8) & 0xFF;
        uint8_t a = val & 0xFF;
        return Color(r, g, b, a);
    }
    return Color::White();
}

static std::string FormatColorHex(const Color& c) {
    std::ostringstream ss;
    ss << "#"
       << std::hex << std::uppercase << std::setfill('0')
       << std::setw(2) << static_cast<int>(c.r)
       << std::setw(2) << static_cast<int>(c.g)
       << std::setw(2) << static_cast<int>(c.b)
       << std::setw(2) << static_cast<int>(c.a);
    return ss.str();
}

bool SettingValueToBool(const SettingValue& v, bool fallback) {
    return std::visit([fallback](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return arg != 0;
        } else if constexpr (std::is_same_v<T, float>) {
            return std::abs(arg) > 1e-5f;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return arg == "true" || arg == "1" || arg == "yes" || arg == "on";
        } else if constexpr (std::is_same_v<T, Color>) {
            return arg.a > 0;
        } else {
            return arg != 0;
        }
    }, v);
}

int32_t SettingValueToInt(const SettingValue& v, int32_t fallback) {
    return std::visit([fallback](auto&& arg) -> int32_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int32_t>) {
            return arg;
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? 1 : 0;
        } else if constexpr (std::is_same_v<T, float>) {
            return static_cast<int32_t>(std::round(arg));
        } else if constexpr (std::is_same_v<T, std::string>) {
            try { return std::stoi(arg); } catch (...) { return fallback; }
        } else if constexpr (std::is_same_v<T, Color>) {
            return static_cast<int32_t>((static_cast<uint32_t>(arg.r) << 24) | (static_cast<uint32_t>(arg.g) << 16) | (static_cast<uint32_t>(arg.b) << 8) | static_cast<uint32_t>(arg.a));
        } else {
            return static_cast<int32_t>(arg);
        }
    }, v);
}

float SettingValueToFloat(const SettingValue& v, float fallback) {
    return std::visit([fallback](auto&& arg) -> float {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, float>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return static_cast<float>(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? 1.0f : 0.0f;
        } else if constexpr (std::is_same_v<T, std::string>) {
            try { return std::stof(arg); } catch (...) { return fallback; }
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return static_cast<float>(arg);
        } else {
            return 0.0f;
        }
    }, v);
}

std::string SettingValueToString(const SettingValue& v, const std::string& fallback) {
    return std::visit([&fallback](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, float>) {
            std::ostringstream ss;
            ss << std::defaultfloat << arg;
            return ss.str();
        } else if constexpr (std::is_same_v<T, Color>) {
            return FormatColorHex(arg);
        } else {
            return std::to_string(arg);
        }
    }, v);
}

Color SettingValueToColor(const SettingValue& v, Color fallback) {
    return std::visit([fallback](auto&& arg) -> Color {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Color>) {
            return arg;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return ParseColorHex(arg);
        } else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, int32_t>) {
            uint32_t u = static_cast<uint32_t>(arg);
            return Color((u >> 24) & 0xFF, (u >> 16) & 0xFF, (u >> 8) & 0xFF, u & 0xFF);
        } else {
            return fallback;
        }
    }, v);
}

uint32_t SettingValueToKeyCode(const SettingValue& v, uint32_t fallback) {
    return std::visit([fallback](auto&& arg) -> uint32_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, uint32_t>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return static_cast<uint32_t>((std::max)(0, arg));
        } else if constexpr (std::is_same_v<T, float>) {
            return static_cast<uint32_t>((std::max)(0.0f, arg));
        } else if constexpr (std::is_same_v<T, std::string>) {
            try { return static_cast<uint32_t>(std::stoul(arg)); } catch (...) { return fallback; }
        } else {
            return fallback;
        }
    }, v);
}

bool SettingValuesEqual(const SettingValue& a, const SettingValue& b) {
    if (a.index() == b.index()) {
        return a == b;
    }
    // Cross-type comparisons
    if (std::holds_alternative<bool>(a) || std::holds_alternative<bool>(b)) {
        return SettingValueToBool(a) == SettingValueToBool(b);
    }
    if (std::holds_alternative<float>(a) || std::holds_alternative<float>(b)) {
        return std::abs(SettingValueToFloat(a) - SettingValueToFloat(b)) < 1e-5f;
    }
    if (std::holds_alternative<int32_t>(a) || std::holds_alternative<int32_t>(b) ||
        std::holds_alternative<uint32_t>(a) || std::holds_alternative<uint32_t>(b)) {
        return SettingValueToInt(a) == SettingValueToInt(b);
    }
    return SettingValueToString(a) == SettingValueToString(b);
}

SettingValue SettingDef::validateAndClamp(const SettingValue& val) const {
    switch (type) {
    case SettingType::Bool:
        return SettingValueToBool(val);

    case SettingType::Int: {
        int32_t v = SettingValueToInt(val);
        if (minValue.has_value()) {
            v = (std::max)(v, static_cast<int32_t>(*minValue));
        }
        if (maxValue.has_value()) {
            v = (std::min)(v, static_cast<int32_t>(*maxValue));
        }
        if (step.has_value() && *step > 0.0f) {
            float base = minValue.value_or(0.0f);
            float diff = static_cast<float>(v) - base;
            v = static_cast<int32_t>(base + std::round(diff / *step) * (*step));
            if (maxValue.has_value()) {
                v = (std::min)(v, static_cast<int32_t>(*maxValue));
            }
        }
        return v;
    }

    case SettingType::Float: {
        float v = SettingValueToFloat(val);
        if (minValue.has_value()) {
            v = (std::max)(v, *minValue);
        }
        if (maxValue.has_value()) {
            v = (std::min)(v, *maxValue);
        }
        if (step.has_value() && *step > 0.0f) {
            float base = minValue.value_or(0.0f);
            float diff = v - base;
            v = base + std::round(diff / *step) * (*step);
            if (maxValue.has_value()) {
                v = (std::min)(v, *maxValue);
            }
        }
        return v;
    }

    case SettingType::String: {
        std::string s = SettingValueToString(val);
        if (!options.empty()) {
            bool found = false;
            for (const auto& opt : options) {
                if (opt.id == s) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                s = SettingValueToString(defaultValue);
            }
        }
        return s;
    }

    case SettingType::Color:
        return SettingValueToColor(val);

    case SettingType::KeyCode:
        return SettingValueToKeyCode(val);
    }

    return val;
}

// ==================== SettingsStore ====================

SettingsStore& SettingsStore::GetSingleton() {
    static SettingsStore s_instance;
    return s_instance;
}

SettingsStore::SettingsStore() {
    // Default directory check
    if (std::filesystem::exists("Data/SKSE/Plugins")) {
        m_settingsDir = "Data/SKSE/Plugins/PerfUI/Settings";
    } else {
        m_settingsDir = "PerfUI_Settings";
    }
}

SettingsStore::~SettingsStore() {
    flushAll();
    m_stopWorker.store(true);
    m_saveCv.notify_all();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void SettingsStore::ensureWorkerRunning() {
    if (!m_workerStarted.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_workerStarted.load(std::memory_order_relaxed)) {
            m_stopWorker.store(false);
            m_workerThread = std::thread(&SettingsStore::workerLoop, this);
            m_workerStarted.store(true, std::memory_order_release);
        }
    }
}

void SettingsStore::workerLoop() {
    while (true) {
        SaveTask task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_saveCv.wait(lock, [this]() {
                return m_stopWorker.load() || !m_pendingSaves.empty();
            });

            if (m_stopWorker.load() && m_pendingSaves.empty()) {
                break;
            }

            auto it = m_pendingSaves.begin();
            task = std::move(it->second);
            m_pendingSaves.erase(it);
            m_activeSavesCount++;
        }

        bool success = executeDiskSave(task);
        if (!success && task.retryCount < 3) {
            task.retryCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_pendingSaves.find(task.modId) == m_pendingSaves.end()) {
                m_pendingSaves[task.modId] = std::move(task);
            }
            m_activeSavesCount--;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_activeSavesCount--;
            if (m_pendingSaves.empty() && m_activeSavesCount == 0) {
                m_flushCv.notify_all();
            }
        }
    }
}

void SettingsStore::enqueueSaveSnapshot(const std::string& modId) {
    SaveTask task;
    task.modId = modId;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_mods.find(modId);
        if (it == m_mods.end()) return;
        task.values = it->second.values;
        task.unknownRawValues = it->second.unknownRawValues;
        task.finalPath = getSettingsFilePath(modId);
        it->second.dirty = false;
        it->second.debounceTimer = -1.0f;
    }

    ensureWorkerRunning();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_pendingSaves[modId] = std::move(task);
    }
    m_saveCv.notify_one();
}

bool SettingsStore::executeDiskSave(const SaveTask& task) {
    try {
        std::error_code ec;
        std::filesystem::create_directories(task.finalPath.parent_path(), ec);

        auto tmpPath = task.finalPath;
        tmpPath.replace_extension(".json.tmp." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        JsonValue root = JsonValue::Object{};
        root["version"] = 1;
        JsonValue& settingsObj = root["settings"];
        settingsObj = JsonValue::Object{};

        for (const auto& [k, v] : task.values) {
            std::visit([&settingsObj, &k](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, bool>) settingsObj[k] = arg;
                else if constexpr (std::is_same_v<T, int32_t>) settingsObj[k] = arg;
                else if constexpr (std::is_same_v<T, float>) settingsObj[k] = arg;
                else if constexpr (std::is_same_v<T, std::string>) settingsObj[k] = arg;
                else if constexpr (std::is_same_v<T, Color>) settingsObj[k] = FormatColorHex(arg);
                else if constexpr (std::is_same_v<T, uint32_t>) settingsObj[k] = static_cast<int64_t>(arg);
            }, v);
        }

        for (const auto& [k, rawJson] : task.unknownRawValues) {
            auto parsed = JsonValue::parse(rawJson);
            settingsObj[k] = parsed;
        }

        std::string serialized = root.serialize(2);
        std::ofstream file(tmpPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[PerfUI] Failed to open temp file for writing: " << tmpPath.string() << std::endl;
            return false;
        }
        file << serialized;
        file.flush();
        file.close();

        // Atomic replace
#ifdef _WIN32
        std::wstring wTmp = tmpPath.wstring();
        std::wstring wFinal = task.finalPath.wstring();
        if (!MoveFileExW(wTmp.c_str(), wFinal.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            std::filesystem::rename(tmpPath, task.finalPath, ec);
        }
#else
        std::filesystem::rename(tmpPath, task.finalPath, ec);
#endif
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[PerfUI] Error saving settings for " << task.modId << ": " << e.what() << std::endl;
        return false;
    }
}

std::string SettingsStore::makeKeyTag(const std::string& modId, const std::string& key) const {
    return modId + "::" + key;
}

void SettingsStore::registerSetting(const std::string& modId, const SettingDef& def) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto& modData = m_mods[modId];
    modData.definitions[def.key] = def;

    // Load file if not loaded yet
    if (!modData.loaded) {
        loadFromFile(modId);
    }

    // Initialize with default if not already present
    if (modData.values.find(def.key) == modData.values.end()) {
        modData.values[def.key] = def.validateAndClamp(def.defaultValue);
    } else {
        // Clamp existing value against newly registered definition
        modData.values[def.key] = def.validateAndClamp(modData.values[def.key]);
    }
}

void SettingsStore::registerSettings(const std::string& modId, const std::vector<SettingDef>& defs) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& d : defs) {
        registerSetting(modId, d);
    }
}

bool SettingsStore::hasSetting(const std::string& modId, const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) return false;
    return it->second.definitions.find(key) != it->second.definitions.end() ||
           it->second.values.find(key) != it->second.values.end();
}

const SettingDef* SettingsStore::getDefinition(const std::string& modId, const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) return nullptr;
    auto dIt = it->second.definitions.find(key);
    if (dIt == it->second.definitions.end()) return nullptr;
    return &dIt->second;
}

std::vector<SettingDef> SettingsStore::getDefinitions(const std::string& modId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<SettingDef> res;
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        res.reserve(it->second.definitions.size());
        for (const auto& [k, def] : it->second.definitions) {
            res.push_back(def);
        }
    }
    return res;
}

std::vector<std::string> SettingsStore::getRegisteredMods() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<std::string> res;
    res.reserve(m_mods.size());
    for (const auto& [modId, _] : m_mods) {
        res.push_back(modId);
    }
    std::sort(res.begin(), res.end());
    return res;
}

SettingValue SettingsStore::get(const std::string& modId, const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        auto vIt = it->second.values.find(key);
        if (vIt != it->second.values.end()) {
            return vIt->second;
        }
        auto dIt = it->second.definitions.find(key);
        if (dIt != it->second.definitions.end()) {
            return dIt->second.defaultValue;
        }
    }
    return false;
}

bool SettingsStore::getBool(const std::string& modId, const std::string& key, bool fallback) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        auto vIt = it->second.values.find(key);
        if (vIt != it->second.values.end()) return SettingValueToBool(vIt->second, fallback);
        auto dIt = it->second.definitions.find(key);
        if (dIt != it->second.definitions.end()) return SettingValueToBool(dIt->second.defaultValue, fallback);
    }
    return fallback;
}

int32_t SettingsStore::getInt(const std::string& modId, const std::string& key, int32_t fallback) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        auto vIt = it->second.values.find(key);
        if (vIt != it->second.values.end()) return SettingValueToInt(vIt->second, fallback);
        auto dIt = it->second.definitions.find(key);
        if (dIt != it->second.definitions.end()) return SettingValueToInt(dIt->second.defaultValue, fallback);
    }
    return fallback;
}

float SettingsStore::getFloat(const std::string& modId, const std::string& key, float fallback) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        auto vIt = it->second.values.find(key);
        if (vIt != it->second.values.end()) return SettingValueToFloat(vIt->second, fallback);
        auto dIt = it->second.definitions.find(key);
        if (dIt != it->second.definitions.end()) return SettingValueToFloat(dIt->second.defaultValue, fallback);
    }
    return fallback;
}

std::string SettingsStore::getString(const std::string& modId, const std::string& key, const std::string& fallback) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        auto vIt = it->second.values.find(key);
        if (vIt != it->second.values.end()) return SettingValueToString(vIt->second, fallback);
        auto dIt = it->second.definitions.find(key);
        if (dIt != it->second.definitions.end()) return SettingValueToString(dIt->second.defaultValue, fallback);
    }
    return fallback;
}

Color SettingsStore::getColor(const std::string& modId, const std::string& key, Color fallback) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        auto vIt = it->second.values.find(key);
        if (vIt != it->second.values.end()) return SettingValueToColor(vIt->second, fallback);
        auto dIt = it->second.definitions.find(key);
        if (dIt != it->second.definitions.end()) return SettingValueToColor(dIt->second.defaultValue, fallback);
    }
    return fallback;
}

uint32_t SettingsStore::getKeyCode(const std::string& modId, const std::string& key, uint32_t fallback) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        auto vIt = it->second.values.find(key);
        if (vIt != it->second.values.end()) return SettingValueToKeyCode(vIt->second, fallback);
        auto dIt = it->second.definitions.find(key);
        if (dIt != it->second.definitions.end()) return SettingValueToKeyCode(dIt->second.defaultValue, fallback);
    }
    return fallback;
}

bool SettingsStore::set(const std::string& modId, const std::string& key, const SettingValue& val, bool notify) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto& modData = m_mods[modId];

    SettingValue clampedVal = val;
    auto dIt = modData.definitions.find(key);
    if (dIt != modData.definitions.end()) {
        clampedVal = dIt->second.validateAndClamp(val);
    }

    auto vIt = modData.values.find(key);
    if (vIt != modData.values.end() && SettingValuesEqual(vIt->second, clampedVal)) {
        return false; // No change
    }

    modData.values[key] = clampedVal;
    modData.dirty = true;
    modData.debounceTimer = 0.4f; // 400ms debounce

    if (notify) {
        notifyListeners(modId, key, clampedVal);
    }
    return true;
}

bool SettingsStore::setBool(const std::string& modId, const std::string& key, bool val, bool notify) {
    return set(modId, key, val, notify);
}

bool SettingsStore::setInt(const std::string& modId, const std::string& key, int32_t val, bool notify) {
    return set(modId, key, val, notify);
}

bool SettingsStore::setFloat(const std::string& modId, const std::string& key, float val, bool notify) {
    return set(modId, key, val, notify);
}

bool SettingsStore::setString(const std::string& modId, const std::string& key, const std::string& val, bool notify) {
    return set(modId, key, val, notify);
}

bool SettingsStore::setColor(const std::string& modId, const std::string& key, Color val, bool notify) {
    return set(modId, key, val, notify);
}

bool SettingsStore::setKeyCode(const std::string& modId, const std::string& key, uint32_t val, bool notify) {
    return set(modId, key, val, notify);
}

void SettingsStore::reset(const std::string& modId, const std::string& key) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) return;

    auto dIt = it->second.definitions.find(key);
    if (dIt != it->second.definitions.end()) {
        set(modId, key, dIt->second.defaultValue, true);
    }
}

void SettingsStore::resetAll(const std::string& modId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) return;

    for (const auto& [k, def] : it->second.definitions) {
        set(modId, k, def.defaultValue, true);
    }
}

bool SettingsStore::isDefault(const std::string& modId, const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) return true;

    auto dIt = it->second.definitions.find(key);
    if (dIt == it->second.definitions.end()) return true;

    auto vIt = it->second.values.find(key);
    if (vIt == it->second.values.end()) return true;

    return SettingValuesEqual(vIt->second, dIt->second.defaultValue);
}

std::unordered_map<std::string, SettingValue> SettingsStore::getSnapshot(const std::string& modId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_mods.find(modId);
    if (it != m_mods.end()) {
        return it->second.values;
    }
    return {};
}

uint64_t SettingsStore::onChanged(const std::string& modId, const std::string& key, ChangeCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    uint64_t id = m_nextSubscriptionId++;
    m_subscriptions[id] = Subscription{ id, modId, key, std::move(callback) };
    return id;
}

uint64_t SettingsStore::onModChanged(const std::string& modId, ChangeCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    uint64_t id = m_nextSubscriptionId++;
    m_subscriptions[id] = Subscription{ id, modId, "", std::move(callback) };
    return id;
}

void SettingsStore::unsubscribe(uint64_t subscriptionId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_subscriptions.erase(subscriptionId);
}

void SettingsStore::notifyListeners(const std::string& modId, const std::string& key, const SettingValue& val) {
    std::string tag = makeKeyTag(modId, key);
    if (m_notifyingKeys[tag]) {
        return; // Prevent re-entrant recursive loop on the exact same key
    }
    m_notifyingKeys[tag] = true;

    // Snapshot callbacks matching this key/mod to allow safe unsubscribe/subscribe during execution
    struct SubInvocation {
        uint64_t id;
        ChangeCallback cb;
    };
    std::vector<SubInvocation> callbacksToCall;
    for (const auto& [id, sub] : m_subscriptions) {
        if (sub.modId == modId && (sub.key.empty() || sub.key == key)) {
            callbacksToCall.push_back({ id, sub.callback });
        }
    }

    for (const auto& item : callbacksToCall) {
        bool stillValid = false;
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            stillValid = (m_subscriptions.find(item.id) != m_subscriptions.end());
        }
        if (stillValid && item.cb) {
            try {
                item.cb(modId, key, val);
            } catch (...) {
                // Protect against external callback exceptions
            }
        }
    }

    m_notifyingKeys[tag] = false;
}

void SettingsStore::setSettingsDirectory(const std::filesystem::path& dir) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_settingsDir = dir;
}

std::filesystem::path SettingsStore::getSettingsDirectory() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_settingsDir;
}

std::filesystem::path SettingsStore::getSettingsFilePath(const std::string& modId) const {
    return m_settingsDir / (modId + ".json");
}

bool SettingsStore::loadFromFile(const std::string& modId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto& modData = m_mods[modId];
    modData.loaded = true;

    auto path = getSettingsFilePath(modId);
    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    file.close();
    std::string content = ss.str();

    std::string parseError;
    auto root = JsonValue::parse(content, &parseError);

    if (!parseError.empty() || !root.isObject()) {
        std::cerr << "[PerfUI] Corrupt settings file: " << path.string() << " (" << parseError << "). Backing up to .bak" << std::endl;
        try {
            auto bakPath = path;
            bakPath.replace_extension(".json.bak");
            std::filesystem::copy_file(path, bakPath, std::filesystem::copy_options::overwrite_existing);
        } catch (...) {}
        return false;
    }

    // Check versioned vs flat structure
    const auto& settingsObj = root.has("settings") ? root["settings"] : root;
    if (!settingsObj.isObject()) {
        return false;
    }

    for (const auto& [k, v] : settingsObj.object()) {
        auto dIt = modData.definitions.find(k);
        if (dIt != modData.definitions.end()) {
            SettingValue loadedVal;
            switch (dIt->second.type) {
            case SettingType::Bool: loadedVal = v.asBool(); break;
            case SettingType::Int: loadedVal = static_cast<int32_t>(v.asInt()); break;
            case SettingType::Float: loadedVal = static_cast<float>(v.asFloat()); break;
            case SettingType::String: loadedVal = v.asString(); break;
            case SettingType::Color: loadedVal = ParseColorHex(v.asString()); break;
            case SettingType::KeyCode: loadedVal = static_cast<uint32_t>(v.asInt()); break;
            }
            modData.values[k] = dIt->second.validateAndClamp(loadedVal);
        } else {
            // Unknown key: preserve
            if (v.isBool()) modData.values[k] = v.asBool();
            else if (v.isInt()) modData.values[k] = static_cast<int32_t>(v.asInt());
            else if (v.isFloat()) modData.values[k] = static_cast<float>(v.asFloat());
            else if (v.isString()) modData.values[k] = v.asString();
            else modData.unknownRawValues[k] = v.serialize(0);
        }
    }

    modData.dirty = false;
    modData.debounceTimer = -1.0f;
    return true;
}

bool SettingsStore::saveToFile(const std::string& modId, bool immediate) {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_mods.find(modId);
        if (it == m_mods.end()) return false;
        if (!immediate) {
            it->second.dirty = true;
            it->second.debounceTimer = 0.4f;
            return true;
        }
    }

    enqueueSaveSnapshot(modId);
    if (immediate) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_flushCv.wait(lock, [this, &modId]() {
            return m_pendingSaves.find(modId) == m_pendingSaves.end() && m_activeSavesCount == 0;
        });
    }
    return true;
}

void SettingsStore::flushAll() {
    std::vector<std::string> dirtyMods;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        for (const auto& [modId, modData] : m_mods) {
            if (modData.dirty) {
                dirtyMods.push_back(modId);
            }
        }
    }
    for (const auto& modId : dirtyMods) {
        enqueueSaveSnapshot(modId);
    }

    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_flushCv.wait(lock, [this]() {
        return m_pendingSaves.empty() && m_activeSavesCount == 0;
    });
}

void SettingsStore::update(float dt) {
    std::vector<std::string> modsToSave;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        for (auto& [modId, modData] : m_mods) {
            if (modData.dirty && modData.debounceTimer > 0.0f) {
                modData.debounceTimer -= dt;
                if (modData.debounceTimer <= 0.0f) {
                    modsToSave.push_back(modId);
                }
            }
        }
    }
    for (const auto& modId : modsToSave) {
        enqueueSaveSnapshot(modId);
    }
}

void SettingsStore::clearAll() {
    flushAll();
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_mods.clear();
    m_subscriptions.clear();
    m_notifyingKeys.clear();
}

} // namespace PerfUI
