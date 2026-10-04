#include "PerfUI/Markup/MarkupLoader.h"
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

#include <pugixml.hpp>
#include <fstream>
#include <sstream>

namespace PerfUI {

using namespace MarkupInternal;

static int calculateLineNumber(std::string_view text, size_t byteOffset) {
    int line = 1;
    for (size_t i = 0; i < byteOffset && i < text.size(); ++i) {
        if (text[i] == '\n') ++line;
    }
    return line;
}

static bool isKnownWidgetAttribute(const std::string& tag, const std::string& key) {
    if (isCommonLayoutAttribute(key)) return true;

    if (tag == "Panel") {
        return key == "bg" || key == "background" || key == "border" || key == "border-width" || key == "radius" || key == "shadow";
    } else if (tag == "Text") {
        return key == "text" || key == "font" || key == "font-size" || key == "color" || key == "bold" || key == "italic" || key == "wrap";
    } else if (tag == "Button") {
        return key == "text" || key == "label" || key == "onClick" || key == "font" || key == "font-size" || key == "radius";
    } else if (tag == "ScrollView") {
        return key == "scroll" || key == "scrollSpeed" || key == "scroll-speed" || key == "showScrollbar" || key == "show-scrollbar" || key == "offset";
    } else if (tag == "Checkbox") {
        return key == "label" || key == "text" || key == "checked" || key == "boxSize" || key == "box-size" || key == "fontSize" || key == "font-size" || key == "checkColor" || key == "check-color" || key == "labelColor" || key == "label-color" || key == "onToggle";
    } else if (tag == "ProgressBar") {
        return key == "value" || key == "progress" || key == "min" || key == "max" || key == "showLabel" || key == "show-label" || key == "fillColor" || key == "fill-color" || key == "trackColor" || key == "track-color" || key == "height" || key == "radius";
    } else if (tag == "Slider") {
        return key == "value" || key == "min" || key == "max" || key == "step" || key == "label" || key == "fillColor" || key == "fill-color" || key == "trackColor" || key == "track-color" || key == "onValueChanged";
    } else if (tag == "Image") {
        return key == "texture" || key == "tint";
    } else if (tag == "TabBar") {
        return key == "tabs" || key == "selectedTab";
    } else if (tag == "ComboBox") {
        return key == "options" || key == "selectedIndex";
    } else if (tag == "TextInput") {
        return key == "placeholder" || key == "text" || key == "fontSize" || key == "font-size";
    }

    return false;
}

static UIElement* searchByName(UIElement* current, std::string_view name) {
    if (!current) return nullptr;
    if (current->name() == name) return current;
    for (const auto& child : current->children()) {
        if (auto* found = searchByName(child.get(), name)) {
            return found;
        }
    }
    return nullptr;
}

static void collectScrollOffsets(UIElement* root, std::unordered_map<std::string, float>& offsets) {
    if (!root) return;
    if (auto* sv = dynamic_cast<ScrollView*>(root)) {
        if (!sv->name().empty()) {
            offsets[sv->name()] = sv->scrollOffset();
        }
    }
    for (const auto& child : root->children()) {
        collectScrollOffsets(child.get(), offsets);
    }
}

static void collectActiveTabs(UIElement* root, std::unordered_map<std::string, size_t>& tabs) {
    if (!root) return;
    if (auto* tb = dynamic_cast<TabBar*>(root)) {
        if (!tb->name().empty()) {
            tabs[tb->name()] = tb->selectedTab();
        }
    }
    for (const auto& child : root->children()) {
        collectActiveTabs(child.get(), tabs);
    }
}

MarkupLoader::MarkupLoader(UIContext& ctx)
    : m_context(ctx)
{
    WidgetRegistry::instance().registerBuiltinWidgets();
}

MarkupLoader::~MarkupLoader() = default;

void MarkupLoader::bindCallback(std::string name, std::function<void()> cb) {
    m_callbacks[std::move(name)] = std::move(cb);
}

void MarkupLoader::enableHotReload(bool on) {
    m_hotReloadEnabled = on;
    if (on && !m_sourceFilePath.empty()) {
        std::error_code ec;
        m_lastWriteTime = std::filesystem::last_write_time(m_sourceFilePath, ec);
        m_lastCheckTime = std::chrono::steady_clock::now();
    }
}

UIElement* MarkupLoader::findByName(std::string_view name) const {
    if (m_root) {
        return searchByName(m_root, name);
    }
    return nullptr;
}

UIElement* MarkupLoader::loadFile(const std::filesystem::path& file, UIElement* parent) {
    m_sourceFilePath = file;
    m_parent = parent;

    std::error_code ec;
    m_lastWriteTime = std::filesystem::last_write_time(file, ec);
    m_lastCheckTime = std::chrono::steady_clock::now();

    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) {
        m_lastResult.success = false;
        m_lastResult.errors.push_back("Failed to open file: " + file.string());
        return nullptr;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return loadString(content, parent);
}

UIElement* MarkupLoader::loadString(std::string_view xml, UIElement* parent) {
    m_parent = parent;
    m_lastResult = LoadResult{};

    UIElement* newTree = parseAndBuildTree(xml, nullptr);
    if (!newTree) {
        return nullptr;
    }

    UIElement* targetParent = parent ? parent : m_context.root();
    if (targetParent) {
        m_root = targetParent->addChild(std::unique_ptr<UIElement>(newTree));
    } else {
        m_root = newTree;
    }

    return m_root;
}

void MarkupLoader::setupElementProperties(UIElement* element, const std::string& tag, const AttrMap& attrs, int lineNumber) {
    if (!element) return;

    for (const auto& [k, v] : attrs) {
        if (!isKnownWidgetAttribute(tag, k)) {
            m_lastResult.warnings.push_back("[Line " + std::to_string(lineNumber) + "] Widget '" + tag + "' ignoring unknown attribute '" + k + "'.");
        }
        applyCommonLayoutAttribute(element, k, v);
    }

    // Bind Button onClick callback
    if (tag == "Button") {
        auto itClick = attrs.find("onClick");
        if (itClick != attrs.end()) {
            const std::string& cbName = itClick->second;
            auto itCb = m_callbacks.find(cbName);
            auto* btn = dynamic_cast<Button*>(element);
            if (btn) {
                if (itCb != m_callbacks.end()) {
                    btn->onClick(itCb->second);
                } else {
                    m_lastResult.warnings.push_back("[Line " + std::to_string(lineNumber) + "] Button '" + element->name() + "' has unbound callback '" + cbName + "'.");
                    btn->onClick([]{});
                }
            }
        }
    }

    // Check bind attribute stub
    auto itBind = attrs.find("bind");
    if (itBind != attrs.end()) {
        m_lastResult.warnings.push_back("[Line " + std::to_string(lineNumber) + "] Data binding '" + itBind->second + "' not implemented (v1 stub).");
    }
}

UIElement* MarkupLoader::parseAndBuildTree(std::string_view xml, UIElement* parent) {
    pugi::xml_document doc;
    pugi::xml_parse_result parseResult = doc.load_buffer(xml.data(), xml.size());
    if (!parseResult) {
        int line = calculateLineNumber(xml, parseResult.offset);
        std::string err = "[Line " + std::to_string(line) + "] XML parse error: " + parseResult.description();
        m_lastResult.success = false;
        m_lastResult.errors.push_back(std::move(err));
        return nullptr;
    }

    pugi::xml_node rootNode = doc.first_child();
    if (!rootNode) {
        m_lastResult.success = false;
        m_lastResult.errors.push_back("XML document has no root node");
        return nullptr;
    }

    std::function<UIElement*(pugi::xml_node, UIElement*)> buildElement =
        [&](pugi::xml_node node, UIElement* currentParent) -> UIElement* {

        std::string tag = node.name();
        int line = calculateLineNumber(xml, node.offset_debug());

        if (tag == "UI") {
            UIElement* firstCreated = nullptr;
            for (pugi::xml_node child : node.children()) {
                UIElement* created = buildElement(child, currentParent);
                if (!firstCreated && created) {
                    firstCreated = created;
                }
            }
            return firstCreated;
        }

        if (!WidgetRegistry::instance().hasWidget(tag)) {
            m_lastResult.warnings.push_back("[Line " + std::to_string(line) + "] Unknown widget tag '" + tag + "' - skipping.");
            return nullptr;
        }

        AttrMap attrs;
        for (pugi::xml_attribute attr : node.attributes()) {
            attrs[attr.name()] = attr.value();
        }

        UIElement* element = WidgetRegistry::instance().create(tag, currentParent, attrs);
        if (!element) return nullptr;

        setupElementProperties(element, tag, attrs, line);

        for (pugi::xml_node child : node.children()) {
            buildElement(child, element);
        }

        return element;
    };

    UIElement* createdRoot = buildElement(rootNode, parent);
    return createdRoot;
}

void MarkupLoader::poll() {
    if (!m_hotReloadEnabled || m_sourceFilePath.empty()) return;

    auto now = std::chrono::steady_clock::now();
    if (m_pollInterval.count() > 0 && (now - m_lastCheckTime < m_pollInterval)) {
        return;
    }
    m_lastCheckTime = now;

    std::error_code ec;
    auto currentWriteTime = std::filesystem::last_write_time(m_sourceFilePath, ec);
    if (ec || currentWriteTime == m_lastWriteTime) {
        return;
    }

    std::ifstream ifs(m_sourceFilePath, std::ios::binary);
    if (!ifs) return;

    std::string fileContent((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    LoadResult prevResult = m_lastResult;
    m_lastResult = LoadResult{};

    pugi::xml_document doc;
    pugi::xml_parse_result parseRes = doc.load_buffer(fileContent.data(), fileContent.size());
    if (!parseRes) {
        int line = calculateLineNumber(fileContent, parseRes.offset);
        std::string err = "[Line " + std::to_string(line) + "] XML parse error: " + parseRes.description();
        m_lastResult.success = false;
        m_lastResult.errors.push_back(err);
        m_context.showToast("Hot-Reload Error", err, ToastType::Error, 4.0f);
        return;
    }

    std::string focusedName;
    if (m_context.focusedElement()) {
        focusedName = m_context.focusedElement()->name();
    }

    std::unordered_map<std::string, float> scrollOffsets;
    collectScrollOffsets(m_root, scrollOffsets);

    std::unordered_map<std::string, size_t> activeTabs;
    collectActiveTabs(m_root, activeTabs);

    UIElement* effectiveParent = m_parent ? m_parent : m_context.root();

    UIElement* newRoot = parseAndBuildTree(fileContent, nullptr);
    if (!newRoot) {
        std::string err = m_lastResult.errors.empty() ? "Failed to build markup hierarchy" : m_lastResult.errors[0];
        m_context.showToast("Hot-Reload Error", err, ToastType::Error, 4.0f);
        return;
    }

    if (m_root && effectiveParent) {
        effectiveParent->removeChild(m_root);
    }

    if (effectiveParent) {
        m_root = effectiveParent->addChild(std::unique_ptr<UIElement>(newRoot));
    } else {
        m_root = newRoot;
    }
    m_lastWriteTime = currentWriteTime;

    // Restore state
    for (const auto& [name, offset] : scrollOffsets) {
        if (auto* el = findByName(name)) {
            if (auto* sv = dynamic_cast<ScrollView*>(el)) {
                sv->scrollTo(offset);
            }
        }
    }

    for (const auto& [name, tabIdx] : activeTabs) {
        if (auto* el = findByName(name)) {
            if (auto* tb = dynamic_cast<TabBar*>(el)) {
                tb->selectTab(tabIdx);
            }
        }
    }

    if (!focusedName.empty()) {
        if (auto* el = findByName(focusedName)) {
            if (el->isFocusable()) {
                m_context.setFocus(el);
            }
        }
    }
}

} // namespace PerfUI
