#include "StyleParser.h"
#include "../markup/MarkupLayoutParser.h"
#include <algorithm>
#include <sstream>

namespace PerfUI {

using namespace MarkupInternal;

static std::string trimString(std::string_view sv) {
    size_t first = 0;
    while (first < sv.size() && std::isspace(static_cast<unsigned char>(sv[first]))) ++first;
    size_t last = sv.size();
    while (last > first && std::isspace(static_cast<unsigned char>(sv[last - 1]))) --last;
    return std::string(sv.substr(first, last - first));
}

static std::string toLower(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return s;
}

static bool parseEasingToken(std::string_view tok, EasingDef& outEasing) {
    std::string s = toLower(tok);
    if (s.rfind("cubic-bezier(", 0) == 0 && s.back() == ')') {
        std::string inner = s.substr(13, s.size() - 14);
        std::vector<std::string> parts;
        std::stringstream ss(inner);
        std::string item;
        while (std::getline(ss, item, ',')) parts.push_back(item);
        if (parts.size() == 4) {
            float x1 = std::strtof(parts[0].c_str(), nullptr);
            float y1 = std::strtof(parts[1].c_str(), nullptr);
            float x2 = std::strtof(parts[2].c_str(), nullptr);
            float y2 = std::strtof(parts[3].c_str(), nullptr);
            outEasing = EasingDef::Bezier(x1, y1, x2, y2);
            return true;
        }
    }
    if (s == "linear") { outEasing = Easing::Linear; return true; }
    if (s == "ease") { outEasing = EasingDef::Bezier(0.25f, 0.1f, 0.25f, 1.0f); return true; }
    if (s == "ease-in") { outEasing = EasingDef::Bezier(0.42f, 0.0f, 1.0f, 1.0f); return true; }
    if (s == "ease-out") { outEasing = EasingDef::Bezier(0.0f, 0.0f, 0.58f, 1.0f); return true; }
    if (s == "ease-in-out") { outEasing = EasingDef::Bezier(0.42f, 0.0f, 0.58f, 1.0f); return true; }
    if (s == "ease-in-quad") { outEasing = Easing::EaseInQuad; return true; }
    if (s == "ease-out-quad") { outEasing = Easing::EaseOutQuad; return true; }
    if (s == "ease-in-out-quad") { outEasing = Easing::EaseInOutQuad; return true; }
    if (s == "ease-in-cubic") { outEasing = Easing::EaseInCubic; return true; }
    if (s == "ease-out-cubic") { outEasing = Easing::EaseOutCubic; return true; }
    if (s == "ease-in-out-cubic") { outEasing = Easing::EaseInOutCubic; return true; }
    if (s == "ease-out-bounce") { outEasing = Easing::EaseOutBounce; return true; }
    if (s == "ease-out-elastic") { outEasing = Easing::EaseOutElastic; return true; }
    return false;
}

StyleParser::StyleParser(std::string_view source, StyleSheet& sheet, std::string sourceName)
    : m_tokenizer(source, std::move(sourceName)), m_sheet(sheet) {
    m_current = m_tokenizer.nextToken();
}

bool StyleParser::parse() {
    while (m_current.type != TokenType::EndOfFile) {
        parseRuleOrRoot();
    }
    return m_sheet.errors().empty();
}

Token StyleParser::peek() {
    return m_current;
}

Token StyleParser::advance() {
    Token prev = m_current;
    m_current = m_tokenizer.nextToken();
    return prev;
}

bool StyleParser::match(TokenType type) {
    if (m_current.type == type) {
        advance();
        return true;
    }
    return false;
}

bool StyleParser::expect(TokenType type, const char* expected) {
    if (m_current.type == type) {
        advance();
        return true;
    }
    error(std::string("Expected ") + expected + " but got '" + m_current.text + "'", m_current.line, m_current.col);
    return false;
}

void StyleParser::warning(const std::string& msg, int line, int col) {
    if (line == 0) line = m_current.line;
    if (col == 0) col = m_current.col;
    std::string w = m_tokenizer.sourceName() + ":" + std::to_string(line) + ":" + std::to_string(col) + ": " + msg;
    m_sheet.addWarning(std::move(w));
}

void StyleParser::error(const std::string& msg, int line, int col) {
    if (line == 0) line = m_current.line;
    if (col == 0) col = m_current.col;
    std::string e = m_tokenizer.sourceName() + ":" + std::to_string(line) + ":" + std::to_string(col) + ": " + msg;
    m_sheet.addError(std::move(e));
}

void StyleParser::skipUntilNextRule() {
    while (m_current.type != TokenType::EndOfFile && m_current.type != TokenType::RBrace && m_current.type != TokenType::LBrace) {
        advance();
    }
    if (m_current.type == TokenType::RBrace || m_current.type == TokenType::LBrace) {
        advance();
    }
}

void StyleParser::parseRuleOrRoot() {
    if (m_current.type == TokenType::Root) {
        advance();
        parseRootBlock();
        return;
    }
    if (m_current.type == TokenType::Keyframes) {
        advance();
        parseKeyframesBlock();
        return;
    }

    std::vector<ComplexSelector> selectors;
    if (!parseSelectorList(selectors)) {
        skipUntilNextRule();
        return;
    }

    StyleDeclaration decl;
    if (!parseDeclarationBlock(decl)) {
        skipUntilNextRule();
        return;
    }

    for (auto& sel : selectors) {
        m_sheet.addRule(StyleRule{ sel, decl, ++m_orderCounter });
    }
}

void StyleParser::parseKeyframesBlock() {
    if (m_current.type != TokenType::Ident && m_current.type != TokenType::String) {
        error("Expected keyframes name");
        skipUntilNextRule();
        return;
    }

    std::string name = advance().text;
    if (name.size() >= 2 && ((name.front() == '"' && name.back() == '"') || (name.front() == '\'' && name.back() == '\''))) {
        name = name.substr(1, name.size() - 2);
    }

    if (!expect(TokenType::LBrace, "{")) {
        skipUntilNextRule();
        return;
    }

    KeyframesDefinition kfDef;
    kfDef.name = name;

    while (m_current.type != TokenType::EndOfFile && m_current.type != TokenType::RBrace) {
        std::vector<float> offsets;
        while (m_current.type != TokenType::LBrace && m_current.type != TokenType::EndOfFile) {
            if (m_current.text == "from") {
                offsets.push_back(0.0f);
                advance();
            } else if (m_current.text == "to") {
                offsets.push_back(1.0f);
                advance();
            } else if (m_current.type == TokenType::Number) {
                std::string txt = advance().text;
                float pct = 0.0f;
                if (!txt.empty() && txt.back() == '%') {
                    pct = std::strtof(txt.substr(0, txt.size() - 1).c_str(), nullptr) / 100.0f;
                } else {
                    pct = std::strtof(txt.c_str(), nullptr);
                    if (pct > 1.0f) pct /= 100.0f;
                }
                offsets.push_back((std::clamp)(pct, 0.0f, 1.0f));
            } else {
                error("Expected keyframe offset (e.g. 0%, 50%, from, to)");
                skipUntilNextRule();
                return;
            }
            match(TokenType::Comma);
        }

        StyleDeclaration decl;
        if (!parseDeclarationBlock(decl)) {
            skipUntilNextRule();
            return;
        }

        for (float off : offsets) {
            KeyframeStep step;
            step.offset = off;

            for (const auto& [rawKey, rawVal] : decl.properties) {
                std::string k = toLower(rawKey);
                std::string v = m_sheet.resolveVariables(rawVal);

                if (k == "opacity" || k == "alpha") {
                    step.opacity = parseNumberOrToken(v);
                } else if (k == "transform-scale" || k == "transformscale" || k == "scale") {
                    Point sc{ 1.0f, 1.0f };
                    if (parseTransformScale(v, sc)) step.transformScale = sc;
                } else if (k == "transform-scale-x" || k == "scale-x") {
                    Point sc = step.transformScale.value_or(Point{ 1.0f, 1.0f });
                    sc.x = parseNumberOrToken(v, 1.0f);
                    step.transformScale = sc;
                } else if (k == "transform-scale-y" || k == "scale-y") {
                    Point sc = step.transformScale.value_or(Point{ 1.0f, 1.0f });
                    sc.y = parseNumberOrToken(v, 1.0f);
                    step.transformScale = sc;
                } else if (k == "transform-offset" || k == "transformoffset" || k == "offset") {
                    Point px{ 0.0f, 0.0f }, pct{ 0.0f, 0.0f };
                    bool hasPct = false;
                    if (parseTransformOffset(v, px, pct, hasPct)) {
                        step.transformOffsetPx = px;
                        if (hasPct) step.transformOffsetPct = pct;
                    }
                } else if (k == "transform-offset-x" || k == "offset-x") {
                    Point cur = step.transformOffsetPx.value_or(Point{ 0.0f, 0.0f });
                    cur.x = parseNumberOrToken(v);
                    step.transformOffsetPx = cur;
                } else if (k == "transform-offset-y" || k == "offset-y") {
                    Point cur = step.transformOffsetPx.value_or(Point{ 0.0f, 0.0f });
                    cur.y = parseNumberOrToken(v);
                    step.transformOffsetPx = cur;
                } else if (k == "background" || k == "background-color" || k == "bg" || k == "bg-color") {
                    step.backgroundColor = parseColorOrToken(v);
                } else if (k == "border-color") {
                    step.borderColor = parseColorOrToken(v);
                } else if (k == "border-width") {
                    step.borderWidth = parseNumberOrToken(v);
                } else if (k == "border-radius" || k == "radius" || k == "corner-radius") {
                    step.cornerRadius = parseNumberOrToken(v);
                } else if (k == "outline-color") {
                    step.outlineColor = parseColorOrToken(v);
                } else if (k == "outline-width") {
                    step.outlineWidth = parseNumberOrToken(v);
                } else if (k == "outline-offset") {
                    step.outlineOffset = parseNumberOrToken(v);
                } else if (k == "color" || k == "text-color") {
                    step.color = parseColorOrToken(v);
                } else if (k == "width") {
                    step.width = parseNumberOrToken(v);
                } else if (k == "height") {
                    step.height = parseNumberOrToken(v);
                } else if (k == "animation-timing-function" || k == "animation-timing") {
                    EasingDef edef{ Easing::Linear };
                    if (parseEasingToken(v, edef)) step.timingFunction = edef;
                } else {
                    auto locIt = decl.propertyLocations.find(rawKey);
                    int l = (locIt != decl.propertyLocations.end()) ? locIt->second.first : 0;
                    int c = (locIt != decl.propertyLocations.end()) ? locIt->second.second : 0;
                    warning("Property '" + rawKey + "' in @keyframes is not animatable", l, c);
                }
            }

            kfDef.steps.push_back(std::move(step));
        }
    }

    expect(TokenType::RBrace, "}");
    std::stable_sort(kfDef.steps.begin(), kfDef.steps.end(), [](const KeyframeStep& a, const KeyframeStep& b) {
        return a.offset < b.offset;
    });

    std::vector<KeyframeStep> mergedSteps;
    mergedSteps.reserve(kfDef.steps.size());

    for (auto& step : kfDef.steps) {
        if (!mergedSteps.empty() && std::abs(mergedSteps.back().offset - step.offset) < 0.0001f) {
            auto& prev = mergedSteps.back();
            if (step.opacity.has_value()) prev.opacity = step.opacity;
            if (step.transformScale.has_value()) prev.transformScale = step.transformScale;
            if (step.transformOffsetPx.has_value()) prev.transformOffsetPx = step.transformOffsetPx;
            if (step.transformOffsetPct.has_value()) prev.transformOffsetPct = step.transformOffsetPct;
            if (step.backgroundColor.has_value()) prev.backgroundColor = step.backgroundColor;
            if (step.borderColor.has_value()) prev.borderColor = step.borderColor;
            if (step.borderWidth.has_value()) prev.borderWidth = step.borderWidth;
            if (step.cornerRadius.has_value()) prev.cornerRadius = step.cornerRadius;
            if (step.outlineColor.has_value()) prev.outlineColor = step.outlineColor;
            if (step.outlineWidth.has_value()) prev.outlineWidth = step.outlineWidth;
            if (step.outlineOffset.has_value()) prev.outlineOffset = step.outlineOffset;
            if (step.color.has_value()) prev.color = step.color;
            if (step.width.has_value()) prev.width = step.width;
            if (step.height.has_value()) prev.height = step.height;
            if (step.timingFunction.has_value()) prev.timingFunction = step.timingFunction;
        } else {
            mergedSteps.push_back(std::move(step));
        }
    }
    kfDef.steps = std::move(mergedSteps);

    m_sheet.addKeyframes(std::move(kfDef));
}

void StyleParser::parseRootBlock() {
    if (!expect(TokenType::LBrace, "{")) {
        skipUntilNextRule();
        return;
    }

    while (m_current.type != TokenType::EndOfFile && m_current.type != TokenType::RBrace) {
        if (m_current.type == TokenType::Semicolon) {
            advance();
            continue;
        }

        if (m_current.type != TokenType::Ident) {
            warning("Expected variable name inside :root, got '" + m_current.text + "'");
            advance();
            continue;
        }

        std::string varName = m_current.text;
        advance();

        if (!expect(TokenType::Colon, ":")) {
            skipUntilNextRule();
            return;
        }

        std::string varValue;
        int parenDepth = 0;
        while (m_current.type != TokenType::EndOfFile) {
            if (m_current.type == TokenType::LParen) {
                parenDepth++;
            } else if (m_current.type == TokenType::RParen) {
                if (parenDepth > 0) parenDepth--;
            }

            if (parenDepth == 0 && (m_current.type == TokenType::Semicolon || m_current.type == TokenType::RBrace)) {
                break;
            }

            if (!varValue.empty() && m_current.type != TokenType::LParen && m_current.type != TokenType::RParen && m_current.type != TokenType::Comma && varValue.back() != '(') {
                varValue += " ";
            }
            varValue += m_current.text;
            advance();
        }

        if (m_current.type == TokenType::Semicolon) {
            advance();
        }

        varValue = trimString(varValue);
        if (!varName.empty() && !varValue.empty()) {
            m_sheet.setVariable(varName, varValue);
        }
    }

    expect(TokenType::RBrace, "}");
}

bool StyleParser::parseSelectorList(std::vector<ComplexSelector>& outSelectors) {
    while (m_current.type != TokenType::EndOfFile && m_current.type != TokenType::LBrace) {
        ComplexSelector sel;
        if (!parseComplexSelector(sel)) {
            return false;
        }
        outSelectors.push_back(std::move(sel));

        if (m_current.type == TokenType::Comma) {
            advance(); // consume ','
            continue;
        }
        break;
    }
    return !outSelectors.empty();
}

bool StyleParser::parseComplexSelector(ComplexSelector& outSelector) {
    Combinator pendingCombinator = Combinator::None;

    while (m_current.type != TokenType::EndOfFile && m_current.type != TokenType::LBrace && m_current.type != TokenType::Comma) {
        if (m_current.type == TokenType::GreaterThan) {
            advance();
            pendingCombinator = Combinator::Child;
            continue;
        }

        SimpleSelector simple;
        bool hasComponent = false;

        // Tag name or Universal *
        if (m_current.type == TokenType::Star) {
            simple.tag = "*";
            hasComponent = true;
            advance();
        } else if (m_current.type == TokenType::Ident && m_current.text[0] != '-' && m_current.text[0] != '.') {
            simple.tag = m_current.text;
            outSelector.specificity.tagCount++;
            hasComponent = true;
            advance();
        }

        // Classes, IDs, Pseudo-classes chained without spaces
        while (m_current.type == TokenType::Dot || m_current.type == TokenType::Hash || m_current.type == TokenType::Colon) {
            if (hasComponent && m_current.precededByWhitespace) {
                break;
            }

            if (m_current.type == TokenType::Dot) {
                advance();
                if (m_current.type == TokenType::Ident) {
                    simple.classes.push_back(m_current.text);
                    outSelector.specificity.classAndPseudoCount++;
                    hasComponent = true;
                    advance();
                } else {
                    error("Expected class name after '.'");
                    return false;
                }
            } else if (m_current.type == TokenType::Hash) {
                // Remove '#' prefix from token text if present
                std::string idText = m_current.text;
                if (!idText.empty() && idText[0] == '#') idText = idText.substr(1);
                simple.id = idText;
                outSelector.specificity.idCount++;
                hasComponent = true;
                advance();
            } else if (m_current.type == TokenType::Colon) {
                advance();
                if (m_current.type == TokenType::Ident) {
                    std::string pseudo = m_current.text;
                    advance();
                    outSelector.specificity.classAndPseudoCount++;
                    hasComponent = true;

                    if (pseudo == "hover") simple.pseudoClasses |= static_cast<uint8_t>(PseudoClass::Hover);
                    else if (pseudo == "active") simple.pseudoClasses |= static_cast<uint8_t>(PseudoClass::Active);
                    else if (pseudo == "focus") simple.pseudoClasses |= static_cast<uint8_t>(PseudoClass::Focus);
                    else if (pseudo == "disabled") simple.pseudoClasses |= static_cast<uint8_t>(PseudoClass::Disabled);
                    else if (pseudo == "checked") simple.pseudoClasses |= static_cast<uint8_t>(PseudoClass::Checked);
                    else if (pseudo == "first-child") simple.pseudoClasses |= static_cast<uint8_t>(PseudoClass::FirstChild);
                    else if (pseudo == "last-child") simple.pseudoClasses |= static_cast<uint8_t>(PseudoClass::LastChild);
                    else warning("Unknown pseudo-class :" + pseudo);
                } else {
                    error("Expected pseudo-class name after ':'");
                    return false;
                }
            }
        }

        if (!hasComponent) {
            break;
        }

        outSelector.components.push_back({ simple, pendingCombinator });
        pendingCombinator = Combinator::Descendant; // default separator between sequence tokens is descendant
    }

    return !outSelector.components.empty();
}

bool StyleParser::parseDeclarationBlock(StyleDeclaration& outDecl) {
    if (!expect(TokenType::LBrace, "{")) {
        return false;
    }

    while (m_current.type != TokenType::EndOfFile && m_current.type != TokenType::RBrace) {
        if (m_current.type == TokenType::Semicolon) {
            advance();
            continue;
        }

        if (m_current.type != TokenType::Ident) {
            warning("Expected property name, got '" + m_current.text + "'");
            advance();
            continue;
        }

        int propLine = m_current.line;
        int propCol = m_current.col;
        std::string propName = m_current.text;
        advance();

        if (!expect(TokenType::Colon, ":")) {
            skipUntilNextRule();
            return false;
        }

        std::string propVal;
        int parenDepth = 0;
        while (m_current.type != TokenType::EndOfFile) {
            if (m_current.type == TokenType::LParen) {
                parenDepth++;
            } else if (m_current.type == TokenType::RParen) {
                if (parenDepth > 0) parenDepth--;
            }

            if (parenDepth == 0 && (m_current.type == TokenType::Semicolon || m_current.type == TokenType::RBrace)) {
                break;
            }

            if (!propVal.empty() && m_current.type != TokenType::LParen && m_current.type != TokenType::RParen && m_current.type != TokenType::Comma && propVal.back() != '(') {
                propVal += " ";
            }
            propVal += m_current.text;
            advance();
        }

        if (m_current.type == TokenType::Semicolon) {
            advance();
        }

        propVal = trimString(propVal);
        if (!propName.empty()) {
            outDecl.properties[propName] = propVal;
            outDecl.propertyLocations[propName] = { propLine, propCol };
        }
    }

    expect(TokenType::RBrace, "}");
    return true;
}

} // namespace PerfUI
