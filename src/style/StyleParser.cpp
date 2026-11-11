#include "StyleParser.h"
#include <algorithm>
#include <sstream>

namespace PerfUI {

static std::string trimString(std::string_view sv) {
    size_t first = 0;
    while (first < sv.size() && std::isspace(static_cast<unsigned char>(sv[first]))) ++first;
    size_t last = sv.size();
    while (last > first && std::isspace(static_cast<unsigned char>(sv[last - 1]))) --last;
    return std::string(sv.substr(first, last - first));
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
        }
    }

    expect(TokenType::RBrace, "}");
    return true;
}

} // namespace PerfUI
