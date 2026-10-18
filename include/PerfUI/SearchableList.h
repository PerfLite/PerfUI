#pragma once

#include "UIElement.h"
#include "TextInput.h"
#include "ScrollView.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

struct SearchableListItem {
    int id{ 0 };
    std::string text;
};

class PERFUI_API SearchableList : public UIElement {
public:
    explicit SearchableList(std::string placeholder = "Search...", std::string name = "SearchableList");
    ~SearchableList() override = default;

    void addItem(int id, std::string text);
    void clearItems();

    int selectedId() const { return m_selectedId; }
    const std::string& selectedText() const { return m_selectedText; }
    void select(int id);

    SearchableList& onItemSelected(std::function<void(int, const std::string&)> cb) {
        m_onItemSelected = std::move(cb);
        return *this;
    }

    using UIElement::style;
    SearchableList& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;

    void filterItems(std::string_view query);

private:
    std::string m_placeholder;
    TextInput* m_searchInput{ nullptr };
    ScrollView* m_scrollView{ nullptr };
    UIElement* m_listContainer{ nullptr };

    std::vector<SearchableListItem> m_items;
    int m_selectedId{ -1 };
    std::string m_selectedText;
    std::function<void(int, const std::string&)> m_onItemSelected;

    void rebuildListUI();
};

} // namespace PerfUI
