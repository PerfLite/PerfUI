#pragma once

#include "PerfUI/Style/StyleTypes.h"
#include "PerfUI/Style/ComputedStyle.h"
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace PerfUI {

class UIElement;
class UIContext;

class StyleSheet {
public:
    StyleSheet() = default;
    ~StyleSheet() = default;

    /// Clears all rules, variables, and error/warning messages.
    void clear();

    /// Loads styles from a PSS string.
    bool loadString(std::string_view pss, const std::string& sourceName = "<string>");

    /// Loads styles from a .pss file.
    bool loadFile(const std::filesystem::path& path);

    /// Reloads all loaded style files from disk without clearing variables.
    bool reloadFiles();
    const std::vector<std::filesystem::path>& loadedFiles() const { return m_loadedFiles; }

    /// Variable management (:root { --name: value })
    void setVariable(std::string name, std::string value);
    std::string getVariable(const std::string& name) const;
    bool hasVariable(const std::string& name) const;
    const std::unordered_map<std::string, std::string>& variables() const { return m_variables; }

    /// Resolves var(--name, fallback) references in a string.
    std::string resolveVariables(const std::string& input) const;

    /// Adds a style rule manually.
    void addRule(StyleRule rule);

    /// Returns list of all parsed rules.
    const std::vector<StyleRule>& rules() const { return m_rules; }

    /// Finds matching rules for an element considering active pseudo-classes.
    /// Result is sorted in ascending specificity and source order (highest priority at the end).
    std::vector<const StyleRule*> matchRules(const UIElement* element, uint8_t activePseudos) const;

    /// Computes the final ComputedStyle for an element.
    ComputedStyle computeStyle(const UIElement* element, const ComputedStyle* parentStyle = nullptr) const;

    /// Diagnostics and error reporting.
    const std::vector<std::string>& warnings() const { return m_warnings; }
    const std::vector<std::string>& errors() const { return m_errors; }
    void addWarning(std::string warning) { m_warnings.push_back(std::move(warning)); }
    void addError(std::string error) { m_errors.push_back(std::move(error)); }

private:
    void rebuildIndex();

    std::vector<StyleRule> m_rules;
    std::unordered_map<std::string, std::string> m_variables;
    std::vector<std::string> m_warnings;
    std::vector<std::string> m_errors;

    // Fast rule lookup indices
    std::unordered_map<std::string, std::vector<size_t>> m_idIndex;
    std::unordered_map<std::string, std::vector<size_t>> m_classIndex;
    std::unordered_map<std::string, std::vector<size_t>> m_tagIndex;
    std::vector<size_t> m_universalIndex;

    std::vector<std::filesystem::path> m_loadedFiles;
};

} // namespace PerfUI
