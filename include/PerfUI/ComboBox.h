#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include <vector>
#include <string>
#include <functional>

namespace PerfUI {

class ComboBox : public UIElement {
public:
    explicit ComboBox(std::vector<std::string> options = {}, size_t initialIndex = 0, std::string name = "ComboBox");
    ~ComboBox() override;

    ComboBox& options(std::vector<std::string> opts);
    const std::vector<std::string>& options() const { return m_options; }

    size_t selectedIndex() const { return m_selectedIndex; }
    const std::string& selectedText() const;
    ComboBox& selectIndex(size_t index);

    ComboBox& onSelectionChanged(std::function<void(size_t index, const std::string& text)> callback) {
        m_onSelectionChanged = std::move(callback);
        return *this;
    }

    bool isOpen() const { return m_isOpen; }
    void setOpen(bool open);

    Rect dropdownBounds() const;
    void onDropdownMouseMove(const Point& screenPos);
    bool onDropdownPointerDown(const Point& screenPos);
    void renderDropdownOverlay(UIRenderBackend& backend);

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;

private:
    std::vector<std::string> m_options;
    size_t m_selectedIndex{ 0 };
    bool m_isOpen{ false };
    int m_hoveredIndex{ -1 };

    TextStyle m_textStyle{ Color::TextPrimary(), 12.0f, false, false };
    float m_cornerRadius{ 6.0f };
    AnimatedFloat m_hoverAnim{ 0.0f, 14.0f };
    AnimatedFloat m_openAnim{ 0.0f, 18.0f };

    std::function<void(size_t, const std::string&)> m_onSelectionChanged;
};

} // namespace PerfUI
