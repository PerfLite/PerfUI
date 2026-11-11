#pragma once

#include <string>
#include <string_view>
#include <cctype>

namespace PerfUI {

enum class TokenType {
    EndOfFile,
    Root,
    Ident,
    Number,
    String,
    Colon,
    Semicolon,
    Comma,
    Dot,
    Hash,
    GreaterThan,
    Star,
    LBrace,
    RBrace,
    LParen,
    RParen
};

struct Token {
    TokenType type{ TokenType::EndOfFile };
    std::string text;
    int line{ 1 };
    int col{ 1 };
    bool precededByWhitespace{ false };
};

class StyleTokenizer {
public:
    explicit StyleTokenizer(std::string_view source, std::string sourceName = "<string>")
        : m_source(source), m_sourceName(std::move(sourceName)) {}

    Token nextToken() {
        bool hadWhitespace = skipWhitespaceAndComments();
        if (m_pos >= m_source.size()) {
            return Token{ TokenType::EndOfFile, "", m_line, m_col, hadWhitespace };
        }

        int startLine = m_line;
        int startCol = m_col;
        Token tok = readNextRawToken(startLine, startCol);
        tok.precededByWhitespace = hadWhitespace;
        return tok;
    }

    const std::string& sourceName() const { return m_sourceName; }

private:
    Token readNextRawToken(int startLine, int startCol) {
        char c = m_source[m_pos];

        if (c == ':') {
            if (m_source.substr(m_pos).rfind(":root", 0) == 0) {
                if (m_pos + 5 >= m_source.size() || !isIdentChar(m_source[m_pos + 5])) {
                    m_pos += 5;
                    m_col += 5;
                    return Token{ TokenType::Root, ":root", startLine, startCol };
                }
            }
            advance();
            return Token{ TokenType::Colon, ":", startLine, startCol };
        }
        if (c == ';') { advance(); return Token{ TokenType::Semicolon, ";", startLine, startCol }; }
        if (c == ',') { advance(); return Token{ TokenType::Comma, ",", startLine, startCol }; }
        if (c == '.') {
            // Check if it's a number like .5 or a class dot
            if (m_pos + 1 < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_pos + 1]))) {
                return readNumber(startLine, startCol);
            }
            advance();
            return Token{ TokenType::Dot, ".", startLine, startCol };
        }
        if (c == '>') { advance(); return Token{ TokenType::GreaterThan, ">", startLine, startCol }; }
        if (c == '*') { advance(); return Token{ TokenType::Star, "*", startLine, startCol }; }
        if (c == '{') { advance(); return Token{ TokenType::LBrace, "{", startLine, startCol }; }
        if (c == '}') { advance(); return Token{ TokenType::RBrace, "}", startLine, startCol }; }
        if (c == '(') { advance(); return Token{ TokenType::LParen, "(", startLine, startCol }; }
        if (c == ')') { advance(); return Token{ TokenType::RParen, ")", startLine, startCol }; }

        if (c == '#') {
            advance();
            std::string text = "#";
            while (m_pos < m_source.size() && isIdentChar(m_source[m_pos])) {
                text += m_source[m_pos];
                advance();
            }
            return Token{ TokenType::Hash, text, startLine, startCol };
        }

        if (c == '"' || c == '\'') {
            return readString(c, startLine, startCol);
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && m_pos + 1 < m_source.size() && (std::isdigit(static_cast<unsigned char>(m_source[m_pos + 1])) || m_source[m_pos + 1] == '.'))) {
            return readNumber(startLine, startCol);
        }

        if (isIdentStart(c)) {
            return readIdent(startLine, startCol);
        }

        // Single unknown character
        advance();
        return Token{ TokenType::Ident, std::string(1, c), startLine, startCol };
    }
    char peek() const {
        return m_pos < m_source.size() ? m_source[m_pos] : '\0';
    }

    char advance() {
        if (m_pos >= m_source.size()) return '\0';
        char c = m_source[m_pos++];
        if (c == '\n') {
            m_line++;
            m_col = 1;
        } else {
            m_col++;
        }
        return c;
    }

    bool skipWhitespaceAndComments() {
        bool hadWhitespace = false;
        while (m_pos < m_source.size()) {
            char c = m_source[m_pos];
            if (std::isspace(static_cast<unsigned char>(c))) {
                advance();
                hadWhitespace = true;
                continue;
            }
            // Check comment /* ... */
            if (c == '/' && m_pos + 1 < m_source.size() && m_source[m_pos + 1] == '*') {
                hadWhitespace = true;
                advance(); // /
                advance(); // *
                while (m_pos < m_source.size()) {
                    if (m_source[m_pos] == '*' && m_pos + 1 < m_source.size() && m_source[m_pos + 1] == '/') {
                        advance();
                        advance();
                        break;
                    }
                    advance();
                }
                continue;
            }
            // Check line comment // ...
            if (c == '/' && m_pos + 1 < m_source.size() && m_source[m_pos + 1] == '/') {
                hadWhitespace = true;
                advance();
                advance();
                while (m_pos < m_source.size() && m_source[m_pos] != '\n') {
                    advance();
                }
                continue;
            }
            break;
        }
        return hadWhitespace;
    }

    Token readString(char quote, int startLine, int startCol) {
        advance(); // opening quote
        std::string text;
        while (m_pos < m_source.size()) {
            char c = m_source[m_pos];
            if (c == quote) {
                advance();
                break;
            }
            if (c == '\\' && m_pos + 1 < m_source.size()) {
                advance();
                text += advance();
            } else {
                text += advance();
            }
        }
        return Token{ TokenType::String, text, startLine, startCol };
    }

    Token readNumber(int startLine, int startCol) {
        std::string text;
        if (m_source[m_pos] == '-') {
            text += advance();
        }
        while (m_pos < m_source.size() && (std::isdigit(static_cast<unsigned char>(m_source[m_pos])) || m_source[m_pos] == '.')) {
            text += advance();
        }
        // Unit suffix (px, %, rem, vw, vh, etc.)
        if (m_pos < m_source.size() && m_source[m_pos] == '%') {
            text += advance();
        } else {
            while (m_pos < m_source.size() && std::isalpha(static_cast<unsigned char>(m_source[m_pos]))) {
                text += advance();
            }
        }
        return Token{ TokenType::Number, text, startLine, startCol };
    }

    Token readIdent(int startLine, int startCol) {
        std::string text;
        while (m_pos < m_source.size() && isIdentChar(m_source[m_pos])) {
            text += advance();
        }
        return Token{ TokenType::Ident, text, startLine, startCol };
    }

    static bool isIdentStart(char c) {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '-';
    }

    static bool isIdentChar(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
    }

    std::string_view m_source;
    std::string m_sourceName;
    size_t m_pos{ 0 };
    int m_line{ 1 };
    int m_col{ 1 };
};

} // namespace PerfUI
