#pragma once

#include "Export.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <filesystem>

namespace PerfUI {

class PERFUI_API LocalizationService {
public:
    static LocalizationService& instance();

    void setLanguage(std::string lang);
    const std::string& currentLanguage() const;

    // Load translations from file (.json, .txt, or .ini format)
    bool loadDictionary(const std::string& lang, const std::filesystem::path& filePath);

    // Load from memory content (supports JSON format {"key": "val"} and Key-Value text "KEY=Val" or "$KEY\tVal")
    bool loadDictionaryContent(const std::string& lang, std::string_view content);

    // Register single translation
    void setTranslation(const std::string& lang, std::string key, std::string value);

    // Translate string key. If not found in currentLanguage, tries default "en", then returns fallback or raw key.
    std::string translate(std::string_view key, std::string_view fallback = {}) const;

    // Helper that expands "$KEY" in text if it matches, otherwise returns input as-is
    std::string expandText(std::string_view text) const;

    // Clear translations for specified language (or all if empty)
    void clear(const std::string& lang = {});

private:
    LocalizationService();
    ~LocalizationService() = default;

    std::string m_currentLanguage{ "en" };
    // [lang][key] -> value
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_dictionaries;
    mutable std::mutex m_mutex;
};

} // namespace PerfUI
