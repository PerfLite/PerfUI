#pragma once

#include "PerfUI/Style/StyleSheet.h"
#include "StyleTokenizer.h"
#include <string_view>

namespace PerfUI {

class StyleParser {
public:
    StyleParser(std::string_view source, StyleSheet& sheet, std::string sourceName = "<string>");
    bool parse();

private:
    void parseRuleOrRoot();
    void parseRootBlock();
    void parseKeyframesBlock();
    bool parseSelectorList(std::vector<ComplexSelector>& outSelectors);
    bool parseComplexSelector(ComplexSelector& outSelector);
    bool parseDeclarationBlock(StyleDeclaration& outDecl);

    Token peek();
    Token advance();
    bool match(TokenType type);
    bool expect(TokenType type, const char* expected);
    void skipUntilNextRule();

    void warning(const std::string& msg, int line = 0, int col = 0);
    void error(const std::string& msg, int line = 0, int col = 0);

    StyleTokenizer m_tokenizer;
    StyleSheet& m_sheet;
    Token m_current;
    uint32_t m_orderCounter{ 0 };
};

} // namespace PerfUI
