#pragma once

#include "Panel.h"
#include "Button.h"
#include "Types.h"
#include <vector>
#include <string>
#include <functional>

namespace PerfUI {

class TabBar : public Panel {
public:
    explicit TabBar(std::string name = "TabBar");
    ~TabBar() override = default;

    TabBar& addTab(std::string title);
    TabBar& onTabChanged(std::function<void(size_t index)> callback) {
        m_onTabChanged = std::move(callback);
        return *this;
    }

    size_t selectedTab() const { return m_selectedTab; }
    void selectTab(size_t index);

    void update(float deltaTime) override;

private:
    void rebuildTabButtons();

    std::vector<std::string> m_tabTitles;
    size_t m_selectedTab{ 0 };
    std::vector<Button*> m_tabButtons;
    std::function<void(size_t index)> m_onTabChanged;
};

} // namespace PerfUI
