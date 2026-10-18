#pragma once

#include "Panel.h"
#include "Types.h"
#include "Animation.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

struct SidebarItem {
    std::string id;
    std::string label;
    std::string icon;
    std::string badge;
    Rect bounds;
};

class PERFUI_API Sidebar : public Panel {
public:
    explicit Sidebar(std::string name = "Sidebar");
    ~Sidebar() override = default;

    Sidebar& setTitle(std::string title) {
        m_title = std::move(title);
        return *this;
    }
    const std::string& title() const { return m_title; }

    Sidebar& addItem(std::string id, std::string label, std::string icon = "", std::string badge = "");
    Sidebar& clearItems();

    size_t count() const { return m_items.size(); }
    const std::string& selectedId() const;
    size_t selectedIndex() const { return m_selectedIndex; }

    void selectItem(const std::string& id);
    void selectIndex(size_t index);

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }
    void toggleCollapsed() { setCollapsed(!m_collapsed); }

    Sidebar& expandedWidth(float w) {
        m_expandedWidth = w;
        if (!m_collapsed) m_widthAnim.setTarget(w);
        return *this;
    }
    float expandedWidth() const { return m_expandedWidth; }

    Sidebar& collapsedWidth(float w) {
        m_collapsedWidth = w;
        if (m_collapsed) m_widthAnim.setTarget(w);
        return *this;
    }
    float collapsedWidth() const { return m_collapsedWidth; }

    Sidebar& onItemSelected(std::function<void(const std::string& id)> callback) {
        m_onItemSelected = std::move(callback);
        return *this;
    }

    Sidebar& onCollapseToggled(std::function<void(bool collapsed)> callback) {
        m_onCollapseToggled = std::move(callback);
        return *this;
    }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;
    void onPointerLeave() override;

private:
    std::string m_title{ "PerfUI" };
    std::vector<SidebarItem> m_items;
    size_t m_selectedIndex{ 0 };
    int m_hoveredIndex{ -1 };
    bool m_hoveredToggle{ false };

    bool m_collapsed{ false };
    float m_expandedWidth{ 220.0f };
    float m_collapsedWidth{ 52.0f };
    float m_itemHeight{ 40.0f };

    Rect m_toggleBtnBounds;
    AnimatedFloat m_widthAnim{ 220.0f, 16.0f };

    std::function<void(const std::string& id)> m_onItemSelected;
    std::function<void(bool collapsed)> m_onCollapseToggled;
};

} // namespace PerfUI
