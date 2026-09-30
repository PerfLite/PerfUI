#pragma once

#include "UIElement.h"
#include "Types.h"
#include "UIRenderBackend.h"

namespace PerfUI {

class ScrollView : public UIElement {
public:
    explicit ScrollView(std::string name = "ScrollView");
    ~ScrollView() override = default;

    float scrollOffset() const { return m_scrollOffset; }
    ScrollView& scrollTo(float offset);
    ScrollView& scrollBy(float delta);

    float scrollSpeed() const { return m_scrollSpeed; }
    ScrollView& scrollSpeed(float speed) { m_scrollSpeed = speed; return *this; }

    bool showScrollbar() const { return m_showScrollbar; }
    ScrollView& showScrollbar(bool show) { m_showScrollbar = show; return *this; }

    float contentHeight() const { return m_contentHeight; }

    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;
    void render(UIRenderBackend& backend) override;

    UIElement* hitTest(const Point& point) override;
    bool onMouseWheel(float delta, const Point& localPoint) override;

private:
    float m_scrollOffset{ 0.0f };
    float m_contentHeight{ 0.0f };
    float m_scrollSpeed{ 32.0f };
    bool m_showScrollbar{ true };
    float m_scrollbarWidth{ 6.0f };

    Color m_trackColor{ 20, 25, 35, 120 };
    Color m_thumbColor{ 139, 148, 158, 180 };
};

} // namespace PerfUI
