#pragma once

#include "Panel.h"
#include "Types.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

struct CategoryItem {
    std::string id;
    std::string title;
    std::string icon;
    std::string badge;
    Rect bounds;
};

class PERFUI_API CategoryList : public UIElement {
public:
    explicit CategoryList(std::string name = "CategoryList");
    ~CategoryList() override = default;

    CategoryList& addCategory(std::string id, std::string title, std::string icon = "", std::string badge = "");
    CategoryList& clearCategories();

    size_t count() const { return m_items.size(); }
    const std::string& selectedId() const;
    size_t selectedIndex() const { return m_selectedIndex; }

    void selectCategory(const std::string& id);
    void selectIndex(size_t index);

    CategoryList& itemHeight(float h) {
        m_itemHeight = h;
        return *this;
    }
    float itemHeight() const { return m_itemHeight; }

    CategoryList& onCategoryChanged(std::function<void(const std::string& id)> callback) {
        m_onCategoryChanged = std::move(callback);
        return *this;
    }

    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;
    void onPointerLeave() override;

private:
    std::vector<CategoryItem> m_items;
    size_t m_selectedIndex{ 0 };
    int m_hoveredIndex{ -1 };
    float m_itemHeight{ 36.0f };

    std::function<void(const std::string& id)> m_onCategoryChanged;
};

} // namespace PerfUI
