#include "PerfUI/Markup/WidgetRegistry.h"
#include "MarkupLayoutParser.h"
#include "PerfUI/Panel.h"
#include "PerfUI/Text.h"
#include "PerfUI/Button.h"
#include "PerfUI/ScrollView.h"
#include "PerfUI/Checkbox.h"
#include "PerfUI/ProgressBar.h"
#include "PerfUI/Slider.h"
#include "PerfUI/Image.h"
#include "PerfUI/TabBar.h"
#include "PerfUI/ComboBox.h"
#include "PerfUI/TextInput.h"

#include <sstream>

namespace PerfUI {

using namespace MarkupInternal;

WidgetRegistry::WidgetRegistry() {
    registerBuiltinWidgets();
}

WidgetRegistry& WidgetRegistry::instance() {
    static WidgetRegistry s_instance;
    return s_instance;
}

void WidgetRegistry::registerWidget(std::string tag, WidgetFactory f) {
    m_factories[std::move(tag)] = std::move(f);
}

UIElement* WidgetRegistry::create(const std::string& tag, UIElement* parent, const AttrMap& attrs) {
    auto it = m_factories.find(tag);
    if (it == m_factories.end()) {
        return nullptr;
    }
    return it->second(parent, attrs);
}

bool WidgetRegistry::hasWidget(const std::string& tag) const {
    return m_factories.find(tag) != m_factories.end();
}

void WidgetRegistry::clear() {
    m_factories.clear();
    m_builtinsRegistered = false;
}

void WidgetRegistry::registerBuiltinWidgets() {
    if (m_builtinsRegistered) return;

    // 1. Panel
    registerWidget("Panel", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Panel";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto panel = std::make_unique<Panel>(name);

        for (const auto& [k, v] : attrs) {
            if (k == "bg" || k == "background") {
                panel->backgroundColor(parseColorOrToken(v));
            } else if (k == "border") {
                panel->borderColor(parseColorOrToken(v));
            } else if (k == "border-width") {
                panel->borderWidth(parseNumberOrToken(v));
            } else if (k == "radius") {
                panel->cornerRadius(parseNumberOrToken(v));
            } else if (k == "shadow") {
                panel->shadow(parseBool(v, true));
            }
        }

        if (parent) {
            return parent->addChild(std::move(panel));
        }
        return panel.release();
    });

