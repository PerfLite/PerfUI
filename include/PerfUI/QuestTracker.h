#pragma once

#include "UIElement.h"
#include "Types.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

struct QuestObjectiveItem {
    std::string text;
    bool completed{ false };
    bool optional{ false };
    int currentProgress{ 0 };
    int maxProgress{ 0 };
    Rect bounds;
};

class PERFUI_API QuestTracker : public UIElement {
public:
    explicit QuestTracker(std::string name = "QuestTracker");
    ~QuestTracker() override = default;

    QuestTracker& setQuestTitle(std::string title);
    const std::string& questTitle() const { return m_title; }

    QuestTracker& setQuestCategory(std::string category);
    const std::string& questCategory() const { return m_category; }

    QuestTracker& addObjective(
        std::string text,
        bool completed = false,
        int current = 0,
        int max = 0,
        bool optional = false
    );
    QuestTracker& clearObjectives();

    size_t objectiveCount() const { return m_objectives.size(); }
    void setObjectiveCompleted(size_t index, bool completed);
    void setObjectiveProgress(size_t index, int current, int max);

    bool isAllCompleted() const;

    QuestTracker& onObjectiveClicked(std::function<void(size_t index)> callback) {
        m_onObjectiveClicked = std::move(callback);
        return *this;
    }

    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;
    void onPointerLeave() override;

private:
    std::string m_title;
    std::string m_category;
    std::vector<QuestObjectiveItem> m_objectives;
    int m_hoveredObjective{ -1 };

    std::function<void(size_t index)> m_onObjectiveClicked;
};

} // namespace PerfUI
