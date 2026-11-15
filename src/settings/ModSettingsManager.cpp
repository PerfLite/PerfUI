#include "PerfUI/ModSettingsManager.h"
#include "PerfUI/SettingsWindow.h"
#include "PerfUI/SettingsView.h"
#include "PerfUI/SettingsSchema.h"
#include "PerfUI/PerfUIApi.h"
#include <algorithm>

namespace PerfUI {

ModSettingsManager& ModSettingsManager::GetSingleton() {
    static ModSettingsManager s_instance;
    return s_instance;
}

const ModSettingsEntry* ModSettingsManager::getMod(const std::string& modId) const {
    auto it = m_modIndex.find(modId);
    if (it != m_modIndex.end() && it->second < m_mods.size()) {
        return &m_mods[it->second];
    }
    return nullptr;
}

bool ModSettingsManager::registerJson(
    const std::string& modId,
    const std::string& displayName,
    const std::string& jsonPath
) {
    if (modId.empty()) return false;

    // Eagerly parse schema and register definitions in store
    if (!jsonPath.empty()) {
        SettingsSchemaLoader::loadFromFile(jsonPath, nullptr, &SettingsStore::GetSingleton());
    }

    auto it = m_modIndex.find(modId);
    if (it != m_modIndex.end()) {
        // Update existing registration without duplicating
        auto& entry = m_mods[it->second];
        entry.displayName = displayName.empty() ? modId : displayName;
        entry.jsonPath = jsonPath;
        entry.isCustomBuilder = false;
        entry.pageBuilder = nullptr;
        entry.builderUserData = nullptr;
    } else {
        ModSettingsEntry entry;
        entry.modId = modId;
        entry.displayName = displayName.empty() ? modId : displayName;
        entry.jsonPath = jsonPath;
        entry.isCustomBuilder = false;
        m_modIndex[modId] = m_mods.size();
        m_mods.push_back(std::move(entry));
    }

    if (m_window) {
        attachToWindow(m_window);
    }
    return true;
}

bool ModSettingsManager::registerPage(
    const std::string& modId,
    const std::string& displayName,
    void (*build)(void*, API::SettingsPageBuilder*),
    void* userData
) {
    if (modId.empty() || !build) return false;

    auto it = m_modIndex.find(modId);
    if (it != m_modIndex.end()) {
        // Update existing registration without duplicating
        auto& entry = m_mods[it->second];
        entry.displayName = displayName.empty() ? modId : displayName;
        entry.jsonPath.clear();
        entry.isCustomBuilder = true;
        entry.pageBuilder = build;
        entry.builderUserData = userData;
    } else {
        ModSettingsEntry entry;
        entry.modId = modId;
        entry.displayName = displayName.empty() ? modId : displayName;
        entry.isCustomBuilder = true;
        entry.pageBuilder = build;
        entry.builderUserData = userData;
        m_modIndex[modId] = m_mods.size();
        m_mods.push_back(std::move(entry));
    }

    if (m_window) {
        attachToWindow(m_window);
    }
    return true;
}

struct CBuilderContext {
    std::string modId;
    SettingsView* view{ nullptr };
    SettingsSection* section{ nullptr };
};

static void CBuilder_AddHeader(API::SettingsPageBuilder* self, const char* title) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (ctx && ctx->section && title) {
        ctx->section->addHeader(title);
    }
}

static void CBuilder_AddText(API::SettingsPageBuilder* self, const char* text, const char* tooltip) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (ctx && ctx->section && text) {
        ctx->section->addText(text, tooltip ? tooltip : "");
    }
}

static void CBuilder_AddSeparator(API::SettingsPageBuilder* self) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (ctx && ctx->section) {
        ctx->section->addSeparator();
    }
}

static void CBuilder_AddEmpty(API::SettingsPageBuilder* self, float height) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (ctx && ctx->section) {
        ctx->section->addEmpty(height > 0.0f ? height : 8.0f);
    }
}

static void CBuilder_AddToggle(API::SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, bool defaultValue) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (!ctx || !ctx->section || !key) return;

    auto& store = SettingsStore::GetSingleton();
    if (!store.hasSetting(ctx->modId, key)) {
        SettingDef def;
        def.key = key;
        def.type = SettingType::Bool;
        def.defaultValue = defaultValue;
        def.displayName = label ? label : key;
        def.tooltip = tooltip ? tooltip : "";
        store.registerSetting(ctx->modId, def);
    }
    ctx->section->addToggle(label ? label : key, key, tooltip ? tooltip : "");
}