    // 2. Text
    registerWidget("Text", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Text";
        std::string textContent = "";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itText = attrs.find("text");
        if (itText != attrs.end()) textContent = itText->second;

        auto txt = std::make_unique<Text>(textContent, name);

        for (const auto& [k, v] : attrs) {
            if (k == "font" || k == "font-size") {
                txt->fontSize(parseNumberOrToken(v, 14.0f));
            } else if (k == "color") {
                txt->color(parseColorOrToken(v));
            } else if (k == "bold") {
                txt->bold(parseBool(v, true));
            } else if (k == "italic") {
                txt->italic(parseBool(v, true));
            } else if (k == "wrap") {
                txt->wrap(parseBool(v, true));
            }
        }

        if (parent) {
            return parent->addChild(std::move(txt));
        }
        return txt.release();
    });

    // 3. Button
    registerWidget("Button", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Button";
        std::string labelText = "Button";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itText = attrs.find("text");
        if (itText != attrs.end()) labelText = itText->second;
        else {
            auto itLabel = attrs.find("label");
            if (itLabel != attrs.end()) labelText = itLabel->second;
        }

        auto btn = std::make_unique<Button>(labelText, name);

        for (const auto& [k, v] : attrs) {
            if (k == "font" || k == "font-size") {
                btn->fontSize(parseNumberOrToken(v, 14.0f));
            } else if (k == "radius") {
                btn->cornerRadius(parseNumberOrToken(v, 6.0f));
            }
        }

        if (parent) {
            return parent->addChild(std::move(btn));
        }
        return btn.release();
    });

    // 4. ScrollView
    registerWidget("ScrollView", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "ScrollView";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto sv = std::make_unique<ScrollView>(name);

        for (const auto& [k, v] : attrs) {
            if (k == "scrollSpeed" || k == "scroll-speed") {
                sv->scrollSpeed(parseNumberOrToken(v, 32.0f));
            } else if (k == "showScrollbar" || k == "show-scrollbar") {
                sv->showScrollbar(parseBool(v, true));
            } else if (k == "offset") {
                sv->scrollTo(parseNumberOrToken(v, 0.0f));
            }
        }

        if (parent) {
            return parent->addChild(std::move(sv));
        }
        return sv.release();
    });

    // 5. Checkbox
    registerWidget("Checkbox", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Checkbox";
        std::string labelText = "";
        bool chk = false;
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itLabel = attrs.find("label");
        if (itLabel != attrs.end()) labelText = itLabel->second;
        else {
            auto itText = attrs.find("text");
            if (itText != attrs.end()) labelText = itText->second;
        }
        auto itChecked = attrs.find("checked");
        if (itChecked != attrs.end()) chk = parseBool(itChecked->second, false);

        auto cb = std::make_unique<Checkbox>(labelText, chk, name);

        for (const auto& [k, v] : attrs) {
            if (k == "boxSize" || k == "box-size") {
                cb->boxSize(parseNumberOrToken(v, 18.0f));
            } else if (k == "fontSize" || k == "font-size") {
                cb->fontSize(parseNumberOrToken(v, 14.0f));
            } else if (k == "checkColor" || k == "check-color") {
                cb->checkColor(parseColorOrToken(v));
            } else if (k == "labelColor" || k == "label-color") {
                cb->labelColor(parseColorOrToken(v));
            }
        }

        if (parent) {
            return parent->addChild(std::move(cb));
        }
        return cb.release();
    });

    // 6. ProgressBar
    registerWidget("ProgressBar", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "ProgressBar";
        float progressVal = 0.0f;
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itVal = attrs.find("value");
        if (itVal != attrs.end()) progressVal = parseNumberOrToken(itVal->second, 0.0f);
        else {
            auto itProg = attrs.find("progress");
            if (itProg != attrs.end()) progressVal = parseNumberOrToken(itProg->second, 0.0f);
        }

        auto pb = std::make_unique<ProgressBar>(progressVal, name);

        for (const auto& [k, v] : attrs) {
            if (k == "showLabel" || k == "show-label") {
                pb->showLabel(parseBool(v, true));
            } else if (k == "fillColor" || k == "fill-color") {
                pb->fillColor(parseColorOrToken(v));
            } else if (k == "trackColor" || k == "track-color") {
                pb->trackColor(parseColorOrToken(v));
            } else if (k == "height") {
                pb->height(parseNumberOrToken(v, 14.0f));
            } else if (k == "radius") {
                pb->cornerRadius(parseNumberOrToken(v, 5.0f));
            }
        }

        if (parent) {
            return parent->addChild(std::move(pb));
        }
        return pb.release();
    });

    // 7. Slider
    registerWidget("Slider", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Slider";
        float val = 0.5f;
        float minVal = 0.0f;
        float maxVal = 1.0f;
        std::string labelText = "";

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itVal = attrs.find("value");
        if (itVal != attrs.end()) val = parseNumberOrToken(itVal->second, 0.5f);
        auto itMin = attrs.find("min");
        if (itMin != attrs.end()) minVal = parseNumberOrToken(itMin->second, 0.0f);
        auto itMax = attrs.find("max");
        if (itMax != attrs.end()) maxVal = parseNumberOrToken(itMax->second, 1.0f);
        auto itLabel = attrs.find("label");
        if (itLabel != attrs.end()) labelText = itLabel->second;

        auto sl = std::make_unique<Slider>(val, minVal, maxVal, labelText, name);

        for (const auto& [k, v] : attrs) {
            if (k == "step") {
                sl->step(parseNumberOrToken(v, 0.0f));
            } else if (k == "fillColor" || k == "fill-color") {
                sl->fillColor(parseColorOrToken(v));
            } else if (k == "trackColor" || k == "track-color") {
                sl->trackColor(parseColorOrToken(v));
            }
        }

        if (parent) {
            return parent->addChild(std::move(sl));
        }
        return sl.release();
    });

    // 8. Image
    registerWidget("Image", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Image";
        uint64_t tex = 0;
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itTex = attrs.find("texture");
        if (itTex != attrs.end()) tex = static_cast<uint64_t>(std::strtoull(itTex->second.c_str(), nullptr, 10));

        auto img = std::make_unique<Image>(tex, name);

        for (const auto& [k, v] : attrs) {
            if (k == "tint") {
                img->tint(parseColorOrToken(v));
            }
        }

        if (parent) {
            return parent->addChild(std::move(img));
        }
        return img.release();
    });

    // 9. TabBar
    registerWidget("TabBar", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "TabBar";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto tab = std::make_unique<TabBar>(name);

        auto itTabs = attrs.find("tabs");
        if (itTabs != attrs.end()) {
            std::string s = itTabs->second;
            std::replace(s.begin(), s.end(), ',', ' ');
            std::stringstream ss(s);
            std::string t;
            while (ss >> t) {
                tab->addTab(t);
            }
        }

        auto itSel = attrs.find("selectedTab");
        if (itSel != attrs.end()) {
            tab->selectTab(static_cast<size_t>(parseNumberOrToken(itSel->second, 0.0f)));
        }

        if (parent) {
            return parent->addChild(std::move(tab));
        }
        return tab.release();
    });

    // 10. ComboBox
    registerWidget("ComboBox", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "ComboBox";
        std::vector<std::string> options;
        size_t initialIdx = 0;

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto itOpts = attrs.find("options");
        if (itOpts != attrs.end()) {
            std::string s = itOpts->second;
            std::replace(s.begin(), s.end(), ',', ' ');
            std::stringstream ss(s);
            std::string opt;
            while (ss >> opt) {
                options.push_back(opt);
            }
        }

        auto itSel = attrs.find("selectedIndex");
        if (itSel != attrs.end()) {
            initialIdx = static_cast<size_t>(parseNumberOrToken(itSel->second, 0.0f));
        }

        auto cb = std::make_unique<ComboBox>(options, initialIdx, name);

        if (parent) {
            return parent->addChild(std::move(cb));
        }
        return cb.release();
    });

    // 11. TextInput
    registerWidget("TextInput", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string placeholder = "Search...";
        auto itPl = attrs.find("placeholder");
        if (itPl != attrs.end()) placeholder = itPl->second;

        auto ti = std::make_unique<TextInput>(placeholder);

        auto itName = attrs.find("name");
        if (itName != attrs.end()) ti->setName(itName->second);

        for (const auto& [k, v] : attrs) {
            if (k == "text") {
                ti->text(v);
            } else if (k == "fontSize" || k == "font-size") {
                ti->fontSize(parseNumberOrToken(v, 13.0f));
            }
        }

        if (parent) {
            return parent->addChild(std::move(ti));
        }
        return ti.release();
    });

    m_builtinsRegistered = true;
}

} // namespace PerfUI
