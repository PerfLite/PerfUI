#include "PerfUI/ConfigManager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace PerfUI {

ConfigManager& ConfigManager::GetSingleton() {
    static ConfigManager s_instance;
    return s_instance;
}

ConfigManager::ConfigManager() {
    m_lastPath = getDefaultConfigPath();
}

std::string ConfigManager::getDefaultConfigPath() const {
    if (std::filesystem::exists("Data/SKSE/Plugins")) {
        return "Data/SKSE/Plugins/PerfUI.json";
    }
    return "PerfUI_Config.json";
}

bool ConfigManager::saveToFile(const std::string& filepath) {
    std::string path = filepath.empty() ? m_lastPath : filepath;
    if (path.empty()) {
        path = getDefaultConfigPath();
    }
    m_lastPath = path;

    try {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }

        file << "{\n";
        file << "  \"windowOpacity\": " << m_config.windowOpacity << ",\n";
        file << "  \"themePreset\": \"" << m_config.themePreset << "\",\n";
        file << "  \"showProfiler\": " << (m_config.showProfiler ? "true" : "false") << ",\n";
        file << "  \"pauseGameWhenOpen\": " << (m_config.pauseGameWhenOpen ? "true" : "false") << ",\n";
        file << "  \"autoTrackNewQuests\": " << (m_config.autoTrackNewQuests ? "true" : "false") << ",\n";
        file << "  \"windowPosX\": " << m_config.windowPosX << ",\n";
        file << "  \"windowPosY\": " << m_config.windowPosY << ",\n";
        file << "  \"toggleHotkey\": " << m_config.toggleHotkey << "\n";
        file << "}\n";

        return true;
    } catch (...) {
        return false;
    }
}

static bool ExtractBool(const std::string& content, const std::string& key, bool defaultVal) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return defaultVal;

    size_t truePos = content.find("true", colon);
    size_t falsePos = content.find("false", colon);
    size_t endLine = content.find_first_of(",\n\r}", colon);

    if (truePos != std::string::npos && (endLine == std::string::npos || truePos < endLine)) {
        return true;
    }
    if (falsePos != std::string::npos && (endLine == std::string::npos || falsePos < endLine)) {
        return false;
    }
    return defaultVal;
}

static float ExtractFloat(const std::string& content, const std::string& key, float defaultVal) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return defaultVal;

    size_t start = content.find_first_of("0123456789.-", colon);
    if (start == std::string::npos) return defaultVal;

    size_t end = content.find_first_of(", \t\r\n}", start);
    std::string valStr = content.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
    try {
        return std::stof(valStr);
    } catch (...) {
        return defaultVal;
    }
}

static std::string ExtractString(const std::string& content, const std::string& key, const std::string& defaultVal) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return defaultVal;

    size_t quote1 = content.find('\"', colon);
    if (quote1 == std::string::npos) return defaultVal;

    size_t quote2 = content.find('\"', quote1 + 1);
    if (quote2 == std::string::npos) return defaultVal;

    return content.substr(quote1 + 1, quote2 - quote1 - 1);
}

static uint32_t ExtractUInt(const std::string& content, const std::string& key, uint32_t defaultVal) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;

    size_t colon = content.find(':', pos);
    if (colon == std::string::npos) return defaultVal;

    size_t start = content.find_first_of("0123456789", colon);
    if (start == std::string::npos) return defaultVal;

    size_t end = content.find_first_of(", \t\r\n}", start);
    std::string valStr = content.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
    try {
        return static_cast<uint32_t>(std::stoul(valStr));
    } catch (...) {
        return defaultVal;
    }
}

bool ConfigManager::loadFromFile(const std::string& filepath) {
    std::string path = filepath.empty() ? m_lastPath : filepath;
    if (path.empty()) {
        path = getDefaultConfigPath();
    }
    m_lastPath = path;

    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    m_config.windowOpacity = ExtractFloat(content, "windowOpacity", m_config.windowOpacity);
    m_config.themePreset = ExtractString(content, "themePreset", m_config.themePreset);
    m_config.showProfiler = ExtractBool(content, "showProfiler", m_config.showProfiler);
    m_config.pauseGameWhenOpen = ExtractBool(content, "pauseGameWhenOpen", m_config.pauseGameWhenOpen);
    m_config.autoTrackNewQuests = ExtractBool(content, "autoTrackNewQuests", m_config.autoTrackNewQuests);
    m_config.windowPosX = ExtractFloat(content, "windowPosX", m_config.windowPosX);
    m_config.windowPosY = ExtractFloat(content, "windowPosY", m_config.windowPosY);
    m_config.toggleHotkey = ExtractUInt(content, "toggleHotkey", m_config.toggleHotkey);

    return true;
}

} // namespace PerfUI
