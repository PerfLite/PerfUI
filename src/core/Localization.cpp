#include "PerfUI/Localization.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace PerfUI {

namespace {

std::string ToLower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

std::string Trim(std::string_view s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return std::string(s.substr(start, end - start));
}

std::string UnescapeString(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[++i];
            switch (next) {
            case 'n': res.push_back('\n'); break;
            case 'r': res.push_back('\r'); break;
            case 't': res.push_back('\t'); break;
            case '\"': res.push_back('\"'); break;
            case '\\': res.push_back('\\'); break;
            default:
                res.push_back('\\');
                res.push_back(next);
                break;
            }
        } else {
            res.push_back(s[i]);
        }
    }
    return res;
}

} // namespace

LocalizationService& LocalizationService::instance() {
    static LocalizationService s_instance;
    return s_instance;
}

LocalizationService::LocalizationService() {
    m_currentLanguage = detectSystemLanguage();
}

std::string LocalizationService::detectSystemLanguage() const {
#ifdef _WIN32
    LANGID langId = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(langId);
    switch (primary) {
    case LANG_RUSSIAN:    return "ru";
    case LANG_FRENCH:     return "fr";
    case LANG_GERMAN:     return "de";
    case LANG_SPANISH:    return "es";
    case LANG_POLISH:     return "pl";
    case LANG_ITALIAN:    return "it";
    case LANG_JAPANESE:   return "ja";
    case LANG_CHINESE:    return "zh";
    case LANG_UKRAINIAN:  return "ru";
    default:              return "en";
    }
#else
    return "en";
#endif
}

void LocalizationService::setLanguage(std::string lang) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentLanguage = ToLower(lang);
}

const std::string& LocalizationService::currentLanguage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentLanguage;
}

void LocalizationService::clear(const std::string& lang) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (lang.empty()) {
        m_dictionaries.clear();
    } else {
        m_dictionaries.erase(ToLower(lang));
    }
}

void LocalizationService::setTranslation(const std::string& lang, std::string key, std::string value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (key.starts_with('$')) {
        key = key.substr(1);
    }
    m_dictionaries[ToLower(lang)][std::move(key)] = std::move(value);
}

bool LocalizationService::loadDictionary(const std::string& lang, const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return loadDictionaryContent(lang, ss.str());
}

bool LocalizationService::loadDictionaryContent(const std::string& lang, std::string_view content) {
    std::string normLang = ToLower(lang);
    if (content.empty()) return true;

    // Detect and decode UTF-16 LE with BOM (\xFF\xFE)
    std::string utf8Decoded;
    if (content.size() >= 2 &&
        static_cast<uint8_t>(content[0]) == 0xFF &&
        static_cast<uint8_t>(content[1]) == 0xFE) {
        const wchar_t* widePtr = reinterpret_cast<const wchar_t*>(content.data() + 2);
        int wideChars = static_cast<int>((content.size() - 2) / sizeof(wchar_t));
#ifdef _WIN32
        int needed = WideCharToMultiByte(CP_UTF8, 0, widePtr, wideChars, nullptr, 0, nullptr, nullptr);
        if (needed > 0) {
            utf8Decoded.resize(needed);
            WideCharToMultiByte(CP_UTF8, 0, widePtr, wideChars, utf8Decoded.data(), needed, nullptr, nullptr);
            content = utf8Decoded;
        }
#endif
    } else if (content.size() >= 3 &&
               static_cast<uint8_t>(content[0]) == 0xEF &&
               static_cast<uint8_t>(content[1]) == 0xBB &&
               static_cast<uint8_t>(content[2]) == 0xBF) {
        // Strip UTF-8 BOM
        content.remove_prefix(3);
    }

    std::string trimmed = Trim(content);
    if (trimmed.empty()) return true;

    std::lock_guard<std::mutex> lock(m_mutex);
    auto& dict = m_dictionaries[normLang];

    // 1. JSON Dictionary parsing (e.g. { "KEY": "Value" })
    if (trimmed.front() == '{') {
        size_t pos = 0;
        const size_t len = trimmed.size();
        while (pos < len) {
            // Find key start quote
            size_t keyStartQuote = trimmed.find('\"', pos);
            if (keyStartQuote == std::string_view::npos) break;

            // Find key end quote
            size_t keyEndQuote = keyStartQuote + 1;
            while (keyEndQuote < len && (trimmed[keyEndQuote] != '\"' || trimmed[keyEndQuote - 1] == '\\')) {
                ++keyEndQuote;
            }
            if (keyEndQuote >= len) break;

            std::string rawKey = UnescapeString(trimmed.substr(keyStartQuote + 1, keyEndQuote - keyStartQuote - 1));

            // Find colon separator
            size_t colon = trimmed.find(':', keyEndQuote + 1);
            if (colon == std::string_view::npos) break;

            // Find value start quote
            size_t valStartQuote = trimmed.find('\"', colon + 1);
            if (valStartQuote == std::string_view::npos) break;

            // Find value end quote
            size_t valEndQuote = valStartQuote + 1;
            while (valEndQuote < len && (trimmed[valEndQuote] != '\"' || trimmed[valEndQuote - 1] == '\\')) {
                ++valEndQuote;
            }
            if (valEndQuote >= len) break;

            std::string rawVal = UnescapeString(trimmed.substr(valStartQuote + 1, valEndQuote - valStartQuote - 1));

            if (rawKey.starts_with('$')) {
                rawKey = rawKey.substr(1);
            }
            dict[std::move(rawKey)] = std::move(rawVal);

            pos = valEndQuote + 1;
        }
        return true;
    }

    // 2. Line-by-line format:
    //    SKSE format: $KEY\tTranslation
    //    INI format:  KEY = Translation
    std::istringstream stream(trimmed);
    std::string line;
    while (std::getline(stream, line)) {
        std::string s = Trim(line);
        if (s.empty() || s.front() == ';' || s.front() == '#') {
            continue; // Skip comments and empty lines
        }

        std::string key;
        std::string val;

        size_t tabPos = s.find('\t');
        if (tabPos != std::string::npos) {
            key = Trim(s.substr(0, tabPos));
            val = Trim(s.substr(tabPos + 1));
        } else {
            size_t eqPos = s.find('=');
            if (eqPos != std::string::npos) {
                key = Trim(s.substr(0, eqPos));
                val = Trim(s.substr(eqPos + 1));
            } else {
                continue;
            }
        }

        // Unquote if wrapped in quotes
        if (val.size() >= 2 && val.front() == '\"' && val.back() == '\"') {
            val = UnescapeString(val.substr(1, val.size() - 2));
        }
        if (key.starts_with('$')) {
            key = key.substr(1);
        }

        if (!key.empty()) {
            dict[std::move(key)] = std::move(val);
        }
    }

    return true;
}

