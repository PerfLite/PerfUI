#pragma once

#include "Panel.h"
#include "Text.h"
#include "Button.h"
#include "ScrollView.h"
#include <vector>
#include <string>

namespace PerfUI {

struct QuestEntry {
    std::string title;
    std::string category;
    std::string location;
    std::string description;
    std::vector<std::pair<std::string, bool>> objectives;
};

class JournalWindow : public Panel {
public:
    explicit JournalWindow(std::string name = "JournalWindow");
    ~JournalWindow() override = default;

    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;

    void selectQuest(size_t index);

private:
    void initQuestData();
    void buildUI();
    void updateDetailsPanel();

    std::vector<QuestEntry> m_quests;
    size_t m_selectedIndex{ 0 };

    // Cached widget pointers for dynamic updates
    Text* m_questCategoryText{ nullptr };
    Text* m_questLocationText{ nullptr };
    Text* m_questTitleText{ nullptr };
    Text* m_questDescText{ nullptr };
    Panel* m_objectivesContainer{ nullptr };
    std::vector<Text*> m_objectiveTexts;
    std::vector<Button*> m_questButtons;
};

} // namespace PerfUI
