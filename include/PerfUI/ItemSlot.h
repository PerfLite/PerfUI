#pragma once

#include "UIElement.h"
#include "Types.h"
#include <string>
#include <functional>

namespace PerfUI {

enum class ItemRarity {
    Common,      // Default Slate / Subtle White
    Fine,        // Verdant Green
    Superior,    // Frost Blue
    Epic,        // Arcane Purple
    Legendary    // Nordic Dragon Gold
};

class PERFUI_API ItemSlot : public UIElement {
public:
    explicit ItemSlot(std::string name = "ItemSlot");
    ~ItemSlot() override = default;

    // Item properties
    TextureId icon() const { return m_icon; }
    ItemSlot& icon(TextureId tex) { m_icon = tex; return *this; }

    int count() const { return m_count; }
    ItemSlot& count(int c) {
        if (m_count != c) {
            m_count = c;
            markLayoutDirty();
        }
        return *this;
    }

    ItemRarity rarity() const { return m_rarity; }
    ItemSlot& rarity(ItemRarity r) { m_rarity = r; return *this; }

    bool isEquipped() const { return m_equipped; }
    ItemSlot& equipped(bool eq) { m_equipped = eq; return *this; }

    bool isSelected() const { return m_selected; }
    ItemSlot& selected(bool sel) { m_selected = sel; return *this; }

    ItemSlot& onClick(std::function<void()> cb) { m_onClick = std::move(cb); return *this; }
    ItemSlot& onRightClick(std::function<void()> cb) { m_onRightClick = std::move(cb); return *this; }

    using UIElement::style;
    ItemSlot& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onAction(NavDirection dir) override;

    Color getRarityBorderColor() const;

private:
    TextureId m_icon{ 0 };
    int m_count{ 1 };
    ItemRarity m_rarity{ ItemRarity::Common };
    bool m_equipped{ false };
    bool m_selected{ false };

    std::function<void()> m_onClick;
    std::function<void()> m_onRightClick;
};

} // namespace PerfUI