std::string LocalizationService::translate(std::string_view key, std::string_view fallback) const {
    std::string lookupKey(key);
    if (lookupKey.starts_with('$')) {
        lookupKey = lookupKey.substr(1);
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Check in current language
    auto itLang = m_dictionaries.find(m_currentLanguage);
    if (itLang != m_dictionaries.end()) {
        auto itKey = itLang->second.find(lookupKey);
        if (itKey != itLang->second.end()) {
            return itKey->second;
        }
    }

    // 2. Fallback to English if current language is different
    if (m_currentLanguage != "en") {
        auto itEn = m_dictionaries.find("en");
        if (itEn != m_dictionaries.end()) {
            auto itKey = itEn->second.find(lookupKey);
            if (itKey != itEn->second.end()) {
                return itKey->second;
            }
        }
    }

    // 3. Fallback string if provided, otherwise return key without '$'
    if (!fallback.empty()) {
        return std::string(fallback);
    }
    return lookupKey;
}

std::string LocalizationService::expandText(std::string_view text) const {
    if (!text.starts_with('$') || text.find(' ') != std::string_view::npos) {
        return std::string(text);
    }

    std::string_view key = text.substr(1);
    std::string trans = translate(key);
    if (trans != key) {
        return trans;
    }
    return std::string(text);
}

void LocalizationService::loadTranslationsFromFolder(const std::filesystem::path& folder) {
    std::error_code ec;
    if (!std::filesystem::exists(folder, ec) || !std::filesystem::is_directory(folder, ec)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (!entry.is_regular_file(ec)) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext != ".json" && ext != ".txt" && ext != ".ini") continue;

        std::string stem = entry.path().stem().string();
        std::string lowerStem = ToLower(stem);

        std::string targetLang = "en";

        if (lowerStem == "ru" || lowerStem == "russian") targetLang = "ru";
        else if (lowerStem == "en" || lowerStem == "english") targetLang = "en";
        else if (lowerStem == "fr" || lowerStem == "french") targetLang = "fr";
        else if (lowerStem == "de" || lowerStem == "german") targetLang = "de";
        else if (lowerStem == "es" || lowerStem == "spanish") targetLang = "es";
        else if (lowerStem == "pl" || lowerStem == "polish") targetLang = "pl";
        else if (lowerStem == "it" || lowerStem == "italian") targetLang = "it";
        else if (lowerStem == "ja" || lowerStem == "japanese") targetLang = "ja";
        else if (lowerStem == "zh" || lowerStem == "chinese") targetLang = "zh";
        else {
            size_t underscore = lowerStem.rfind('_');
            if (underscore != std::string::npos && underscore + 1 < lowerStem.size()) {
                std::string suffix = lowerStem.substr(underscore + 1);
                if (suffix == "ru" || suffix == "russian") targetLang = "ru";
                else if (suffix == "en" || suffix == "english") targetLang = "en";
                else if (suffix == "fr" || suffix == "french") targetLang = "fr";
                else if (suffix == "de" || suffix == "german") targetLang = "de";
                else if (suffix == "es" || suffix == "spanish") targetLang = "es";
                else if (suffix == "pl" || suffix == "polish") targetLang = "pl";
                else if (suffix == "it" || suffix == "italian") targetLang = "it";
                else if (suffix == "ja" || suffix == "japanese") targetLang = "ja";
                else if (suffix == "zh" || suffix == "chinese") targetLang = "zh";
                else targetLang = suffix;
            }
        }

        loadDictionary(targetLang, entry.path());
    }
}

std::vector<std::string> LocalizationService::availableLanguages() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> langs;
    for (const auto& [lang, dict] : m_dictionaries) {
        if (!dict.empty()) {
            langs.push_back(lang);
        }
    }
    if (std::find(langs.begin(), langs.end(), "en") == langs.end()) {
        langs.push_back("en");
    }
    if (std::find(langs.begin(), langs.end(), "ru") == langs.end()) {
        langs.push_back("ru");
    }
    std::sort(langs.begin(), langs.end());
    return langs;
}

} // namespace PerfUI
