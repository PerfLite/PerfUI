#pragma once

#include "Types.h"
#include "UIRenderBackend.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

struct ContextMenuItem {
    std::string text;
    std::function<void()> action;
    bool isSeparator{ false };
    bool disabled{ false };

    static ContextMenuItem Action(std::string text, std::function<void()> action, bool disabled = false) {
        ContextMenuItem item;
        item.text = std::move(text);
        item.action = std::move(action);
        item.disabled = disabled;
        return item;
    }

    static ContextMenuItem Separator() {
        ContextMenuItem item;
        item.isSeparator = true;
        return item;
    }
};

class PERFUI_API ContextMenu {
public:
    ContextMenu() = default;
    ContextMenu(Point position, std::vector<ContextMenuItem> items);
    ~ContextMenu() = default;

    void open(Point position, std::vector<ContextMenuItem> items);
    void close();
    bool isOpen() const { return m_isOpen; }

    const Rect& bounds() const { return m_bounds; }
    void update(float deltaTime);
    void render(UIRenderBackend& backend, Dimensions viewportSize);

    bool onPointerDown(const Point& screenPos);
    void onMouseMove(const Point& screenPos);

private:
    bool m_isOpen{ false };
    Point m_position{ 0.0f, 0.0f };
    Rect m_bounds{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::vector<ContextMenuItem> m_items;
    int m_hoveredIndex{ -1 };
    float m_animAlpha{ 0.0f };
};

} // namespace PerfUI