static void CBuilder_AddSlider(API::SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, float minVal, float maxVal, float stepVal, float defaultValue, const char* formatString) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (!ctx || !ctx->section || !key) return;

    auto& store = SettingsStore::GetSingleton();
    if (!store.hasSetting(ctx->modId, key)) {
        SettingDef def;
        def.key = key;
        def.type = SettingType::Float;
        def.minValue = minVal;
        def.maxValue = maxVal;
        def.step = stepVal > 0.0f ? stepVal : 1.0f;
        def.defaultValue = defaultValue;
        def.formatString = formatString && *formatString ? formatString : "%.2f";
        def.displayName = label ? label : key;
        def.tooltip = tooltip ? tooltip : "";
        store.registerSetting(ctx->modId, def);
    }
    ctx->section->addSlider(label ? label : key, key, tooltip ? tooltip : "");
}

static void CBuilder_AddStepper(API::SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, int32_t minVal, int32_t maxVal, int32_t stepVal, int32_t defaultValue) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (!ctx || !ctx->section || !key) return;

    auto& store = SettingsStore::GetSingleton();
    if (!store.hasSetting(ctx->modId, key)) {
        SettingDef def;
        def.key = key;
        def.type = SettingType::Int;
        def.minValue = static_cast<float>(minVal);
        def.maxValue = static_cast<float>(maxVal);
        def.step = static_cast<float>(stepVal > 0 ? stepVal : 1);
        def.defaultValue = defaultValue;
        def.formatString = "%.0f";
        def.displayName = label ? label : key;
        def.tooltip = tooltip ? tooltip : "";
        store.registerSetting(ctx->modId, def);
    }
    ctx->section->addStepper(label ? label : key, key, tooltip ? tooltip : "");
}

static void CBuilder_AddChoice(API::SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, const char** optionIds, const char** optionLabels, size_t optionCount, const char* defaultId) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (!ctx || !ctx->section || !key) return;

    std::vector<ChoiceOption> opts;
    if (optionIds && optionCount > 0) {
        opts.reserve(optionCount);
        for (size_t i = 0; i < optionCount; ++i) {
            std::string oId = optionIds[i] ? optionIds[i] : "";
            std::string oLbl = (optionLabels && optionLabels[i]) ? optionLabels[i] : oId;
            opts.push_back(ChoiceOption{ std::move(oId), std::move(oLbl) });
        }
    }

    auto& store = SettingsStore::GetSingleton();
    if (!store.hasSetting(ctx->modId, key)) {
        SettingDef def;
        def.key = key;
        def.type = SettingType::String;
        def.defaultValue = (defaultId && *defaultId) ? std::string(defaultId) : (opts.empty() ? "" : opts.front().id);
        def.options = opts;
        def.displayName = label ? label : key;
        def.tooltip = tooltip ? tooltip : "";
        store.registerSetting(ctx->modId, def);
    }
    ctx->section->addChoice(label ? label : key, key, tooltip ? tooltip : "");
}

static void CBuilder_AddColor(API::SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, uint32_t defaultRgba) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (!ctx || !ctx->section || !key) return;

    Color defC((defaultRgba >> 24) & 0xFF, (defaultRgba >> 16) & 0xFF, (defaultRgba >> 8) & 0xFF, defaultRgba & 0xFF);
    auto& store = SettingsStore::GetSingleton();
    if (!store.hasSetting(ctx->modId, key)) {
        SettingDef def;
        def.key = key;
        def.type = SettingType::Color;
        def.defaultValue = defC;
        def.displayName = label ? label : key;
        def.tooltip = tooltip ? tooltip : "";
        store.registerSetting(ctx->modId, def);
    }
    ctx->section->addColor(label ? label : key, key, tooltip ? tooltip : "");
}

static void CBuilder_AddKeybind(API::SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, uint32_t defaultKey) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (!ctx || !ctx->section || !key) return;

    auto& store = SettingsStore::GetSingleton();
    if (!store.hasSetting(ctx->modId, key)) {
        SettingDef def;
        def.key = key;
        def.type = SettingType::KeyCode;
        def.defaultValue = defaultKey;
        def.displayName = label ? label : key;
        def.tooltip = tooltip ? tooltip : "";
        store.registerSetting(ctx->modId, def);
    }
    ctx->section->addKeybind(label ? label : key, key, tooltip ? tooltip : "");
}

