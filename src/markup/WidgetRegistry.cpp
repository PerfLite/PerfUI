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
#include "PerfUI/MapCanvas.h"
#include "PerfUI/Divider.h"
#include "PerfUI/Spacer.h"
#include "PerfUI/Switch.h"
#include "PerfUI/RadioButton.h"
#include "PerfUI/RadioGroup.h"
#include "PerfUI/Label.h"
#include "PerfUI/ItemSlot.h"
#include "PerfUI/KeybindSelector.h"
#include "PerfUI/Accordion.h"
#include "PerfUI/SearchableList.h"
#include "PerfUI/TextArea.h"
#include "PerfUI/SegmentedControl.h"
#include "PerfUI/CategoryList.h"
#include "PerfUI/QuestTracker.h"
#include "PerfUI/Notification.h"
#include "PerfUI/Sidebar.h"
#include "PerfUI/SvgImage.h"
#include "PerfUI/WorldAnchor.h"
#include "PerfUI/UIContext.h"

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
            if (k == "bg" || k == "background" || k == "bg-color") {
                panel->backgroundColor(parseColorOrToken(v));
            } else if (k == "border" || k == "border-color") {
                panel->borderColor(parseColorOrToken(v));
            } else if (k == "border-width") {
                panel->borderWidth(parseNumberOrToken(v));
            } else if (k == "radius" || k == "corner-radius") {
                panel->cornerRadius(parseNumberOrToken(v));
            } else if (k == "shadow") {
                panel->shadow(parseBool(v, true));
            } else if (k == "clip-children" || k == "clipChildren") {
                panel->clipChildren(parseBool(v, true));
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
        if (itText != attrs.end()) textContent = resolveLocalizedText(itText->second);

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
        if (itText != attrs.end()) labelText = resolveLocalizedText(itText->second);
        else {
            auto itLabel = attrs.find("label");
            if (itLabel != attrs.end()) labelText = resolveLocalizedText(itLabel->second);
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
        if (itLabel != attrs.end()) labelText = resolveLocalizedText(itLabel->second);
        else {
            auto itText = attrs.find("text");
            if (itText != attrs.end()) labelText = resolveLocalizedText(itText->second);
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
            } else if (k == "text") {
                pb->text(v);
            } else if (k == "borderColor" || k == "border-color") {
                pb->borderColor(parseColorOrToken(v));
            } else if (k == "borderWidth" || k == "border-width") {
                pb->borderWidth(parseNumberOrToken(v, 1.0f));
            } else if (k == "fontSize" || k == "font-size") {
                pb->fontSize(parseNumberOrToken(v, 10.0f));
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

        auto itSrc = attrs.find("src");
        if (itSrc != attrs.end() && tex == 0 && parent && parent->context()) {
            tex = parent->context()->loadTexture(itSrc->second);
        }

        auto img = std::make_unique<Image>(tex, name);

        for (const auto& [k, v] : attrs) {
            if (k == "tint") {
                img->tint(parseColorOrToken(v));
            } else if (k == "flip-y" || k == "flipY") {
                img->flipY(parseBool(v, true));
            } else if (k == "uv") {
                std::string s = v;
                std::replace(s.begin(), s.end(), ',', ' ');
                std::stringstream ss(s);
                float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
                if (ss >> u0 >> v0 >> u1 >> v1) {
                    img->uv({ u0, v0 }, { u1, v1 });
                }
            }
        }

        if (parent) {
            return parent->addChild(std::move(img));
        }
        return img.release();
    });

    // 8b. Svg / SvgImage
    auto registerSvgWidget = [this](const char* tag) {
        registerWidget(tag, [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
            std::string name = "SvgImage";
            std::string src;
            std::string content;
            float tw = 0.0f;
            float th = 0.0f;

            auto itName = attrs.find("name");
            if (itName != attrs.end()) name = itName->second;
            auto itSrc = attrs.find("src");
            if (itSrc != attrs.end()) src = itSrc->second;
            else {
                auto itFile = attrs.find("file");
                if (itFile != attrs.end()) src = itFile->second;
            }
            auto itContent = attrs.find("content");
            if (itContent != attrs.end()) content = itContent->second;

            auto itW = attrs.find("width");
            if (itW != attrs.end()) tw = parseNumberOrToken(itW->second, 0.0f);
            auto itH = attrs.find("height");
            if (itH != attrs.end()) th = parseNumberOrToken(itH->second, 0.0f);

            auto svg = std::make_unique<SvgImage>(src, tw, th, name);
            if (!content.empty()) {
                svg->svgContent(content);
            }

            for (const auto& [k, v] : attrs) {
                if (k == "tint") {
                    svg->tint(parseColorOrToken(v));
                }
            }

            if (parent) {
                return parent->addChild(std::move(svg));
            }
            return svg.release();
        });
    };
    registerSvgWidget("Svg");
    registerSvgWidget("SvgImage");

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
        if (itPl != attrs.end()) placeholder = resolveLocalizedText(itPl->second);

        auto ti = std::make_unique<TextInput>(placeholder);

        auto itName = attrs.find("name");
        if (itName != attrs.end()) ti->setName(itName->second);

        for (const auto& [k, v] : attrs) {
            if (k == "text") {
                ti->text(resolveLocalizedText(v));
            } else if (k == "fontSize" || k == "font-size") {
                ti->fontSize(parseNumberOrToken(v, 13.0f));
            }
        }

        if (parent) {
            return parent->addChild(std::move(ti));
        }
        return ti.release();
    });

    // 12. MapCanvas. Data source (IMapDataSource) is assigned by the mod in C++;
    // the markup configures only visual map styling.
    registerWidget("MapCanvas", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "MapCanvas";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto map = std::make_unique<MapCanvas>(std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "mode") {
                map->mode(v == "world" ? MapMode::World : MapMode::Radar);
            } else if (k == "radar-range") {
                map->radarRange(parseNumberOrToken(v, 300.0f));
            } else if (k == "show-grid") {
                map->showGrid(parseBool(v, true));
            } else if (k == "show-distances") {
                map->showDistances(parseBool(v, true));
            } else if (k == "rotate-with-player") {
                map->rotateWithPlayer(parseBool(v, true));
            }
        }

        if (parent) {
            return parent->addChild(std::move(map));
        }
        return map.release();
    });

    // 13. Divider
    registerWidget("Divider", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Divider";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        LayoutDirection orientation = LayoutDirection::Horizontal;
        auto itOri = attrs.find("orientation");
        if (itOri != attrs.end()) {
            if (itOri->second == "vertical") orientation = LayoutDirection::Vertical;
        }

        auto div = std::make_unique<Divider>(orientation, std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "thickness") {
                div->thickness(parseNumberOrToken(v, 1.0f));
            } else if (k == "color") {
                div->color(parseColorOrToken(v));
            }
        }

        if (parent) {
            return parent->addChild(std::move(div));
        }
        return div.release();
    });

    // 14. Spacer
    registerWidget("Spacer", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Spacer";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        float size = 0.0f;
        float flex = 0.0f;
        auto itSize = attrs.find("size");
        if (itSize != attrs.end()) size = parseNumberOrToken(itSize->second, 0.0f);
        auto itFlex = attrs.find("flex");
        if (itFlex != attrs.end()) flex = parseNumberOrToken(itFlex->second, 0.0f);

        auto sp = std::make_unique<Spacer>(size, flex, std::move(name));

        if (parent) {
            return parent->addChild(std::move(sp));
        }
        return sp.release();
    });

    // 15. Switch
    registerWidget("Switch", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Switch";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        bool checked = false;
        auto itCh = attrs.find("checked");
        if (itCh != attrs.end()) checked = parseBool(itCh->second, false);

        auto sw = std::make_unique<Switch>(checked, std::move(name));

        if (parent) {
            return parent->addChild(std::move(sw));
        }
        return sw.release();
    });

    // 16. RadioButton
    registerWidget("RadioButton", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "RadioButton";
        std::string labelText = "Option";
        int val = 0;
        bool selected = false;

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itLabel = attrs.find("label");
        if (itLabel != attrs.end()) labelText = itLabel->second;
        auto itVal = attrs.find("value");
        if (itVal != attrs.end()) val = static_cast<int>(parseNumberOrToken(itVal->second, 0.0f));
        auto itSel = attrs.find("selected");
        if (itSel != attrs.end()) selected = parseBool(itSel->second, false);

        auto rb = std::make_unique<RadioButton>(val, labelText, std::move(name));
        rb->setSelected(selected);

        if (parent) {
            return parent->addChild(std::move(rb));
        }
        return rb.release();
    });

    // 17. RadioGroup
    registerWidget("RadioGroup", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "RadioGroup";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto rg = std::make_unique<RadioGroup>(std::move(name));
        auto itSel = attrs.find("selected");
        if (itSel != attrs.end()) {
            rg->select(static_cast<int>(parseNumberOrToken(itSel->second, 0.0f)));
        }

        if (parent) {
            return parent->addChild(std::move(rg));
        }
        return rg.release();
    });

    // 18. Label
    registerWidget("Label", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Label";
        std::string textContent = "";
        LabelVariant variant = LabelVariant::Body;

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itText = attrs.find("text");
        if (itText != attrs.end()) textContent = itText->second;

        auto itVar = attrs.find("variant");
        if (itVar != attrs.end()) {
            if (itVar->second == "heading" || itVar->second == "title") variant = LabelVariant::Heading;
            else if (itVar->second == "caption" || itVar->second == "small") variant = LabelVariant::Caption;
            else if (itVar->second == "accent") variant = LabelVariant::Accent;
        }

        auto lbl = std::make_unique<Label>(textContent, variant, std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "color") {
                lbl->color(parseColorOrToken(v));
            } else if (k == "font-size" || k == "fontSize") {
                lbl->fontSize(parseNumberOrToken(v, 14.0f));
            } else if (k == "bold") {
                lbl->bold(parseBool(v, true));
            }
        }

        if (parent) {
            return parent->addChild(std::move(lbl));
        }
        return lbl.release();
    });

    // 19. ItemSlot
    registerWidget("ItemSlot", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "ItemSlot";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto slot = std::make_unique<ItemSlot>(std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "count") {
                slot->count(static_cast<int>(parseNumberOrToken(v, 1.0f)));
            } else if (k == "rarity") {
                if (v == "fine" || v == "green") slot->rarity(ItemRarity::Fine);
                else if (v == "superior" || v == "blue") slot->rarity(ItemRarity::Superior);
                else if (v == "epic" || v == "purple") slot->rarity(ItemRarity::Epic);
                else if (v == "legendary" || v == "gold") slot->rarity(ItemRarity::Legendary);
                else slot->rarity(ItemRarity::Common);
            } else if (k == "equipped") {
                slot->equipped(parseBool(v, false));
            } else if (k == "selected") {
                slot->selected(parseBool(v, false));
            }
        }

        if (parent) {
            return parent->addChild(std::move(slot));
        }
        return slot.release();
    });

    // 20. KeybindSelector
    registerWidget("KeybindSelector", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "KeybindSelector";
        std::string labelText = "Keybind";
        uint32_t keyCode = 0;

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itLabel = attrs.find("label");
        if (itLabel != attrs.end()) labelText = itLabel->second;
        auto itKey = attrs.find("key");
        if (itKey != attrs.end()) keyCode = static_cast<uint32_t>(parseNumberOrToken(itKey->second, 0.0f));

        auto kb = std::make_unique<KeybindSelector>(keyCode, labelText, std::move(name));

        if (parent) {
            return parent->addChild(std::move(kb));
        }
        return kb.release();
    });

    // 21. Accordion
    registerWidget("Accordion", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Accordion";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto acc = std::make_unique<Accordion>(std::move(name));

        auto itEx = attrs.find("exclusive");
        if (itEx != attrs.end()) {
            acc->exclusive(parseBool(itEx->second, false));
        }

        if (parent) {
            return parent->addChild(std::move(acc));
        }
        return acc.release();
    });

    // 22. AccordionSection
    registerWidget("AccordionSection", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "AccordionSection";
        std::string title = "Section";
        bool expanded = false;

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itTitle = attrs.find("title");
        if (itTitle != attrs.end()) title = itTitle->second;
        auto itExp = attrs.find("expanded");
        if (itExp != attrs.end()) expanded = parseBool(itExp->second, false);

        auto sec = std::make_unique<AccordionSection>(title, nullptr, expanded, std::move(name));

        if (parent) {
            return parent->addChild(std::move(sec));
        }
        return sec.release();
    });

    // 23. SearchableList
    registerWidget("SearchableList", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "SearchableList";
        std::string placeholder = "Search...";

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itPl = attrs.find("placeholder");
        if (itPl != attrs.end()) placeholder = itPl->second;

        auto sl = std::make_unique<SearchableList>(placeholder, std::move(name));

        if (parent) {
            return parent->addChild(std::move(sl));
        }
        return sl.release();
    });

    // 24. TextArea
    registerWidget("TextArea", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "TextArea";
        std::string textContent = "";

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;
        auto itText = attrs.find("text");
        if (itText != attrs.end()) textContent = itText->second;

        auto ta = std::make_unique<TextArea>(textContent, std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "read-only" || k == "readOnly") {
                ta->readOnly(parseBool(v, true));
            } else if (k == "font-size" || k == "fontSize") {
                ta->fontSize(parseNumberOrToken(v, 14.0f));
            } else if (k == "color") {
                ta->textColor(parseColorOrToken(v));
            }
        }

        if (parent) {
            return parent->addChild(std::move(ta));
        }
        return ta.release();
    });

    // 25. SegmentedControl
    registerWidget("SegmentedControl", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "SegmentedControl";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto sc = std::make_unique<SegmentedControl>(std::move(name));

        auto itSel = attrs.find("selected");
        if (itSel != attrs.end()) {
            sc->setSelectedIndex(static_cast<size_t>(parseNumberOrToken(itSel->second, 0.0f)));
        }

        if (parent) {
            return parent->addChild(std::move(sc));
        }
        return sc.release();
    });

    // 26. CategoryList
    registerWidget("CategoryList", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "CategoryList";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto cl = std::make_unique<CategoryList>(std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "item-height" || k == "itemHeight") {
                cl->itemHeight(parseNumberOrToken(v, 36.0f));
            }
        }

        if (parent) {
            return parent->addChild(std::move(cl));
        }
        return cl.release();
    });

    // 27. QuestTracker
    registerWidget("QuestTracker", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "QuestTracker";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto qt = std::make_unique<QuestTracker>(std::move(name));

        auto itTitle = attrs.find("title");
        if (itTitle != attrs.end()) qt->setQuestTitle(itTitle->second);

        auto itCat = attrs.find("category");
        if (itCat != attrs.end()) qt->setQuestCategory(itCat->second);

        if (parent) {
            return parent->addChild(std::move(qt));
        }
        return qt.release();
    });

    // 28. Notification
    registerWidget("Notification", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Notification";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto notif = std::make_unique<Notification>(std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "title") {
                notif->setTitle(v);
            } else if (k == "message") {
                notif->setMessage(v);
            } else if (k == "level") {
                if (v == "success") notif->setLevel(NotificationLevel::Success);
                else if (v == "warning") notif->setLevel(NotificationLevel::Warning);
                else if (v == "error") notif->setLevel(NotificationLevel::Error);
                else notif->setLevel(NotificationLevel::Info);
            } else if (k == "duration") {
                notif->setDuration(parseNumberOrToken(v, 4.0f));
            } else if (k == "closable") {
                notif->setClosable(parseBool(v, true));
            }
        }

        if (parent) {
            return parent->addChild(std::move(notif));
        }
        return notif.release();
    });

    // 29. Sidebar
    registerWidget("Sidebar", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "Sidebar";
        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto sb = std::make_unique<Sidebar>(std::move(name));

        for (const auto& [k, v] : attrs) {
            if (k == "title") {
                sb->setTitle(v);
            } else if (k == "collapsed") {
                sb->setCollapsed(parseBool(v, false));
            } else if (k == "expanded-width" || k == "expandedWidth") {
                sb->expandedWidth(parseNumberOrToken(v, 220.0f));
            } else if (k == "collapsed-width" || k == "collapsedWidth") {
                sb->collapsedWidth(parseNumberOrToken(v, 52.0f));
            }
        }

        if (parent) {
            return parent->addChild(std::move(sb));
        }
        return sb.release();
    });

    // 30. WorldAnchor
    registerWidget("WorldAnchor", [](UIElement* parent, const AttrMap& attrs) -> UIElement* {
        std::string name = "WorldAnchor";
        uint32_t targetId = 0;

        auto itName = attrs.find("name");
        if (itName != attrs.end()) name = itName->second;

        auto itTarget = attrs.find("targetId");
        if (itTarget == attrs.end()) itTarget = attrs.find("target");
        if (itTarget != attrs.end()) {
            targetId = static_cast<uint32_t>(std::strtoul(itTarget->second.c_str(), nullptr, 0));
        }

        auto anchor = std::make_unique<WorldAnchor>(targetId);
        anchor->setName(name);

        float baseW = 100.0f, baseH = 40.0f;
        float minDist = 100.0f, maxDist = 3000.0f;
        float minScale = 0.5f, maxScale = 1.0f;
        bool distScaleEnabled = false;

        float minAlphaDist = 100.0f, maxAlphaDist = 3000.0f;
        float minAlpha = 0.0f, maxAlpha = 1.0f;
        bool distAlphaEnabled = false;

        bool edgeClamping = false;
        float screenMargin = 30.0f;

        for (const auto& [k, v] : attrs) {
            if (k == "align" || k == "alignment") {
                if (v == "center") anchor->setAnchorAlignment(AnchorAlignment::Center);
                else if (v == "top-center" || v == "topCenter") anchor->setAnchorAlignment(AnchorAlignment::TopCenter);
                else if (v == "top-left" || v == "topLeft") anchor->setAnchorAlignment(AnchorAlignment::TopLeft);
                else anchor->setAnchorAlignment(AnchorAlignment::BottomCenter);
            } else if (k == "offset" || k == "screen-offset" || k == "screenOffset") {
                std::string s = v;
                std::replace(s.begin(), s.end(), ',', ' ');
                std::stringstream ss(s);
                float ox = 0.0f, oy = 0.0f;
                if (ss >> ox >> oy) {
                    anchor->setScreenOffset({ ox, oy });
                }
            } else if (k == "width" || k == "base-width" || k == "baseWidth") {
                baseW = parseNumberOrToken(v, 100.0f);
            } else if (k == "height" || k == "base-height" || k == "baseHeight") {
                baseH = parseNumberOrToken(v, 40.0f);
            } else if (k == "distance-scaling" || k == "distanceScaling") {
                distScaleEnabled = parseBool(v, true);
            } else if (k == "min-dist" || k == "minDist") {
                minDist = parseNumberOrToken(v, 100.0f);
            } else if (k == "max-dist" || k == "maxDist") {
                maxDist = parseNumberOrToken(v, 3000.0f);
            } else if (k == "min-scale" || k == "minScale") {
                minScale = parseNumberOrToken(v, 0.5f);
            } else if (k == "max-scale" || k == "maxScale") {
                maxScale = parseNumberOrToken(v, 1.0f);
            } else if (k == "distance-alpha" || k == "distanceAlpha") {
                distAlphaEnabled = parseBool(v, true);
            } else if (k == "min-alpha" || k == "minAlpha") {
                minAlpha = parseNumberOrToken(v, 0.0f);
            } else if (k == "max-alpha" || k == "maxAlpha") {
                maxAlpha = parseNumberOrToken(v, 1.0f);
            } else if (k == "hide-behind-camera" || k == "hideBehindCamera") {
                anchor->setHideBehindCamera(parseBool(v, true));
            } else if (k == "hide-when-occluded" || k == "hideWhenOccluded") {
                anchor->setHideWhenOccluded(parseBool(v, true));
            } else if (k == "edge-clamping" || k == "edgeClamping") {
                edgeClamping = parseBool(v, true);
            } else if (k == "screen-margin" || k == "screenMargin") {
                screenMargin = parseNumberOrToken(v, 30.0f);
            }
        }

        anchor->setBaseSize({ baseW, baseH });
        if (distScaleEnabled) {
            anchor->setDistanceScaling(minDist, maxDist, minScale, maxScale, true);
        }
        if (distAlphaEnabled) {
            anchor->setDistanceAlpha(minAlphaDist, maxAlphaDist, minAlpha, maxAlpha, true);
        }
        if (edgeClamping) {
            anchor->setEdgeClamping(true, screenMargin);
        }

        if (parent) {
            return parent->addChild(std::move(anchor));
        }
        return anchor.release();
    });

    m_builtinsRegistered = true;
}
} // namespace PerfUI
