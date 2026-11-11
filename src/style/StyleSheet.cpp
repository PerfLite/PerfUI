#include "PerfUI/Style/StyleSheet.h"
#include "StyleParser.h"
#include "PerfUI/UIElement.h"
#include "MarkupLayoutParser.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace PerfUI {

using namespace MarkupInternal;

static std::string toLowerString(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return s;
}

void StyleSheet::clear() {
    m_rules.clear();
    m_variables.clear();
    m_warnings.clear();
    m_errors.clear();
    m_idIndex.clear();
    m_classIndex.clear();
    m_tagIndex.clear();
    m_universalIndex.clear();
}

bool StyleSheet::loadString(std::string_view pss, const std::string& sourceName) {
    StyleParser parser(pss, *this, sourceName);
    bool ok = parser.parse();
    rebuildIndex();
    return ok;
}

bool StyleSheet::loadFile(const std::filesystem::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        addError("Failed to open style file: " + path.string());
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    bool ok = loadString(content, path.string());
    if (ok) {
        if (std::find(m_loadedFiles.begin(), m_loadedFiles.end(), path) == m_loadedFiles.end()) {
            m_loadedFiles.push_back(path);
        }
    }
    return ok;
}

bool StyleSheet::reloadFiles() {
    auto files = m_loadedFiles;
    auto vars = m_variables;
    clear();
    m_variables = std::move(vars);
    bool allOk = true;
    for (const auto& f : files) {
        if (!loadFile(f)) {
            allOk = false;
        }
    }
    return allOk;
}

void StyleSheet::setVariable(std::string name, std::string value) {
    m_variables[std::move(name)] = std::move(value);
}

std::string StyleSheet::getVariable(const std::string& name) const {
    auto it = m_variables.find(name);
    return it != m_variables.end() ? it->second : "";
}

bool StyleSheet::hasVariable(const std::string& name) const {
    return m_variables.find(name) != m_variables.end();
}

std::string StyleSheet::resolveVariables(const std::string& input) const {
    std::string res = input;
    size_t start = 0;
    int depth = 0;

    while (depth < 10) {
        size_t varPos = res.find("var", start);
        if (varPos == std::string::npos) break;

        size_t lparen = varPos + 3;
        while (lparen < res.size() && std::isspace(static_cast<unsigned char>(res[lparen]))) {
            lparen++;
        }
        if (lparen >= res.size() || res[lparen] != '(') {
            start = varPos + 3;
            continue;
        }

        depth++;
        size_t rparen = res.find(')', lparen + 1);
        if (rparen == std::string::npos) break;

        std::string inner = res.substr(lparen + 1, rparen - (lparen + 1));
        std::string varName = inner;
        std::string fallback = "";

        size_t comma = inner.find(',');
        if (comma != std::string::npos) {
            varName = inner.substr(0, comma);
            fallback = inner.substr(comma + 1);
            while (!fallback.empty() && std::isspace(static_cast<unsigned char>(fallback.front()))) fallback.erase(0, 1);
            while (!fallback.empty() && std::isspace(static_cast<unsigned char>(fallback.back()))) fallback.pop_back();
        }
        while (!varName.empty() && std::isspace(static_cast<unsigned char>(varName.front()))) varName.erase(0, 1);
        while (!varName.empty() && std::isspace(static_cast<unsigned char>(varName.back()))) varName.pop_back();

        std::string replacement = fallback;
        if (hasVariable(varName)) {
            replacement = getVariable(varName);
        }

        res.replace(varPos, rparen - varPos + 1, replacement);
        start = varPos + replacement.size();
    }

    return res;
}

void StyleSheet::addRule(StyleRule rule) {
    m_rules.push_back(std::move(rule));
}

