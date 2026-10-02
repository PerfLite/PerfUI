#pragma once

#include "PerfUI/JournalWindow.h"
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

namespace PerfUI::Skyrim {

struct SkyrimDataBundle {
    std::vector<PerfUI::QuestEntry> activeQuests;
    std::vector<PerfUI::QuestEntry> completedQuests;
    PerfUI::PlayerStats playerStats;
};

class SkyrimQuestService {
public:
    static SkyrimQuestService& GetSingleton();

    // Requests asynchronous refresh on game thread
    void RequestQuestRefresh(std::function<void(SkyrimDataBundle)> onComplete = nullptr);

    // Reads bundle synchronously on game thread
    SkyrimDataBundle ReadBundleGameThread();

    // Sets or clears quest tracking marker
    void SetQuestActive(uint32_t formId, bool active);

    // Render thread retrieves newly prepared bundle
    bool ConsumeNewData(SkyrimDataBundle& outBundle);

private:
    SkyrimQuestService() = default;
    ~SkyrimQuestService() = default;

    std::mutex m_mutex;
    SkyrimDataBundle m_cachedBundle;
    std::atomic<bool> m_hasNewData{ false };
};

} // namespace PerfUI::Skyrim
