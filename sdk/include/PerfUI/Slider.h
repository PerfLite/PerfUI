#pragma once

#include "UIElement.h"
#include "Types.h"
#include "UIRenderBackend.h"
#include <string>
#include <string_view>
#include <functional>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace PerfUI {

class PERFUI_API Slider : public UIElement {
public:
    explicit Slider(float value = 0.5f, float minVal = 0.0f, float maxVal = 1.0f, std::string label = "", std::string name = "Slider");
    ~Slider() override = default;

    float value() const { return m_value; }
    Slider& value(float v);

    Slider& range(float minVal, float maxVal);
    float min() const { return m_minValue; }
    float max() const { return m_maxValue; }
    Slider& min(float m) { return range(m, m_maxValue); }
    Slider& max(float m) { return range(m_minValue, m); }
    Slider& step(float s) { m_step = s; return *this; }

    const std::string& label() const { return m_label; }
    Slider& label(std::string_view text);

    Slider& onValueChanged(std::function<void(float)> callback) {
        m_onValueChanged = std::move(callback);
        return *this;
    }

    Slider& fillColor(Color color) { m_fillColor = color; return *this; }
    Slider& trackColor(Color color) { m_trackColor = color; return *this; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onPointerUp(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;
    void update(float deltaTime) override;
    bool onAction(NavDirection dir) override;

private:
    void updateValueFromX(float localX);

    float m_value{ 0.5f };
    float m_minValue{ 0.0f };
    float m_maxValue{ 1.0f };
    float m_step{ 0.0f };
    std::string m_label;
    bool m_dragging{ false };

    float m_trackHeight{ 6.0f };
    float m_thumbRadius{ 7.0f };

    Color m_trackColor{ 24, 30, 42, 220 };
    Color m_trackBorder{ Color::BorderSubtle() };
    Color m_fillColor{ Color::NordicGold() };
    Color m_thumbColor{ Color::White() };
    TextStyle m_labelStyle{ Color::TextPrimary(), 13.0f, false, false };

    std::function<void(float)> m_onValueChanged;
};

} // namespace PerfUI
