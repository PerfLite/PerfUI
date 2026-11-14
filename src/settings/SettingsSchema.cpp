#include "PerfUI/SettingsSchema.h"
#include "PerfUI/SettingsView.h"
#include "PerfUI/Localization.h"
#include "SimpleJson.h"
#include <fstream>
#include <sstream>

namespace PerfUI {

static std::string resolveLocalized(const JsonValue& jv, const std::string& fallback = "") {
    if (jv.isString()) {
        const auto& s = jv.asString();
        if (s.empty()) return fallback;
        if (s.front() == '$') {
            std::string res = LocalizationService::instance().expandText(s);
            if (res == s && !fallback.empty()) return fallback;
            return res;
        }
        return s;
    }
    if (jv.isObject()) {
        std::string key = jv.get("key").asString();
        std::string fb = jv.get("fallback").asString(fallback);
        if (!key.empty()) {
            return LocalizationService::instance().translate(key.front() == '$' ? key.substr(1) : key, fb);
        }
        return fb;
    }
    return fallback;
}

static SettingCondition parseCondition(const JsonValue& jv) {
    if (jv.isString()) {
        std::string s = jv.asString();
        if (!s.empty() && s.front() == '!') {
            return SettingCondition(s.substr(1), true);
        }
        return SettingCondition(s, false);
    }
    if (jv.isObject()) {
        std::string key = jv.get("key").asString();
        bool invert = jv.get("invert").asBool(false);
        if (jv.has("equals")) {
            const auto& eq = jv.get("equals");
            if (eq.isBool()) return SettingCondition(key, eq.asBool(), invert);
            if (eq.isInt()) return SettingCondition(key, static_cast<int32_t>(eq.asInt()), invert);
            if (eq.isFloat()) return SettingCondition(key, static_cast<float>(eq.asFloat()), invert);
            if (eq.isString()) return SettingCondition(key, eq.asString(), invert);
        }
        return SettingCondition(key, invert);
    }
    return SettingCondition();
}

static Color parseColorValue(const JsonValue& jv, Color defColor = Color::White()) {
    if (jv.isString()) {
        return Color::FromHex(jv.asString(), defColor);
    }
    if (jv.isArray()) {
        const auto& arr = jv.array();
        if (arr.size() >= 3) {
            uint8_t r = static_cast<uint8_t>(arr[0].asInt());
            uint8_t g = static_cast<uint8_t>(arr[1].asInt());
            uint8_t b = static_cast<uint8_t>(arr[2].asInt());
            uint8_t a = arr.size() >= 4 ? static_cast<uint8_t>(arr[3].asInt()) : 255;
            return Color(r, g, b, a);
        }
    }
    if (jv.isObject()) {
        uint8_t r = static_cast<uint8_t>(jv.get("r").asInt(defColor.r));
        uint8_t g = static_cast<uint8_t>(jv.get("g").asInt(defColor.g));
        uint8_t b = static_cast<uint8_t>(jv.get("b").asInt(defColor.b));
        uint8_t a = static_cast<uint8_t>(jv.get("a").asInt(defColor.a));
        return Color(r, g, b, a);
    }
    if (jv.isInt()) {
        uint32_t u = static_cast<uint32_t>(jv.asInt());
        return Color((u >> 24) & 0xFF, (u >> 16) & 0xFF, (u >> 8) & 0xFF, u & 0xFF);
    }
    return defColor;
}

SchemaParseResult SettingsSchemaLoader::loadFromFile(
    const std::filesystem::path& jsonPath,
    SettingsView* targetView,
    SettingsStore* store
) {
    SchemaParseResult res;
    std::ifstream f(jsonPath, std::ios::in | std::ios::binary);
    if (!f.is_open()) {
        res.success = false;
        res.errors.push_back("Failed to open schema file: " + jsonPath.string());
        return res;
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    return loadFromString(ss.str(), jsonPath.filename().string(), targetView, store);
}

SchemaParseResult SettingsSchemaLoader::loadFromString(
    std::string_view jsonContent,
    const std::string& sourceName,
    SettingsView* targetView,
    SettingsStore* store
) {
    SchemaParseResult res;
    if (!store) {
        store = &SettingsStore::GetSingleton();
    }

    std::string parseErr;
    JsonValue root = JsonValue::parse(jsonContent, &parseErr);
    if (!parseErr.empty() || !root.isObject()) {
        res.success = false;
        res.errors.push_back("[" + sourceName + "] JSON parse error: " + (parseErr.empty() ? "Root must be an object" : parseErr));
        return res;
    }

    res.modId = root.get("modName").asString();
    if (res.modId.empty()) {
        res.modId = root.get("id").asString();
    }
    if (res.modId.empty()) {
        res.modId = sourceName;
    }

    res.displayName = resolveLocalized(root.get("displayName"), res.modId);

    if (targetView) {
        targetView->setModId(res.modId);
    }

    // Process pages or flat content
    bool hasPages = root.has("pages") && root.get("pages").isArray() && !root.get("pages").array().empty();
    bool hasContent = root.has("content") && root.get("content").isArray();

    if (hasPages) {
        const auto& pagesArr = root.get("pages").array();
        for (size_t pIdx = 0; pIdx < pagesArr.size(); ++pIdx) {
            const auto& pageVal = pagesArr[pIdx];
            if (!pageVal.isObject()) {
                res.warnings.push_back("[" + sourceName + "] pages[" + std::to_string(pIdx) + "] is not an object, skipping.");
                continue;
            }

            std::string pageTitle = resolveLocalized(pageVal.get("pageDisplayName"), "");
            if (pageTitle.empty()) {
                pageTitle = resolveLocalized(pageVal.get("displayName"), "");
            }
            if (pageTitle.empty()) {
                pageTitle = resolveLocalized(pageVal.get("title"), "Page " + std::to_string(pIdx + 1));
            }
            res.pageTitles.push_back(pageTitle);

            SettingsSection* sec = nullptr;
            if (targetView) {
                sec = targetView->addSection(pageTitle);
            }

            if (pageVal.has("content") && pageVal.get("content").isArray()) {
                populateSectionFromContent(
                    sec,
                    res.modId,
                    pageVal.get("content"),
                    sourceName + "::" + pageTitle,
                    res.errors,
                    res.warnings,
                    store
                );
            }
        }
    }

    if (hasContent) {
        std::string secTitle = res.displayName.empty() ? res.modId : res.displayName;
        if (!hasPages) {
            res.pageTitles.push_back(secTitle);
        }

        SettingsSection* sec = nullptr;
        if (targetView) {
            sec = targetView->addSection(secTitle);
        }

        populateSectionFromContent(
            sec,
            res.modId,
            root.get("content"),
            sourceName + "::content",
            res.errors,
            res.warnings,
            store
        );
    }

    res.success = res.errors.empty();
    return res;
}

bool SettingsSchemaLoader::populateSectionFromContent(
    SettingsSection* section,
    const std::string& modId,
    const JsonValue& contentArray,
    const std::string& sourceName,
    std::vector<std::string>& outErrors,
    std::vector<std::string>& outWarnings,
    SettingsStore* store
) {
    if (!contentArray.isArray()) {
        outErrors.push_back("[" + sourceName + "] content is not an array");
        return false;
    }

    if (!store) {
        store = &SettingsStore::GetSingleton();
    }

    const auto& items = contentArray.array();
    std::string lastGroupControlId;

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        if (!item.isObject()) {
            outWarnings.push_back("[" + sourceName + "] item[" + std::to_string(i) + "] is not an object, skipping.");
            continue;
        }

        std::string type = item.get("type").asString();
        if (type.empty()) {
            outWarnings.push_back("[" + sourceName + "] item[" + std::to_string(i) + "] missing 'type', skipping.");
            continue;
        }

        std::string text = resolveLocalized(item.get("text"), "");
        if (text.empty()) {
            text = resolveLocalized(item.get("label"), "");
        }

        std::string help = resolveLocalized(item.get("help"), "");
        if (help.empty()) {
            help = resolveLocalized(item.get("tooltip"), "");
        }
        if (help.empty()) {
            help = resolveLocalized(item.get("description"), "");
        }

        std::string id = item.get("id").asString();
        if (id.empty()) {
            id = item.get("key").asString();
        }

        const auto& valOpts = item.get("valueOptions");

        // Helper to attach PSS classes
        auto applyClasses = [&](UIElement* elem) {
            if (!elem) return;
            if (item.has("class")) {
                const auto& c = item.get("class");
                if (c.isString()) elem->addClass(c.asString());
                else if (c.isArray()) {
                    for (const auto& cv : c.array()) {
                        if (cv.isString()) elem->addClass(cv.asString());
                    }
                }
            }
            if (item.has("classes") && item.get("classes").isArray()) {
                for (const auto& cv : item.get("classes").array()) {
                    if (cv.isString()) elem->addClass(cv.asString());
                }
            }
        };

        UIElement* createdWidget = nullptr;

        // Structural elements (no id required)
        if (type == "header") {
            if (section) {
                createdWidget = section->addHeader(text);
                applyClasses(createdWidget);
            }
            continue;
        } else if (type == "text") {
            if (section) {
                createdWidget = section->addText(text, help);
                applyClasses(createdWidget);
            }
            continue;
        } else if (type == "separator") {
            if (section) {
                createdWidget = section->addSeparator();
                applyClasses(createdWidget);
            }
            continue;
        } else if (type == "empty") {
            if (section) {
                float h = 8.0f;
                if (valOpts.has("step")) h = static_cast<float>(valOpts.get("step").asFloat(8.0));
                else if (item.has("height")) h = static_cast<float>(item.get("height").asFloat(8.0));
                createdWidget = section->addEmpty(h);
                applyClasses(createdWidget);
            }
            continue;
        }

        // Check if known interactive type
        bool isKnownInteractive = (type == "toggle" || type == "slider" || type == "stepper" ||
                                   type == "menu" || type == "enum" || type == "choice" ||
                                   type == "color" || type == "keymap" || type == "keybind" ||
                                   type == "input" || type == "textinput");
        if (!isKnownInteractive) {
            outWarnings.push_back("[" + sourceName + "] item[" + std::to_string(i) + "] unknown element type '" + type + "', skipping.");
            continue;
        }

        // Interactive controls require an ID
        if (id.empty()) {
            outErrors.push_back("[" + sourceName + "] item[" + std::to_string(i) + "] of type '" + type + "' missing required 'id', skipping.");
            continue;
        }

        if (type == "toggle") {
            bool defVal = false;
            if (valOpts.has("default")) {
                defVal = valOpts.get("default").asBool(false);
            }
            if (!store->hasSetting(modId, id)) {
                SettingDef def;
                def.key = id;
                def.type = SettingType::Bool;
                def.defaultValue = defVal;
                def.displayName = text;
                def.tooltip = help;
                store->registerSetting(modId, def);
            }

            if (section) {
                createdWidget = section->addToggle(text, id, help);
                applyClasses(createdWidget);
            }
        } else if (type == "slider") {
            float minVal = static_cast<float>(valOpts.get("min").asFloat(0.0));
            float maxVal = static_cast<float>(valOpts.get("max").asFloat(100.0));
            float stepVal = static_cast<float>(valOpts.get("step").asFloat(1.0));
            std::string fmt = valOpts.get("formatString").asString("%.2f");
            float defVal = static_cast<float>(valOpts.get("default").asFloat(minVal));

            if (!store->hasSetting(modId, id)) {
                SettingDef def;
                def.key = id;
                def.type = SettingType::Float;
                def.defaultValue = defVal;
                def.minValue = minVal;
                def.maxValue = maxVal;
                def.step = stepVal;
                def.formatString = fmt;
                def.displayName = text;
                def.tooltip = help;
                store->registerSetting(modId, def);
            }

            if (section) {
                createdWidget = section->addSlider(text, id, help);
                applyClasses(createdWidget);
            }
        } else if (type == "stepper") {
            float minVal = static_cast<float>(valOpts.get("min").asFloat(0.0));
            float maxVal = static_cast<float>(valOpts.get("max").asFloat(10.0));
            float stepVal = static_cast<float>(valOpts.get("step").asFloat(1.0));
            std::string fmt = valOpts.get("formatString").asString("%.0f");
            int32_t defVal = static_cast<int32_t>(valOpts.get("default").asInt(static_cast<int64_t>(minVal)));

            if (!store->hasSetting(modId, id)) {
                SettingDef def;
                def.key = id;
                def.type = SettingType::Int;
                def.defaultValue = defVal;
                def.minValue = minVal;
                def.maxValue = maxVal;
                def.step = stepVal;
                def.formatString = fmt;
                def.displayName = text;
                def.tooltip = help;
                store->registerSetting(modId, def);
            }

            if (section) {
                createdWidget = section->addStepper(text, id, help);
                applyClasses(createdWidget);
            }
        } else if (type == "menu" || type == "enum" || type == "choice") {
            std::vector<ChoiceOption> options;
            if (valOpts.has("options") && valOpts.get("options").isArray()) {
                const auto& optsArr = valOpts.get("options").array();
                for (size_t o = 0; o < optsArr.size(); ++o) {
                    const auto& ov = optsArr[o];
                    if (ov.isString()) {
                        std::string optText = resolveLocalized(ov, ov.asString());
                        options.push_back(ChoiceOption{ ov.asString(), optText });
                    } else if (ov.isObject()) {
                        std::string optId = ov.get("id").asString();
                        if (optId.empty()) optId = ov.get("value").toString();
                        std::string optText = resolveLocalized(ov.get("text"), optId);
                        options.push_back(ChoiceOption{ optId, optText });
                    }
                }
            }

            std::string defVal;
            if (valOpts.has("default")) {
                defVal = valOpts.get("default").toString();
            } else if (!options.empty()) {
                defVal = options.front().id;
            }

            if (!store->hasSetting(modId, id)) {
                SettingDef def;
                def.key = id;
                def.type = SettingType::String;
                def.defaultValue = defVal;
                def.options = options;
                def.displayName = text;
                def.tooltip = help;
                store->registerSetting(modId, def);
            }

            if (section) {
                createdWidget = section->addChoice(text, id, help);
                applyClasses(createdWidget);
            }
        } else if (type == "color") {
            Color defColor = parseColorValue(valOpts.get("default"), Color::White());

            if (!store->hasSetting(modId, id)) {
                SettingDef def;
                def.key = id;
                def.type = SettingType::Color;
                def.defaultValue = defColor;
                def.displayName = text;
                def.tooltip = help;
                store->registerSetting(modId, def);
            }

            if (section) {
                createdWidget = section->addColor(text, id, help);
                applyClasses(createdWidget);
            }
        } else if (type == "keymap" || type == "keybind") {
            uint32_t defKey = static_cast<uint32_t>(valOpts.get("default").asInt(0));

            if (!store->hasSetting(modId, id)) {
                SettingDef def;
                def.key = id;
                def.type = SettingType::KeyCode;
                def.defaultValue = defKey;
                def.displayName = text;
                def.tooltip = help;
                store->registerSetting(modId, def);
            }

            if (section) {
                createdWidget = section->addKeybind(text, id, help);
                applyClasses(createdWidget);
            }
        } else if (type == "input" || type == "textinput") {
            std::string defStr = valOpts.get("default").asString("");
            std::string placeholder = resolveLocalized(valOpts.get("placeholder"), "");

            if (!store->hasSetting(modId, id)) {
                SettingDef def;
                def.key = id;
                def.type = SettingType::String;
                def.defaultValue = defStr;
                def.displayName = text;
                def.tooltip = help;
                store->registerSetting(modId, def);
            }

            if (section) {
                createdWidget = section->addTextInput(text, id, placeholder, help);
                applyClasses(createdWidget);
            }
        }

        // Group condition & control handling
        if (item.has("groupControl") && item.get("groupControl").asBool()) {
            lastGroupControlId = id;
        }

        if (section) {
            if (item.has("groupCondition") && !lastGroupControlId.empty()) {
                section->setRowEnabledIf(id, SettingCondition(lastGroupControlId));
            }

            if (item.has("visibleIf")) {
                section->setRowVisibleIf(id, parseCondition(item.get("visibleIf")));
            }

            if (item.has("enabledIf")) {
                section->setRowEnabledIf(id, parseCondition(item.get("enabledIf")));
            }
        }
    }

    if (section) {
        section->updateConditions();
    }

    return true;
}

} // namespace PerfUI
