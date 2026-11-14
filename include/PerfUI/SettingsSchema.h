#pragma once

#include "Export.h"
#include "SettingsStore.h"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <filesystem>

namespace PerfUI {

class SettingsView;
class SettingsSection;
class JsonValue;

struct SchemaParseResult {
    bool success{ false };
    std::string modId;
    std::string displayName;
    std::vector<std::string> pageTitles;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class PERFUI_API SettingsSchemaLoader {
public:
    // Load declarative schema from JSON file into an existing SettingsView (or into SettingsStore + SettingsView)
    static SchemaParseResult loadFromFile(
        const std::filesystem::path& jsonPath,
        SettingsView* targetView = nullptr,
        SettingsStore* store = nullptr
    );

    // Load declarative schema from JSON string
    static SchemaParseResult loadFromString(
        std::string_view jsonContent,
        const std::string& sourceName = "<memory>",
        SettingsView* targetView = nullptr,
        SettingsStore* store = nullptr
    );

    // Populate a specific SettingsSection from a JSON array of content objects
    static bool populateSectionFromContent(
        SettingsSection* section,
        const std::string& modId,
        const JsonValue& contentArray,
        const std::string& sourceName,
        std::vector<std::string>& outErrors,
        std::vector<std::string>& outWarnings,
        SettingsStore* store = nullptr
    );
};

} // namespace PerfUI