void StyleSheet::rebuildIndex() {
    m_idIndex.clear();
    m_classIndex.clear();
    m_tagIndex.clear();
    m_universalIndex.clear();

    for (size_t i = 0; i < m_rules.size(); ++i) {
        const auto& rule = m_rules[i];
        if (rule.selector.components.empty()) continue;

        const auto& target = rule.selector.components.back().selector;
        if (!target.id.empty()) {
            m_idIndex[target.id].push_back(i);
        } else if (!target.classes.empty()) {
            for (const auto& c : target.classes) {
                m_classIndex[c].push_back(i);
            }
        } else if (!target.tag.empty() && target.tag != "*") {
            m_tagIndex[toLowerString(target.tag)].push_back(i);
        } else {
            m_universalIndex.push_back(i);
        }
    }
}

static bool matchSimpleSelector(const SimpleSelector& sel, const UIElement* el, uint8_t activePseudos) {
    if (!el) return false;

    // 1. Tag name
    if (!sel.tag.empty() && sel.tag != "*") {
        if (toLowerString(sel.tag) != toLowerString(el->typeName())) {
            return false;
        }
    }

    // 2. ID
    if (!sel.id.empty()) {
        if (el->name() != sel.id) {
            return false;
        }
    }

    // 3. Classes
    for (const auto& c : sel.classes) {
        if (!el->hasClass(c)) {
            return false;
        }
    }

    // 4. Pseudo-classes
    if (sel.pseudoClasses != 0) {
        if ((activePseudos & sel.pseudoClasses) != sel.pseudoClasses) {
            return false;
        }
    }

    return true;
}

static bool matchComplexSelector(const ComplexSelector& sel, const UIElement* element, uint8_t activePseudos) {
    if (sel.components.empty() || !element) return false;

    // Check target component (last component in sequence)
    const auto& targetComp = sel.components.back();
    if (!matchSimpleSelector(targetComp.selector, element, activePseudos)) {
        return false;
    }

    if (sel.components.size() == 1) {
        return true;
    }

    // Match preceding components right-to-left
    const UIElement* current = element;
    for (int i = static_cast<int>(sel.components.size()) - 2; i >= 0; --i) {
        Combinator comb = sel.components[i + 1].combinator;
        const auto& prevSel = sel.components[i].selector;

        if (comb == Combinator::Child) {
            current = current->parent();
            if (!current) return false;
            // For ancestors, activePseudos doesn't apply to hover/active unless ancestor is also hovered/active
            if (!matchSimpleSelector(prevSel, current, current->activePseudoClasses())) {
                return false;
            }
        } else {
            // Descendant
            bool found = false;
            current = current->parent();
            while (current) {
                if (matchSimpleSelector(prevSel, current, current->activePseudoClasses())) {
                    found = true;
                    break;
                }
                current = current->parent();
            }
            if (!found) return false;
        }
    }

    return true;
}

std::vector<const StyleRule*> StyleSheet::matchRules(const UIElement* element, uint8_t activePseudos) const {
    std::vector<const StyleRule*> matched;
    if (!element) return matched;

    std::unordered_set<size_t> candidateIndices;

    // 1. Universal rules
    for (size_t idx : m_universalIndex) candidateIndices.insert(idx);

    // 2. Tag rules
    std::string tagLower = toLowerString(element->typeName());
    if (auto it = m_tagIndex.find(tagLower); it != m_tagIndex.end()) {
        for (size_t idx : it->second) candidateIndices.insert(idx);
    }

    // 3. Class rules
    for (const auto& c : element->classes()) {
        if (auto it = m_classIndex.find(c); it != m_classIndex.end()) {
            for (size_t idx : it->second) candidateIndices.insert(idx);
        }
    }

    // 4. ID rules
    if (!element->name().empty()) {
        if (auto it = m_idIndex.find(element->name()); it != m_idIndex.end()) {
            for (size_t idx : it->second) candidateIndices.insert(idx);
        }
    }

    // Test candidate rules
    for (size_t idx : candidateIndices) {
        const auto& rule = m_rules[idx];
        if (matchComplexSelector(rule.selector, element, activePseudos)) {
            matched.push_back(&rule);
        }
    }

    // Sort by specificity and source order
    std::sort(matched.begin(), matched.end(), [](const StyleRule* a, const StyleRule* b) {
        if (a->selector.specificity != b->selector.specificity) {
            return a->selector.specificity < b->selector.specificity;
        }
        return a->sourceOrder < b->sourceOrder;
    });

    return matched;
}

