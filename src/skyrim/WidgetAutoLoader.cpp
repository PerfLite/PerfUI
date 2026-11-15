#include "Pch.h"
#include "WidgetAutoLoader.h"
#include "PerfUI/Markup/MarkupLoader.h"
#include "PerfUI/Lua/ScriptHost.h"
#include "PerfUI/Lua/ScriptCallbackResolver.h"
#include "PerfUI/Panel.h"
#include "PerfUI/Overlay.h"
#include "SkyrimSoundService.h"
#include "WorldTrackingService.h"
#include "PerfUI/SettingsStore.h"
#include "PerfUI/ModSettingsManager.h"
#include <spdlog/spdlog.h>
#include <system_error>
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace PerfUI::Skyrim {

struct WidgetAutoLoader::LoadedWidget {
    std::string name;
    std::filesystem::path dirPath;
    std::filesystem::path xmlPath;
    std::filesystem::path luaPath;
    std::filesystem::file_time_type luaWriteTime{};
    std::chrono::steady_clock::time_point lastLuaCheck{};

    std::unique_ptr<Panel> rootPanel;
    std::unique_ptr<ScriptHost> scriptHost;
    std::unique_ptr<ScriptCallbackResolver> callbackResolver;
    std::unique_ptr<MarkupLoader> markupLoader;
    OverlayId overlayId{ 0 };

    void pollLua() {
        if (luaPath.empty() || !scriptHost) return;
        auto now = std::chrono::steady_clock::now();
        if (now - lastLuaCheck < std::chrono::milliseconds(500)) return;
        lastLuaCheck = now;

        std::error_code ec;
        auto currentWriteTime = std::filesystem::last_write_time(luaPath, ec);
        if (!ec && currentWriteTime != luaWriteTime) {
            luaWriteTime = currentWriteTime;
            SKSE::log::info("WidgetAutoLoader: Hot-reloading Lua script for widget '{}': {}", name, luaPath.string());
            scriptHost->loadFile(luaPath);
            if (scriptHost->hasFunction("onReload")) {
                scriptHost->call("onReload");
            }
        }
    }
};

static bool IsPlayerInWorld() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    return player && player->GetParentCell() != nullptr;
}

