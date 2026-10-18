#include "PerfUI/SearchableList.h"
#include "PerfUI/Button.h"
#include "PerfUI/Theme.h"
#include <algorithm>
#include <cctype>

namespace PerfUI {

static std::string ToLower(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) {
        s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return s;
}

SearchableList::SearchableList(std::string placeholder, std::string name)
    : UIElement(std::move(name))
    , m_placeholder(std::move(placeholder))
{
    layout().direction(LayoutDirection::Vertical);
    layout().gap(6.0f);

    m_searchInput = add<TextInput>(m_placeholder);
    m_searchInput->setName("SearchInput");
    m_searchInput->layout().height(32.0f);
    m_searchInput->onTextChanged([this](const std::string& text) {
        filterItems(text);
    });

    m_scrollView = add<ScrollView>("SearchScroll");
    m_scrollView->layout().flex(1.0f);
    m_scrollView->showScrollbar(true);

    m_listContainer = m_scrollView->add<UIElement>("ListContainer");
    m_listContainer->layout().direction(LayoutDirection::Vertical);
    m_listContainer->layout().gap(2.0f);
}

void SearchableList::addItem(int id, std::string text) {
    m_items.push_back(SearchableListItem{ id, std::move(text) });
    rebuildListUI();
}

void SearchableList::clearItems() {
    m_items.clear();
    m_selectedId = -1;
    m_selectedText.clear();
    if (m_listContainer) {
        m_listContainer->clearChildren();
    }
    markLayoutDirty();
}

void SearchableList::select(int id) {
    m_selectedId = id;
    for (const auto& item : m_items) {
        if (item.id == id) {
            m_selectedText = item.text;
            break;
        }
    }

    if (m_listContainer) {
        for (const auto& child : m_listContainer->children()) {
            if (auto* btn = dynamic_cast<Button*>(child.get())) {
                bool isSel = (btn->name() == "Item_" + std::to_string(id));
                if (isSel) {
                    btn->normalColor(Color(45, 55, 75, 240), Theme::Current().colors.borderFocus, Color::White());
                } else {
                    btn->normalColor(Color(22, 27, 34, 200), Color::Transparent(), Theme::Current().colors.textPrimary);
                }
            }
        }
    }

    if (m_onItemSelected) {
        m_onItemSelected(m_selectedId, m_selectedText);
    }
}

void SearchableList::filterItems(std::string_view query) {
    if (!m_listContainer) return;

    std::string lowerQuery = ToLower(query);

    for (size_t i = 0; i < m_items.size() && i < m_listContainer->children().size(); ++i) {
        auto* child = m_listContainer->children()[i].get();
        if (lowerQuery.empty()) {
            child->setVisible(true);
        } else {
            std::string lowerItem = ToLower(m_items[i].text);
            bool match = (lowerItem.find(lowerQuery) != std::string::npos);
            child->setVisible(match);
        }
    }
    markLayoutDirty();
}

void SearchableList::rebuildListUI() {
    if (!m_listContainer) return;
    m_listContainer->clearChildren();

    for (const auto& item : m_items) {
        auto* btn = m_listContainer->add<Button>(item.text, "Item_" + std::to_string(item.id));
        btn->layout().height(28.0f);
        btn->layout().width(DimensionConstraint::Percent(100.0f));
        btn->cornerRadius(4.0f);

        bool isSel = (item.id == m_selectedId);
        if (isSel) {
            btn->normalColor(Color(45, 55, 75, 240), Theme::Current().colors.borderFocus, Color::White());
        } else {
            btn->normalColor(Color(22, 27, 34, 200), Color::Transparent(), Theme::Current().colors.textPrimary);
        }

        btn->onClick([this, id = item.id]() {
            select(id);
        });
    }

    markLayoutDirty();
}

void SearchableList::measure(Dimensions availableSize) {
    float w = availableSize.width > 0.0f ? availableSize.width : 200.0f;
    float h = availableSize.height > 0.0f ? availableSize.height : 250.0f;

    if (layout().width().mode == SizeMode::Fixed) w = layout().width().value;
    if (layout().height().mode == SizeMode::Fixed) h = layout().height().value;

    m_desiredSize = Dimensions{ w, h };
    UIElement::measure(m_desiredSize);
}

void SearchableList::arrange(const Rect& finalRect) {
    UIElement::arrange(finalRect);
}

} // namespace PerfUI
