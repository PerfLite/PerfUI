#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

struct SegmentItem {
    std::string text;
    std::string value;
    Rect bounds;
};

class PERFUI_API SegmentedControl : public UIElement {
public:
    explicit SegmentedControl(std::string name = "SegmentedControl");
    ~SegmentedControl() override = default;

    SegmentedControl& addSegment(std::string text, std::string value = "");
    SegmentedControl& clearSegments();

    size_t segmentCount() const { return m_segments.size(); }
    size_t selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(size_t index);

    std::string selectedText() const;
    std::string selectedValue() const;

    SegmentedControl& onSelectionChanged(std::function<void(size_t index, const std::string& text)> callback) {
        m_onSelectionChanged = std::move(callback);
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
    void calculateThumbTarget();

    std::vector<SegmentItem> m_segments;
    size_t m_selectedIndex{ 0 };
    int m_hoveredIndex{ -1 };

    AnimatedFloat m_thumbX{ 0.0f, 18.0f };
    AnimatedFloat m_thumbWidth{ 0.0f, 18.0f };
    bool m_initializedThumb{ false };

    std::function<void(size_t index, const std::string& text)> m_onSelectionChanged;
};

} // namespace PerfUI