static RE::ActorValue ResolveActorValue(std::string_view rawName) {
    if (rawName.empty()) return RE::ActorValue::kNone;

    // Fast check if integer ID
    char* end = nullptr;
    long id = std::strtol(rawName.data(), &end, 10);
    if (end != rawName.data() && *end == '\0') {
        if (id >= 0 && id < 164) {
            return static_cast<RE::ActorValue>(id);
        }
    }

    std::string name;
    name.reserve(rawName.size());
    for (char c : rawName) {
        name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    static const std::unordered_map<std::string, RE::ActorValue> s_avMap = {
        { "carryweight", RE::ActorValue::kCarryWeight },
        { "inventoryweight", RE::ActorValue::kInventoryWeight },
        { "health", RE::ActorValue::kHealth },
        { "magicka", RE::ActorValue::kMagicka },
        { "stamina", RE::ActorValue::kStamina },
        { "healrate", RE::ActorValue::kHealRate },
        { "magickarate", RE::ActorValue::kMagickaRate },
        { "staminarate", RE::ActorValue::kStaminaRate },
        { "speedmult", RE::ActorValue::kSpeedMult },
        { "onehanded", RE::ActorValue::kOneHanded },
        { "twohanded", RE::ActorValue::kTwoHanded },
        { "archery", RE::ActorValue::kArchery },
        { "marksman", RE::ActorValue::kArchery },
        { "block", RE::ActorValue::kBlock },
        { "smithing", RE::ActorValue::kSmithing },
        { "heavyarmor", RE::ActorValue::kHeavyArmor },
        { "lightarmor", RE::ActorValue::kLightArmor },
        { "pickpocket", RE::ActorValue::kPickpocket },
        { "lockpicking", RE::ActorValue::kLockpicking },
        { "sneak", RE::ActorValue::kSneak },
        { "alchemy", RE::ActorValue::kAlchemy },
        { "speech", RE::ActorValue::kSpeech },
        { "speechcraft", RE::ActorValue::kSpeech },
        { "alteration", RE::ActorValue::kAlteration },
        { "conjuration", RE::ActorValue::kConjuration },
        { "destruction", RE::ActorValue::kDestruction },
        { "illusion", RE::ActorValue::kIllusion },
        { "restoration", RE::ActorValue::kRestoration },
        { "enchanting", RE::ActorValue::kEnchanting },
        { "dragonsouls", RE::ActorValue::kDragonSouls },
        { "damageresist", RE::ActorValue::kDamageResist },
        { "poisonresist", RE::ActorValue::kPoisonResist },
        { "resistfire", RE::ActorValue::kResistFire },
        { "resistshock", RE::ActorValue::kResistShock },
        { "resistfrost", RE::ActorValue::kResistFrost },
        { "resistmagic", RE::ActorValue::kResistMagic },
        { "resistdisease", RE::ActorValue::kResistDisease },
        { "shoutrecoverymult", RE::ActorValue::kShoutRecoveryMult },
        { "mass", RE::ActorValue::kMass }
    };

    auto it = s_avMap.find(name);
    return it != s_avMap.end() ? it->second : RE::ActorValue::kNone;
}

static double GetPlayerActorValue(std::string_view name) {
    if (!IsPlayerInWorld()) return 0.0;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return 0.0;
    auto* avOwner = player->AsActorValueOwner();
    if (!avOwner) return 0.0;

    auto av = ResolveActorValue(name);
    if (av == RE::ActorValue::kNone) return 0.0;
    return static_cast<double>(avOwner->GetActorValue(av));
}

static double GetPlayerBaseActorValue(std::string_view name) {
    if (!IsPlayerInWorld()) return 0.0;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return 0.0;
    auto* avOwner = player->AsActorValueOwner();
    if (!avOwner) return 0.0;

    auto av = ResolveActorValue(name);
    if (av == RE::ActorValue::kNone) return 0.0;
    return static_cast<double>(avOwner->GetBaseActorValue(av));
}

static double GetPlayerPermanentActorValue(std::string_view name) {
    if (!IsPlayerInWorld()) return 0.0;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return 0.0;
    auto* avOwner = player->AsActorValueOwner();
    if (!avOwner) return 0.0;

    auto av = ResolveActorValue(name);
    if (av == RE::ActorValue::kNone) return 0.0;
    return static_cast<double>(avOwner->GetPermanentActorValue(av));
}

static double GetPlayerItemCount(uint32_t formId) {
    if (!IsPlayerInWorld()) return 0.0;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return 0.0;

    // 0xF (15) is Gold in Skyrim
    if (formId == 0xF || formId == 15) {
        using getGold_t = int(*)(RE::Actor*);
        static REL::Relocation<getGold_t> funcGetGold{ RELOCATION_ID(36527, 37527) };
        return static_cast<double>(funcGetGold(player));
    }

    auto* form = RE::TESForm::LookupByID(formId);
    if (!form) return 0.0;
    auto* bound = form->As<RE::TESBoundObject>();
    if (!bound) return 0.0;
    return static_cast<double>(player->GetItemCount(bound));
}

static double GetGlobalVariable(std::string_view name) {
    auto* form = RE::TESForm::LookupByEditorID(name);
    if (form) {
        if (auto* g = form->As<RE::TESGlobal>()) {
            return static_cast<double>(g->value);
        }
    }
    return 0.0;
}

static bool IsMenuOpen(std::string_view menuName) {
    auto* ui = RE::UI::GetSingleton();
    return ui ? ui->IsMenuOpen(RE::BSFixedString(menuName.data())) : false;
}

static void PlaySound(std::string_view soundId) {
    SkyrimSoundService::PlayUISound(soundId.data());
}

void WidgetAutoLoader::RegisterSkyrimLuaBindings(ScriptHost& host) {
    // 1. skyrim.player sub-module
    host.registerModule("skyrim.player", [](ModuleBuilder& m) {
        m.function("getActorValue", [](std::string avName) -> double {
            return GetPlayerActorValue(avName);
        });
        m.function("getBaseActorValue", [](std::string avName) -> double {
            return GetPlayerBaseActorValue(avName);
        });
        m.function("getPermanentActorValue", [](std::string avName) -> double {
            return GetPlayerPermanentActorValue(avName);
        });
        m.function("getItemCount", [](uint32_t formId) -> double {
            return GetPlayerItemCount(formId);
        });
        m.function("getLevel", []() -> double {
            if (!IsPlayerInWorld()) return 1.0;
            auto* player = RE::PlayerCharacter::GetSingleton();
            return player ? static_cast<double>(player->GetLevel()) : 1.0;
        });
        m.function("isInCombat", []() -> bool {
            if (!IsPlayerInWorld()) return false;
            auto* player = RE::PlayerCharacter::GetSingleton();
            return player ? player->IsInCombat() : false;
        });
    });

    // 2. skyrim.game sub-module
    host.registerModule("skyrim.game", [](ModuleBuilder& m) {
        m.function("getGlobal", [](std::string name) -> double {
            return GetGlobalVariable(name);
        });
        m.function("isMenuOpen", [](std::string menuName) -> bool {
            return IsMenuOpen(menuName);
        });
        m.function("playSound", [](std::string soundId) {
            PlaySound(soundId);
        });
    });

    // 3. skyrim.tracking sub-module
    host.registerModule("skyrim.tracking", [](ModuleBuilder& m) {
        m.function("trackActor", [](uint32_t formId, double ox, double oy, double oz, bool checkLoS) -> double {
            auto* form = RE::TESForm::LookupByID(formId);
            if (!form) return 0.0;
            auto* refr = form->As<RE::TESObjectREFR>();
            if (!refr) return 0.0;
            uint32_t tid = WorldTrackingService::GetSingleton().RegisterHandle(
                refr->GetHandle(), RE::NiPoint3{ static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz) }, checkLoS);
            return static_cast<double>(tid);
        });
        m.function("trackPosition", [](double x, double y, double z, bool checkLoS) -> double {
            uint32_t tid = WorldTrackingService::GetSingleton().RegisterStatic(
                RE::NiPoint3{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) }, checkLoS);
            return static_cast<double>(tid);
        });
        m.function("untrack", [](uint32_t targetId) {
            WorldTrackingService::GetSingleton().Unregister(targetId);
        });
    });

    // 3. Convenience root functions on skyrim table
    host.registerModule("skyrim", [](ModuleBuilder& m) {
        m.function("getActorValue", [](std::string avName) -> double {
            return GetPlayerActorValue(avName);
        });
        m.function("getBaseActorValue", [](std::string avName) -> double {
            return GetPlayerBaseActorValue(avName);
        });
        m.function("getPermanentActorValue", [](std::string avName) -> double {
            return GetPlayerPermanentActorValue(avName);
        });
        m.function("getItemCount", [](uint32_t formId) -> double {
            return GetPlayerItemCount(formId);
        });
        m.function("getGlobal", [](std::string name) -> double {
            return GetGlobalVariable(name);
        });
        m.function("isMenuOpen", [](std::string menuName) -> bool {
            return IsMenuOpen(menuName);
        });
        m.function("playSound", [](std::string soundId) {
            PlaySound(soundId);
        });
        m.function("trackActor", [](uint32_t formId, double ox, double oy, double oz, bool checkLoS) -> double {
            auto* form = RE::TESForm::LookupByID(formId);
            if (!form) return 0.0;
            auto* refr = form->As<RE::TESObjectREFR>();
            if (!refr) return 0.0;
            uint32_t tid = WorldTrackingService::GetSingleton().RegisterHandle(
                refr->GetHandle(), RE::NiPoint3{ static_cast<float>(ox), static_cast<float>(oy), static_cast<float>(oz) }, checkLoS);
            return static_cast<double>(tid);
        });
        m.function("trackPosition", [](double x, double y, double z, bool checkLoS) -> double {
            uint32_t tid = WorldTrackingService::GetSingleton().RegisterStatic(
                RE::NiPoint3{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) }, checkLoS);
            return static_cast<double>(tid);
        });
        m.function("untrack", [](uint32_t targetId) {
            WorldTrackingService::GetSingleton().Unregister(targetId);
        });
    });

    // 3. settings module (isolated to host.name())
    std::string modName = host.name();
    host.registerModule("settings", [modName](ModuleBuilder& m) {
        m.function("get", [modName](std::string key) -> std::string {
            return SettingsStore::GetSingleton().getString(modName, key);
        });
        m.function("getBool", [modName](std::string key, bool defVal) -> bool {
            return SettingsStore::GetSingleton().getBool(modName, key, defVal);
        });
        m.function("getInt", [modName](std::string key, int32_t defVal) -> int32_t {
            return SettingsStore::GetSingleton().getInt(modName, key, defVal);
        });
        m.function("getFloat", [modName](std::string key, double defVal) -> double {
            return static_cast<double>(SettingsStore::GetSingleton().getFloat(modName, key, static_cast<float>(defVal)));
        });
        m.function("getString", [modName](std::string key, std::string defVal) -> std::string {
            return SettingsStore::GetSingleton().getString(modName, key, defVal);
        });
        m.function("set", [modName](std::string key, std::string val) {
            SettingsStore::GetSingleton().setString(modName, key, val);
        });
        m.function("setBool", [modName](std::string key, bool val) {
            SettingsStore::GetSingleton().setBool(modName, key, val);
        });
        m.function("setInt", [modName](std::string key, int32_t val) {
            SettingsStore::GetSingleton().setInt(modName, key, val);
        });
        m.function("setFloat", [modName](std::string key, double val) {
            SettingsStore::GetSingleton().setFloat(modName, key, static_cast<float>(val));
        });
        m.function("register", [modName](std::string path) -> bool {
            return ModSettingsManager::GetSingleton().registerJson(modName, modName, path);
        });
    });
}

