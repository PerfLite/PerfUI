#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>
#include "PerfUI/UIElement.h"
#include "MarkupTypes.h"

namespace PerfUI {

using WidgetFactory = std::function<UIElement*(UIElement* parent, const AttrMap& attrs)>;

class WidgetRegistry {
public:
    static WidgetRegistry& instance();

    void registerWidget(std::string tag, WidgetFactory f);
    UIElement* create(const std::string& tag, UIElement* parent, const AttrMap& attrs);
    bool hasWidget(const std::string& tag) const;

    void registerBuiltinWidgets();
    void clear();

private:
    WidgetRegistry();
    ~WidgetRegistry() = default;

    std::unordered_map<std::string, WidgetFactory> m_factories;
    bool m_builtinsRegistered{ false };
};

} // namespace PerfUI
