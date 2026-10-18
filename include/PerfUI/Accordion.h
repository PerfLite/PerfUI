#pragma once

#include "UIElement.h"
#include "Animation.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace PerfUI {

class Accordion;

class PERFUI_API AccordionSection : public UIElement {
public:
    explicit AccordionSection(std::string title = "Section", std::unique_ptr<UIElement> content = nullptr, bool expanded = false, std::string name = "AccordionSection");
    ~AccordionSection() override = default;

    const std::string& title() const { return m_title; }
    AccordionSection& title(std::string t);

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);
    AccordionSection& expanded(bool exp) { setExpanded(exp); return *this; }

    UIElement* content() const { return m_content; }
    void setContent(std::unique_ptr<UIElement> content);

    using UIElement::style;
    AccordionSection& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;
    void render(UIRenderBackend& backend) override;

    bool onPointerDown(const Point& localPoint) override;
    bool onAction(NavDirection dir) override;

private:
    std::string m_title;
    bool m_expanded{ false };
    UIElement* m_content{ nullptr }; // Owned in m_children
    AnimatedFloat m_animProgress{ 0.0f, 12.0f };
    float m_headerHeight{ 32.0f };
    float m_contentDesiredHeight{ 0.0f };

    friend class Accordion;
};

class PERFUI_API Accordion : public UIElement {
public:
    explicit Accordion(std::string name = "Accordion");
    ~Accordion() override = default;

    bool isExclusive() const { return m_exclusive; }
    Accordion& exclusive(bool ex) { m_exclusive = ex; return *this; }

    AccordionSection* addSection(const std::string& title, std::unique_ptr<UIElement> content, bool expanded = false);

    using UIElement::style;
    Accordion& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void notifySectionToggled(AccordionSection* section);

private:
    bool m_exclusive{ false };
};

} // namespace PerfUI
