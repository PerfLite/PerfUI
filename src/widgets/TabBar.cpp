#include "PerfUI/TabBar.h"

namespace PerfUI {

TabBar::TabBar(std::string name)
    : Panel(std::move(name))
{
    backgroundColor(Color::Transparent());
    borderWidth(0.0f);
    layout()
        .direction(LayoutDirection::Horizontal)
        .gap(6.0f)
        .alignment(Alignment::Center);
}

TabBar& TabBar::addTab(std::string title) {
    m_tabTitles.push_back(std::move(title));
    rebuildTabButtons();
    return *this;
}

void TabBar::selectTab(size_t index) {
    if (index >= m_tabTitles.size()) return;
    m_selectedTab = index;

    for (size_t i = 0; i < m_tabButtons.size(); ++i) {
        if (i == m_selectedTab) {
            m_tabButtons[i]->normalColor(Color(42, 54, 72, 250), Color::BorderFocus(), Color::NordicGold());
            m_tabButtons[i]->hoverColor(Color(50, 64, 86, 255), Color::BorderFocus(), Color::White());
        } else {
            m_tabButtons[i]->normalColor(Color(20, 26, 36, 180), Color::BorderSubtle(), Color::TextSecondary());
            m_tabButtons[i]->hoverColor(Color(32, 42, 56, 220), Color::BorderStrong(), Color::White());
        }
    }

    if (m_onTabChanged) {
        m_onTabChanged(m_selectedTab);
    }
}

void TabBar::rebuildTabButtons() {
    clearChildren();
    m_tabButtons.clear();

    for (size_t i = 0; i < m_tabTitles.size(); ++i) {
        auto* btn = add<Button>(m_tabTitles[i]);
        btn->layout().padding(14.0f, 6.0f);
        btn->fontSize(12.0f);
        btn->cornerRadius(6.0f);

        if (i == m_selectedTab) {
            btn->normalColor(Color(42, 54, 72, 250), Color::BorderFocus(), Color::NordicGold());
            btn->hoverColor(Color(50, 64, 86, 255), Color::BorderFocus(), Color::White());
        } else {
            btn->normalColor(Color(20, 26, 36, 180), Color::BorderSubtle(), Color::TextSecondary());
            btn->hoverColor(Color(32, 42, 56, 220), Color::BorderStrong(), Color::White());
        }

        btn->onClick([this, i]() {
            selectTab(i);
        });

        m_tabButtons.push_back(btn);
    }

    markLayoutDirty();
}

void TabBar::update(float deltaTime) {
    Panel::update(deltaTime);
}

} // namespace PerfUI
