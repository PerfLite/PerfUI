#include "Pch.h"
#include "SkyrimQuestService.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace PerfUI::Skyrim {

SkyrimQuestService& SkyrimQuestService::GetSingleton() {
    static SkyrimQuestService instance;
    return instance;
}

void SkyrimQuestService::RequestQuestRefresh(std::function<void(SkyrimDataBundle)> onComplete) {
    SKSE::GetTaskInterface()->AddTask([this, onComplete]() {
        auto bundle = ReadBundleGameThread();
        if (!bundle.activeQuests.empty() || !bundle.completedQuests.empty()) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cachedBundle = bundle;
            m_hasNewData.store(true);
        }
        if (onComplete) {
            onComplete(std::move(bundle));
        }
    });
}

bool SkyrimQuestService::ConsumeNewData(SkyrimDataBundle& outBundle) {
    if (!m_hasNewData.load()) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    outBundle = m_cachedBundle;
    m_hasNewData.store(false);
    return true;
}

static std::string GetQuestCategory(RE::QUEST_DATA::Type type) {
    switch (type) {
        case RE::QUEST_DATA::Type::kMainQuest:        return "MAIN QUEST";
        case RE::QUEST_DATA::Type::kMagesGuild:       return "COLLEGE OF WINTERHOLD";
        case RE::QUEST_DATA::Type::kThievesGuild:     return "THIEVES GUILD";
        case RE::QUEST_DATA::Type::kDarkBrotherhood:  return "DARK BROTHERHOOD";
        case RE::QUEST_DATA::Type::kCompanionsQuest:  return "THE COMPANIONS";
        case RE::QUEST_DATA::Type::kDaedric:          return "DAEDRIC QUEST";
        case RE::QUEST_DATA::Type::kSideQuest:        return "SIDE QUEST";
        case RE::QUEST_DATA::Type::kCivilWar:         return "CIVIL WAR";
        case RE::QUEST_DATA::Type::kDLC01_Vampire:     return "DAWNGUARD";
        case RE::QUEST_DATA::Type::kDLC02_Dragonborn:  return "DRAGONBORN";
        case RE::QUEST_DATA::Type::kMiscellaneous:    return "MISCELLANEOUS";
        default: return "QUEST";
    }
}

static PerfUI::QuestEntry ConvertQuest(RE::TESQuest* quest) {
    PerfUI::QuestEntry entry;
    entry.title = quest->GetName();
    entry.category = GetQuestCategory(quest->GetType());
    entry.formId = quest->GetFormID();
    entry.active = quest->IsActive();
    entry.location = "Skyrim";

    std::ostringstream ss;
    ss << "Stage: " << std::dec << quest->GetCurrentStageID();
    const char* edId = quest->GetFormEditorID();
    if (edId && edId[0]) {
        ss << " [" << edId << "]";
    }
    ss << " | FormID: 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << quest->GetFormID();
    entry.description = ss.str();

    for (auto* obj : quest->objectives) {
        if (!obj) continue;
        auto state = obj->state.get();
        if (state == RE::QUEST_OBJECTIVE_STATE::kDormant) {
            continue;
        }

        bool completed = (state == RE::QUEST_OBJECTIVE_STATE::kCompleted ||
                          state == RE::QUEST_OBJECTIVE_STATE::kCompletedDisplayed);

        const char* objText = obj->displayText.c_str();
        if (objText && objText[0]) {
            entry.objectives.push_back({ objText, completed, obj->index });
        }
    }

    if (entry.objectives.empty()) {
        entry.objectives.push_back({ "Objective in progress", quest->IsCompleted(), 0 });
    }

    return entry;
}

SkyrimDataBundle SkyrimQuestService::ReadBundleGameThread() {
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!dataHandler || !player || !player->GetParentCell()) {
        SKSE::log::info("SkyrimQuestService: Player cell not loaded or not in active world space");
        return {};
    }

    SkyrimDataBundle bundle;

    // 1. Player Stats
    const char* playerName = player->GetName();
    bundle.playerStats.name = (playerName && playerName[0]) ? playerName : "Dovahkiin";
    bundle.playerStats.level = player->GetLevel();
    using getGold_t = int(*)(RE::Actor*);
    static REL::Relocation<getGold_t> funcGetGold{ RELOCATION_ID(36527, 37527) };
    bundle.playerStats.gold = funcGetGold(player);
    auto* avOwner = player->AsActorValueOwner();
    if (avOwner) {
        bundle.playerStats.health = avOwner->GetActorValue(RE::ActorValue::kHealth);
        bundle.playerStats.maxHealth = (std::max)(1.0f, avOwner->GetBaseActorValue(RE::ActorValue::kHealth));
        bundle.playerStats.magicka = avOwner->GetActorValue(RE::ActorValue::kMagicka);
        bundle.playerStats.maxMagicka = (std::max)(1.0f, avOwner->GetBaseActorValue(RE::ActorValue::kMagicka));
        bundle.playerStats.stamina = avOwner->GetActorValue(RE::ActorValue::kStamina);
        bundle.playerStats.maxStamina = (std::max)(1.0f, avOwner->GetBaseActorValue(RE::ActorValue::kStamina));
    }

    // 2. Quests
    auto& questArray = dataHandler->GetFormArray<RE::TESQuest>();
    bundle.activeQuests.reserve(32);
    bundle.completedQuests.reserve(32);

    for (auto* quest : questArray) {
        if (!quest) continue;

        const char* name = quest->GetName();
        if (!name || !name[0]) {
            continue; // Technical quest
        }

        if (quest->IsRunning() && !quest->IsCompleted() && !quest->IsStopped()) {
            bundle.activeQuests.push_back(ConvertQuest(quest));
        } else if (quest->IsCompleted()) {
            bundle.completedQuests.push_back(ConvertQuest(quest));
        }
    }

    // Sort active quests: tracked first, then title
    std::sort(bundle.activeQuests.begin(), bundle.activeQuests.end(), [](const PerfUI::QuestEntry& a, const PerfUI::QuestEntry& b) {
        if (a.active != b.active) return a.active > b.active;
        return a.title < b.title;
    });

    // Sort completed quests alphabetically
    std::sort(bundle.completedQuests.begin(), bundle.completedQuests.end(), [](const PerfUI::QuestEntry& a, const PerfUI::QuestEntry& b) {
        return a.title < b.title;
    });

    bundle.playerStats.activeQuestsCount = static_cast<int>(bundle.activeQuests.size());
    bundle.playerStats.completedQuestsCount = static_cast<int>(bundle.completedQuests.size());

    SKSE::log::info("SkyrimQuestService: Loaded {} active, {} completed quests, player lvl {}",
                    bundle.activeQuests.size(), bundle.completedQuests.size(), bundle.playerStats.level);

    return bundle;
}

void SkyrimQuestService::SetQuestActive(uint32_t formId, bool active) {
    SKSE::GetTaskInterface()->AddTask([formId, active]() {
        auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(formId);
        if (quest) {
            if (active) {
                quest->data.flags.set(RE::QuestFlag::kActive);
            } else {
                quest->data.flags.reset(RE::QuestFlag::kActive);
            }
            quest->AddChange(RE::TESQuest::ChangeFlags::kQuestFlags);
            SKSE::log::info("SkyrimQuestService: Quest 0x{:08X} active toggled to {}", formId, active);
        }
    });
}

} // namespace PerfUI::Skyrim
