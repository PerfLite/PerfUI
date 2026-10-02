#pragma once

#include "Panel.h"
#include "Text.h"
#include "Button.h"
#include "ScrollView.h"
#include "Checkbox.h"
#include "ProgressBar.h"
#include "Slider.h"
#include "TabBar.h"
#include "ComboBox.h"
#include "TextInput.h"
#include "SettingsView.h"
#include <vector>
#include <string>
#include <functional>

namespace PerfUI {

struct QuestObjective {
    std::string text;
    bool completed{ false };
    uint32_t index{ 0 };
};

struct QuestEntry {
    std::string title;
    std::string category;
    std::string location;
    std::string description;
    bool active{ false };
    uint32_t formId{ 0 };
    std::vector<QuestObjective> objectives;
};

struct PlayerStats {
    std::string name{ "Dragonborn" };
    int level{ 1 };
    int gold{ 0 };
    float health{ 100.0f };
    float maxHealth{ 100.0f };
    float magicka{ 100.0f };
    float maxMagicka{ 100.0f };
    float stamina{ 100.0f };
    float maxStamina{ 100.0f };
    int activeQuestsCount{ 0 };
    int completedQuestsCount{ 0 };
};

class JournalWindow : public Panel {
public:
    explicit JournalWindow(std::string name = "JournalWindow");
    ~JournalWindow() override = default;

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;

    void selectQuest(size_t index);
    void setQuests(std::vector<QuestEntry> activeQuests, std::vector<QuestEntry> completedQuests = {});
    void setPlayerStats(const PlayerStats& stats);

    void onTrackQuest(std::function<void(uint32_t formId, bool active)> callback) {
        m_onTrackQuest = std::move(callback);
    }

    bool onPointerDown(const Point& localPoint) override;
    bool onPointerUp(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;

    void setCustomPosition(const Point& pos);
    void resetPosition();
    Point customPosition() const { return m_customPosition; }

private:
    void initQuestData();
    void buildUI();
    void rebuildQuestList();
    void updateDetailsPanel();
    void updateStatsView();
    void buildSettingsView();
    void switchView(size_t tabIndex);
    void openQuestContextMenu(size_t questIdx);

    std::vector<QuestEntry> m_activeQuests;
    std::vector<QuestEntry> m_completedQuests;
    PlayerStats m_playerStats;

    size_t m_currentViewIndex{ 0 }; // 0 = Active, 1 = Completed, 2 = Stats, 3 = Settings
    size_t m_selectedActiveIndex{ 0 };
    size_t m_selectedCompletedIndex{ 0 };

    std::function<void(uint32_t formId, bool active)> m_onTrackQuest;

    // Header & Tab Bar
    TabBar* m_tabBar{ nullptr };
    Text* m_profilerText{ nullptr };
    float m_profilerTimer{ 0.0f };

    // Views
    Panel* m_questViewPanel{ nullptr };
    Panel* m_statsViewPanel{ nullptr };
    SettingsView* m_settingsView{ nullptr };

    // Quest View Elements
    ComboBox* m_categoryFilter{ nullptr };
    std::string m_selectedCategoryFilter{ "All Quests" };
    TextInput* m_searchInput{ nullptr };
    std::string m_searchQuery;
    std::vector<size_t> m_filteredIndices;

    ScrollView* m_questScrollView{ nullptr };
    Text* m_questCategoryText{ nullptr };
    Text* m_questLocationText{ nullptr };
    Text* m_questTitleText{ nullptr };
    Text* m_questDescText{ nullptr };
    ProgressBar* m_progressBar{ nullptr };
    Panel* m_objectivesContainer{ nullptr };
    Button* m_setActiveBtn{ nullptr };
    std::vector<Checkbox*> m_objectiveCheckboxes;
    std::vector<Button*> m_questButtons;

    // Stats View Elements
    Text* m_statsHeroNameText{ nullptr };
    Text* m_statsHeroLevelText{ nullptr };
    Text* m_statsHeroGoldText{ nullptr };
    ProgressBar* m_healthBar{ nullptr };
    ProgressBar* m_magickaBar{ nullptr };
    ProgressBar* m_staminaBar{ nullptr };
    Text* m_statsActiveCountText{ nullptr };
    Text* m_statsCompletedCountText{ nullptr };

    Slider* m_opacitySlider{ nullptr };

    Point m_customPosition{ -1.0f, -1.0f };
    bool m_isDragging{ false };
    Point m_dragStartMouse{ 0.0f, 0.0f };
    Point m_dragStartPos{ 0.0f, 0.0f };
    float m_headerHeight{ 60.0f };
};

} // namespace PerfUI