static void applyPropertyToComputed(const std::string& key, const std::string& val, ComputedStyle& c) {
    std::string lower = toLowerString(key);

    if (lower == "opacity") {
        c.opacity = parseNumberOrToken(val, 1.0f);
    } else if (lower == "blend-mode" || lower == "blendmode" || lower == "mix-blend-mode") {
        if (val == "additive" || val == "add") c.blendMode = BlendMode::Additive;
        else if (val == "multiply" || val == "mult") c.blendMode = BlendMode::Multiply;
        else if (val == "screen") c.blendMode = BlendMode::Screen;
        else c.blendMode = BlendMode::Normal;
    } else if (lower == "background" || lower == "background-color" || lower == "bg" || lower == "bg-color") {
        c.backgroundColor = parseColorOrToken(val);
    } else if (lower == "border") {
        std::istringstream iss(val);
        std::string token1, token2, token3;
        iss >> token1;
        if (!token1.empty()) {
            char* end = nullptr;
            float num = std::strtof(token1.c_str(), &end);
            if (end != token1.c_str() && (*end == '\0' || *end == 'p' || *end == 'P')) {
                c.borderWidth = num;
                iss >> token2;
                if (!token2.empty()) {
                    if (token2 == "solid" || token2 == "none") {
                        iss >> token3;
                        if (!token3.empty()) {
                            c.borderColor = parseColorOrToken(token3);
                        }
                    } else {
                        c.borderColor = parseColorOrToken(token2);
                    }
                }
            } else {
                c.borderColor = parseColorOrToken(token1);
            }
        }
    } else if (lower == "border-color") {
        c.borderColor = parseColorOrToken(val);
    } else if (lower == "border-width") {
        c.borderWidth = parseNumberOrToken(val);
    } else if (lower == "border-radius" || lower == "radius" || lower == "corner-radius") {
        c.cornerRadius = parseNumberOrToken(val);
    } else if (lower == "shadow" || lower == "box-shadow") {
        c.shadow = parseBool(val, true);
    } else if (lower == "color") {
        c.color = parseColorOrToken(val);
    } else if (lower == "font") {
        c.font = val;
    } else if (lower == "font-size" || lower == "fontsize") {
        c.fontSize = parseNumberOrToken(val);
    } else if (lower == "bold" || lower == "font-weight") {
        c.bold = (val == "bold" || val == "700" || parseBool(val, false));
    } else if (lower == "italic" || lower == "font-style") {
        c.italic = (val == "italic" || parseBool(val, false));
    } else if (lower == "wrap") {
        c.wrap = parseNumberOrToken(val);
    } else if (lower == "width") {
        DimensionConstraint d = parseDimension(val);
        if (d.mode == SizeMode::Fixed) c.width = d.value;
    } else if (lower == "height") {
        DimensionConstraint d = parseDimension(val);
        if (d.mode == SizeMode::Fixed) c.height = d.value;
    } else if (lower == "min-width") {
        c.minWidth = parseNumberOrToken(val);
    } else if (lower == "max-width") {
        c.maxWidth = parseNumberOrToken(val);
    } else if (lower == "min-height") {
        c.minHeight = parseNumberOrToken(val);
    } else if (lower == "max-height") {
        c.maxHeight = parseNumberOrToken(val);
    } else if (lower == "flex") {
        c.flex = parseNumberOrToken(val, 1.0f);
    } else if (lower == "direction" || lower == "flex-direction") {
        std::string d = toLowerString(val);
        if (d == "row" || d == "horizontal") c.direction = LayoutDirection::Horizontal;
        else if (d == "column" || d == "vertical") c.direction = LayoutDirection::Vertical;
    } else if (lower == "align" || lower == "align-items") {
        std::string a = toLowerString(val);
        if (a == "start") c.alignment = Alignment::Start;
        else if (a == "center") c.alignment = Alignment::Center;
        else if (a == "end") c.alignment = Alignment::End;
        else if (a == "stretch") c.alignment = Alignment::Stretch;
    } else if (lower == "justify" || lower == "justify-content") {
        std::string j = toLowerString(val);
        if (j == "start") c.justify = JustifyContent::Start;
        else if (j == "center") c.justify = JustifyContent::Center;
        else if (j == "end") c.justify = JustifyContent::End;
        else if (j == "space-between") c.justify = JustifyContent::SpaceBetween;
        else if (j == "space-around") c.justify = JustifyContent::SpaceAround;
    } else if (lower == "gap") {
        c.gap = parseNumberOrToken(val);
    } else if (lower == "padding") {
        LayoutProps temp;
        parsePaddingOrMargin(val, temp, true);
        c.padding = temp.padding();
    } else if (lower == "margin") {
        LayoutProps temp;
        parsePaddingOrMargin(val, temp, false);
        c.margin = temp.margin();
    } else if (lower == "position") {
        std::string p = toLowerString(val);
        c.isAbsolute = (p == "absolute");
    } else if (lower == "left") {
        c.left = parseNumberOrToken(val);
    } else if (lower == "right") {
        c.right = parseNumberOrToken(val);
    } else if (lower == "top") {
        c.top = parseNumberOrToken(val);
    } else if (lower == "bottom") {
        c.bottom = parseNumberOrToken(val);
    } else if (lower == "center-x") {
        c.centerX = parseBool(val, true);
    } else if (lower == "visible" || lower == "display") {
        if (lower == "display") {
            c.visible = (val != "none");
        } else {
            c.visible = parseBool(val, true);
        }
    } else if (lower == "transform-offset" || lower == "transformoffset") {
        Point px{ 0.0f, 0.0f }, pct{ 0.0f, 0.0f };
        bool hasPct = false;
        if (parseTransformOffset(val, px, pct, hasPct)) {
            c.transformOffsetPx = px;
            c.transformOffsetPct = pct;
            c.hasTransformOffsetPct = hasPct;
        }
    } else if (lower == "transform-scale" || lower == "transformscale") {
        Point scale{ 1.0f, 1.0f };
        if (parseTransformScale(val, scale)) {
            c.transformScale = scale;
        }
    } else if (lower == "transform-pivot" || lower == "transformpivot") {
        Point piv{ 0.5f, 0.5f };
        if (parseTransformPivot(val, piv)) {
            c.transformPivot = piv;
        }
    } else if (lower == "tint") {
        c.tint = parseColorOrToken(val);
    }
}