void WidgetAutoLoader::Initialize(UIContext* ctx) {
    std::lock_guard lock(m_mutex);
    if (!ctx) {
        SKSE::log::warn("WidgetAutoLoader::Initialize called with null UIContext");
        return;
    }
    m_context = ctx;
    m_initialized = true;
    ScanAndLoad();
}

void WidgetAutoLoader::ScanAndLoad() {
    if (!m_context) return;

    const std::vector<std::filesystem::path> searchRoots = {
        "Data/Interface/widgets",
        "Interface/widgets",
        "SKSE/Plugins/PerfUI/widgets"
    };

    std::vector<std::filesystem::path> foundDirs;
    std::error_code ec;

    for (const auto& root : searchRoots) {
        if (!std::filesystem::exists(root, ec)) continue;

        for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
            if (entry.is_directory(ec)) {
                foundDirs.push_back(entry.path());
            }
        }
    }

    for (const auto& dir : foundDirs) {
        std::string widgetName = dir.filename().string();

        // Avoid duplicate loading if same name already registered
        bool alreadyLoaded = false;
        for (const auto& existing : m_widgets) {
            if (existing && existing->name == widgetName) {
                alreadyLoaded = true;
                break;
            }
        }
        if (alreadyLoaded) continue;

        // Check for widget.xml (or any .xml in folder)
        std::filesystem::path xmlPath = dir / "widget.xml";
        if (!std::filesystem::exists(xmlPath, ec)) {
            for (const auto& subEntry : std::filesystem::directory_iterator(dir, ec)) {
                if (subEntry.is_regular_file(ec) && subEntry.path().extension() == ".xml") {
                    xmlPath = subEntry.path();
                    break;
                }
            }
        }

        if (!std::filesystem::exists(xmlPath, ec)) {
            continue; // No XML found in widget folder
        }

        // Check for widget.lua
        std::filesystem::path luaPath = dir / "widget.lua";
        if (!std::filesystem::exists(luaPath, ec)) {
            std::filesystem::path altLua = xmlPath;
            altLua.replace_extension(".lua");
            if (std::filesystem::exists(altLua, ec)) {
                luaPath = altLua;
            } else {
                luaPath.clear();
            }
        }

        SKSE::log::info("WidgetAutoLoader: Discovering standalone widget '{}' at {}", widgetName, xmlPath.string());

        auto w = std::make_shared<LoadedWidget>();
        w->name = widgetName;
        w->dirPath = dir;
        w->xmlPath = xmlPath;
        w->luaPath = luaPath;

        // Isolated root container spanning screen
        w->rootPanel = std::make_unique<Panel>(widgetName + "Root");
        w->rootPanel->setContext(m_context);
        w->rootPanel->backgroundColor(Color::Transparent());
        w->rootPanel->borderWidth(0.0f);
        w->rootPanel->shadow(false);

        // Lua Script Host
        w->scriptHost = std::make_unique<ScriptHost>(widgetName);
        w->scriptHost->bindContext(m_context, w->rootPanel.get());
        RegisterSkyrimLuaBindings(*w->scriptHost);

        // Callback resolver
        w->callbackResolver = std::make_unique<ScriptCallbackResolver>(*w->scriptHost);

        // Markup loader with hot-reload enabled
        w->markupLoader = std::make_unique<MarkupLoader>(*m_context);
        w->markupLoader->setCallbackResolver(w->callbackResolver.get());
        w->markupLoader->enableHotReload(true);
        w->markupLoader->setPollInterval(std::chrono::milliseconds(300));

        w->markupLoader->loadFile(w->xmlPath, w->rootPanel.get());

        // If luaPath was not found initially, check if XML specified script attribute
        if (w->luaPath.empty() && !w->markupLoader->scriptAttribute().empty()) {
            std::filesystem::path scriptAttrPath = dir / w->markupLoader->scriptAttribute();
            if (std::filesystem::exists(scriptAttrPath, ec)) {
                w->luaPath = scriptAttrPath;
            }
        }

        // Load Lua script if present
        if (!w->luaPath.empty() && std::filesystem::exists(w->luaPath, ec)) {
            w->luaWriteTime = std::filesystem::last_write_time(w->luaPath, ec);
            w->scriptHost->loadFile(w->luaPath);
            if (w->scriptHost->hasFunction("onLoad")) {
                w->scriptHost->call("onLoad");
            }
        }

        // Hot-reload reconnect
        w->markupLoader->onAfterReload([this, wWeak = std::weak_ptr<LoadedWidget>(w)](UIElement*) {
            auto widget = wWeak.lock();
            if (!widget || !widget->scriptHost) return;

            widget->scriptHost->bindContext(m_context, widget->rootPanel.get());
            if (!widget->luaPath.empty() && std::filesystem::exists(widget->luaPath)) {
                widget->scriptHost->loadFile(widget->luaPath);
            }
            if (widget->scriptHost->hasFunction("onReload")) {
                widget->scriptHost->call("onReload");
            }
        });

        // Register overlay
        w->overlayId = m_context->registerOverlay(w->name.c_str(), [wWeak = std::weak_ptr<LoadedWidget>(w)](OverlayContext& ctx) {
            auto widget = wWeak.lock();
            if (!widget) return;

            auto* skseUi = RE::UI::GetSingleton();
            if (skseUi && (skseUi->IsMenuOpen(RE::MainMenu::MENU_NAME) || skseUi->IsMenuOpen(RE::LoadingMenu::MENU_NAME))) {
                return;
            }

            if (!IsPlayerInWorld()) {
                return;
            }

            if (widget->markupLoader) {
                widget->markupLoader->poll();
            }
            if (widget->scriptHost) {
                widget->pollLua();
                widget->scriptHost->update(ctx.deltaTime);
                if (widget->scriptHost->hasFunction("onUpdate")) {
                    widget->scriptHost->call("onUpdate", { static_cast<double>(ctx.deltaTime) });
                }
            }
            if (widget->rootPanel) {
                widget->rootPanel->measure(ctx.screenSize);
                widget->rootPanel->arrange(Rect{ 0.0f, 0.0f, ctx.screenSize.width, ctx.screenSize.height });
                widget->rootPanel->render(ctx.renderer);
            }
        }, 5, true);

        m_widgets.push_back(w);
        SKSE::log::info("WidgetAutoLoader: Successfully loaded widget '{}' (overlay id={})", widgetName, w->overlayId);
    }
}

} // namespace PerfUI::Skyrim