static void CBuilder_AddTextInput(API::SettingsPageBuilder* self, const char* label, const char* key, const char* placeholder, const char* tooltip, const char* defaultText) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (!ctx || !ctx->section || !key) return;

    auto& store = SettingsStore::GetSingleton();
    if (!store.hasSetting(ctx->modId, key)) {
        SettingDef def;
        def.key = key;
        def.type = SettingType::String;
        def.defaultValue = defaultText ? std::string(defaultText) : "";
        def.displayName = label ? label : key;
        def.tooltip = tooltip ? tooltip : "";
        store.registerSetting(ctx->modId, def);
    }
    ctx->section->addTextInput(label ? label : key, key, placeholder ? placeholder : "", tooltip ? tooltip : "");
}

static void CBuilder_SetRowVisibleIf(API::SettingsPageBuilder* self, const char* key, const char* conditionKey, bool invert) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (ctx && ctx->section && key && conditionKey) {
        ctx->section->setRowVisibleIf(key, SettingCondition(conditionKey, invert));
    }
}

static void CBuilder_SetRowEnabledIf(API::SettingsPageBuilder* self, const char* key, const char* conditionKey, bool invert) {
    auto* ctx = static_cast<CBuilderContext*>(self->internalData);
    if (ctx && ctx->section && key && conditionKey) {
        ctx->section->setRowEnabledIf(key, SettingCondition(conditionKey, invert));
    }
}

void ModSettingsManager::attachToWindow(SettingsWindow* window) {
    if (!window) return;
    m_window = window;

    for (const auto& entry : m_mods) {
        std::string catId = "mod_" + entry.modId;
        auto* view = window->getOrCreateCategory(catId, entry.displayName, "settings", "MOD");
        if (!view) continue;

        view->clearSections();
        view->setModId(entry.modId);

        if (entry.isCustomBuilder && entry.pageBuilder) {
            auto* sec = view->addSection(entry.displayName);
            CBuilderContext ctx{ entry.modId, view, sec };

            API::SettingsPageBuilder builder;
            builder.AddHeader = CBuilder_AddHeader;
            builder.AddText = CBuilder_AddText;
            builder.AddSeparator = CBuilder_AddSeparator;
            builder.AddEmpty = CBuilder_AddEmpty;
            builder.AddToggle = CBuilder_AddToggle;
            builder.AddSlider = CBuilder_AddSlider;
            builder.AddStepper = CBuilder_AddStepper;
            builder.AddChoice = CBuilder_AddChoice;
            builder.AddColor = CBuilder_AddColor;
            builder.AddKeybind = CBuilder_AddKeybind;
            builder.AddTextInput = CBuilder_AddTextInput;
            builder.SetRowVisibleIf = CBuilder_SetRowVisibleIf;
            builder.SetRowEnabledIf = CBuilder_SetRowEnabledIf;
            builder.internalData = &ctx;

            entry.pageBuilder(entry.builderUserData, &builder);
        } else if (!entry.jsonPath.empty()) {
            SettingsSchemaLoader::loadFromFile(entry.jsonPath, view);
        }
    }
}

void ModSettingsManager::openSettings(const std::string& targetModId) {
    if (m_window) {
        attachToWindow(m_window);
        if (!targetModId.empty()) {
            m_window->selectCategory("mod_" + targetModId);
        }
        m_window->setVisible(true);
    }
}

void ModSettingsManager::closeSettings() {
    if (m_window) {
        m_window->close();
    }
}

bool ModSettingsManager::isSettingsOpen() const {
    return m_window && m_window->isVisible();
}

void ModSettingsManager::setSettingsWindow(SettingsWindow* win) {
    m_window = win;
    if (m_window) {
        attachToWindow(m_window);
    }
}

void ModSettingsManager::clear() {
    m_mods.clear();
    m_modIndex.clear();
}