ComputedStyle StyleSheet::computeStyle(const UIElement* element, const ComputedStyle* parentStyle) const {
    ComputedStyle computed;

    // 1. Inherit properties from parent style (Text properties)
    if (parentStyle) {
        if (parentStyle->color.has_value()) computed.color = parentStyle->color;
        if (parentStyle->font.has_value()) computed.font = parentStyle->font;
        if (parentStyle->fontSize.has_value()) computed.fontSize = parentStyle->fontSize;
        if (parentStyle->bold.has_value()) computed.bold = parentStyle->bold;
        if (parentStyle->italic.has_value()) computed.italic = parentStyle->italic;
    }

    if (!element) return computed;

    // 2. Apply PSS rules in specificity order
    auto matched = matchRules(element, element->activePseudoClasses());
    for (const auto* rule : matched) {
        for (const auto& [k, v] : rule->declaration.properties) {
            std::string resolved = resolveVariables(v);
            applyPropertyToComputed(k, resolved, computed);
        }
    }

    // 3. Apply inline XML attributes (Inline attributes beat PSS rules)
    for (const auto& [k, v] : element->inlineAttrs()) {
        std::string resolved = resolveVariables(v);
        applyPropertyToComputed(k, resolved, computed);
    }

    return computed;
}

} // namespace PerfUI
