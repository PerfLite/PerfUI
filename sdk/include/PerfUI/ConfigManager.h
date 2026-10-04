#pragma once

#include <cstdint>
#include <string>
#include "Export.h"

namespace PerfUI {

struct ConfigData {
    float windowOpacity{ 0.95f };
    std::string themePreset{ "Nordic Gold" };
    bool showProfiler{ true };
    bool pauseGameWhenOpen{ false };
    bool autoTrackNewQuests{ false };
    float windowPosX{ -1.0f };
    float windowPosY{ -1.0f };
    uint32_t toggleHotkey{ 0x7A }; // VK_F11
};

class PERFUI_API ConfigManager {
public:
    static ConfigManager& GetSingleton();

    ConfigData& config() { return m_config; }
    const ConfigData& config() const { return m_config; }

    bool loadFromFile(const std::string& filepath = "");
    bool saveToFile(const std::string& filepath = "");

    std::string getDefaultConfigPath() const;

private:
    ConfigManager();
    ~ConfigManager() = default;

    ConfigData m_config;
    std::string m_lastPath;
};

} // namespace PerfUI