namespace API {

static bool API_RegisterModSettingsJson(const char* modId, const char* displayName, const char* jsonPath) {
    if (!modId || !*modId) return false;
    return ModSettingsManager::GetSingleton().registerJson(modId, displayName ? displayName : "", jsonPath ? jsonPath : "");
}

static bool API_RegisterModSettingsPage(const char* modId, const char* displayName, void (*build)(void*, SettingsPageBuilder*), void* userData) {
    if (!modId || !*modId || !build) return false;
    return ModSettingsManager::GetSingleton().registerPage(modId, displayName ? displayName : "", build, userData);
}

static bool API_GetSettingBool(const char* modId, const char* key, bool defaultVal) {
    return SettingsStore::GetSingleton().getBool(modId ? modId : "", key ? key : "", defaultVal);
}

static void API_SetSettingBool(const char* modId, const char* key, bool val) {
    SettingsStore::GetSingleton().setBool(modId ? modId : "", key ? key : "", val);
}

static int32_t API_GetSettingInt(const char* modId, const char* key, int32_t defaultVal) {
    return SettingsStore::GetSingleton().getInt(modId ? modId : "", key ? key : "", defaultVal);
}

static void API_SetSettingInt(const char* modId, const char* key, int32_t val) {
    SettingsStore::GetSingleton().setInt(modId ? modId : "", key ? key : "", val);
}

static float API_GetSettingFloat(const char* modId, const char* key, float defaultVal) {
    return SettingsStore::GetSingleton().getFloat(modId ? modId : "", key ? key : "", defaultVal);
}

static void API_SetSettingFloat(const char* modId, const char* key, float val) {
    SettingsStore::GetSingleton().setFloat(modId ? modId : "", key ? key : "", val);
}

static const char* API_GetSettingString(const char* modId, const char* key, const char* defaultVal) {
    thread_local static std::string s_lastString;
    s_lastString = SettingsStore::GetSingleton().getString(modId ? modId : "", key ? key : "", defaultVal ? defaultVal : "");
    return s_lastString.c_str();
}

static void API_SetSettingString(const char* modId, const char* key, const char* val) {
    SettingsStore::GetSingleton().setString(modId ? modId : "", key ? key : "", val ? val : "");
}

static uint32_t API_GetSettingColor(const char* modId, const char* key, uint32_t defaultRgba) {
    Color defC((defaultRgba >> 24) & 0xFF, (defaultRgba >> 16) & 0xFF, (defaultRgba >> 8) & 0xFF, defaultRgba & 0xFF);
    Color c = SettingsStore::GetSingleton().getColor(modId ? modId : "", key ? key : "", defC);
    return (static_cast<uint32_t>(c.r) << 24) | (static_cast<uint32_t>(c.g) << 16) | (static_cast<uint32_t>(c.b) << 8) | static_cast<uint32_t>(c.a);
}

static void API_SetSettingColor(const char* modId, const char* key, uint32_t rgba) {
    Color c((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
    SettingsStore::GetSingleton().setColor(modId ? modId : "", key ? key : "", c);
}

static uint32_t API_GetSettingKey(const char* modId, const char* key, uint32_t defaultKey) {
    return SettingsStore::GetSingleton().getKeyCode(modId ? modId : "", key ? key : "", defaultKey);
}

static void API_SetSettingKey(const char* modId, const char* key, uint32_t keycode) {
    SettingsStore::GetSingleton().setKeyCode(modId ? modId : "", key ? key : "", keycode);
}

static uint64_t API_SubscribeSettingChanged(const char* modId, const char* key, void (*cb)(void*, const char*, const char*), void* userData) {
    if (!cb) return 0;
    return SettingsStore::GetSingleton().subscribe(modId ? modId : "", key ? key : "", [cb, userData](const std::string& m, const std::string& k, const SettingValue&) {
        cb(userData, m.c_str(), k.c_str());
    });
}

static void API_UnsubscribeSettingChanged(uint64_t subscriptionId) {
    SettingsStore::GetSingleton().unsubscribe(subscriptionId);
}

static void API_OpenSettings(const char* modId) {
    ModSettingsManager::GetSingleton().openSettings(modId ? modId : "");
}

static void API_CloseSettings() {
    ModSettingsManager::GetSingleton().closeSettings();
}

void PopulateApiV4(IPerfUI_v4& api) {
    api.RegisterModSettingsJson = API_RegisterModSettingsJson;
    api.RegisterModSettingsPage = API_RegisterModSettingsPage;
    api.GetSettingBool = API_GetSettingBool;
    api.SetSettingBool = API_SetSettingBool;
    api.GetSettingInt = API_GetSettingInt;
    api.SetSettingInt = API_SetSettingInt;
    api.GetSettingFloat = API_GetSettingFloat;
    api.SetSettingFloat = API_SetSettingFloat;
    api.GetSettingString = API_GetSettingString;
    api.SetSettingString = API_SetSettingString;
    api.GetSettingColor = API_GetSettingColor;
    api.SetSettingColor = API_SetSettingColor;
    api.GetSettingKey = API_GetSettingKey;
    api.SetSettingKey = API_SetSettingKey;
    api.SubscribeSettingChanged = API_SubscribeSettingChanged;
    api.UnsubscribeSettingChanged = API_UnsubscribeSettingChanged;
    api.OpenSettings = API_OpenSettings;
    api.CloseSettings = API_CloseSettings;
}

} // namespace API

} // namespace PerfUI
