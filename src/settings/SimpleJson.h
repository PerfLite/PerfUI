#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cctype>
#include <charconv>
#include <cmath>

namespace PerfUI {

class JsonValue {
public:
    enum class Type {
        Null,
        Bool,
        Int,
        Float,
        String,
        Array,
        Object
    };

    using Array = std::vector<JsonValue>;
    using Object = std::vector<std::pair<std::string, JsonValue>>;

    JsonValue() : m_type(Type::Null) {}
    JsonValue(std::nullptr_t) : m_type(Type::Null) {}
    JsonValue(bool val) : m_type(Type::Bool), m_bool(val) {}
    JsonValue(int val) : m_type(Type::Int), m_int(val) {}
    JsonValue(int64_t val) : m_type(Type::Int), m_int(val) {}
    JsonValue(uint32_t val) : m_type(Type::Int), m_int(static_cast<int64_t>(val)) {}
    JsonValue(uint64_t val) : m_type(Type::Int), m_int(static_cast<int64_t>(val)) {}
    JsonValue(float val) : m_type(Type::Float), m_float(val) {}
    JsonValue(double val) : m_type(Type::Float), m_float(val) {}
    JsonValue(const char* val) : m_type(Type::String), m_string(val ? val : "") {}
    JsonValue(std::string val) : m_type(Type::String), m_string(std::move(val)) {}
    JsonValue(std::string_view val) : m_type(Type::String), m_string(val) {}
    JsonValue(Array val) : m_type(Type::Array), m_array(std::make_shared<Array>(std::move(val))) {}
    JsonValue(Object val) : m_type(Type::Object), m_object(std::make_shared<Object>(std::move(val))) {}

    Type type() const { return m_type; }
    bool isNull() const { return m_type == Type::Null; }
    bool isBool() const { return m_type == Type::Bool; }
    bool isInt() const { return m_type == Type::Int; }
    bool isFloat() const { return m_type == Type::Float; }
    bool isNumber() const { return m_type == Type::Int || m_type == Type::Float; }
    bool isString() const { return m_type == Type::String; }
    bool isArray() const { return m_type == Type::Array; }
    bool isObject() const { return m_type == Type::Object; }

    bool asBool(bool def = false) const {
        if (m_type == Type::Bool) return m_bool;
        if (m_type == Type::Int) return m_int != 0;
        if (m_type == Type::Float) return std::abs(m_float) > 1e-6;
        if (m_type == Type::String) {
            return m_string == "true" || m_string == "1" || m_string == "yes";
        }
        return def;
    }

    int64_t asInt(int64_t def = 0) const {
        if (m_type == Type::Int) return m_int;
        if (m_type == Type::Float) return static_cast<int64_t>(std::round(m_float));
        if (m_type == Type::Bool) return m_bool ? 1 : 0;
        if (m_type == Type::String) {
            try { return std::stoll(m_string); } catch (...) {}
        }
        return def;
    }

    double asFloat(double def = 0.0) const {
        if (m_type == Type::Float) return m_float;
        if (m_type == Type::Int) return static_cast<double>(m_int);
        if (m_type == Type::Bool) return m_bool ? 1.0 : 0.0;
        if (m_type == Type::String) {
            try { return std::stod(m_string); } catch (...) {}
        }
        return def;
    }

    const std::string& asString() const {
        static const std::string s_empty;
        if (m_type == Type::String) return m_string;
        return s_empty;
    }

    std::string toString() const {
        if (m_type == Type::String) return m_string;
        if (m_type == Type::Bool) return m_bool ? "true" : "false";
        if (m_type == Type::Int) return std::to_string(m_int);
        if (m_type == Type::Float) {
            std::ostringstream ss;
            ss << m_float;
            return ss.str();
        }
        if (m_type == Type::Null) return "null";
        return serialize(0);
    }

    size_t size() const {
        if (m_type == Type::Array && m_array) return m_array->size();
        if (m_type == Type::Object && m_object) return m_object->size();
        return 0;
    }

    bool empty() const {
        return size() == 0;
    }

    const Array& array() const {
        static const Array s_empty;
        return (m_type == Type::Array && m_array) ? *m_array : s_empty;
    }

    Array& array() {
        if (m_type != Type::Array || !m_array) {
            m_type = Type::Array;
            m_array = std::make_shared<Array>();
        }
        return *m_array;
    }

    const Object& object() const {
        static const Object s_empty;
        return (m_type == Type::Object && m_object) ? *m_object : s_empty;
    }

    Object& object() {
        if (m_type != Type::Object || !m_object) {
            m_type = Type::Object;
            m_object = std::make_shared<Object>();
        }
        return *m_object;
    }

    bool has(std::string_view key) const {
        if (m_type != Type::Object || !m_object) return false;
        for (const auto& [k, v] : *m_object) {
            if (k == key) return true;
        }
        return false;
    }

    const JsonValue& get(std::string_view key) const {
        static const JsonValue s_null;
        if (m_type != Type::Object || !m_object) return s_null;
        for (const auto& [k, v] : *m_object) {
            if (k == key) return v;
        }
        return s_null;
    }

    const JsonValue& operator[](std::string_view key) const {
        return get(key);
    }

    JsonValue& operator[](std::string_view key) {
        if (m_type != Type::Object || !m_object) {
            m_type = Type::Object;
            m_object = std::make_shared<Object>();
        }
        for (auto& [k, v] : *m_object) {
            if (k == key) return v;
        }
        m_object->push_back({ std::string(key), JsonValue() });
        return m_object->back().second;
    }

    const JsonValue& operator[](size_t index) const {
        static const JsonValue s_null;
        if (m_type != Type::Array || !m_array || index >= m_array->size()) return s_null;
        return (*m_array)[index];
    }

    JsonValue& operator[](size_t index) {
        if (m_type != Type::Array || !m_array) {
            m_type = Type::Array;
            m_array = std::make_shared<Array>();
        }
        if (index >= m_array->size()) {
            m_array->resize(index + 1);
        }
        return (*m_array)[index];
    }

    void push_back(JsonValue val) {
        array().push_back(std::move(val));
    }

    // Serialization
    std::string serialize(int indent = 2) const {
        std::ostringstream ss;
        serializeInternal(ss, indent, 0);
        return ss.str();
    }

    // Parsing
    static JsonValue parse(std::string_view json, std::string* error = nullptr) {
        size_t pos = 0;
        skipWhitespace(json, pos);
        if (pos >= json.size()) {
            if (error) *error = "Empty JSON input";
            return JsonValue();
        }
        auto res = parseValue(json, pos, error);
        skipWhitespace(json, pos);
        if (pos < json.size() && (!error || error->empty())) {
            // Trailing characters check
            if (error) *error = "Extra characters after JSON root at pos " + std::to_string(pos);
        }
        return res;
    }

private:
    Type m_type{ Type::Null };
    bool m_bool{ false };
    int64_t m_int{ 0 };
    double m_float{ 0.0 };
    std::string m_string;
    std::shared_ptr<Array> m_array;
    std::shared_ptr<Object> m_object;

    static void skipWhitespace(std::string_view s, size_t& pos) {
        while (pos < s.size()) {
            char c = s[pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++pos;
            } else if (c == '/' && pos + 1 < s.size() && s[pos + 1] == '/') {
                // Line comment support (comments in config)
                pos += 2;
                while (pos < s.size() && s[pos] != '\n') ++pos;
            } else if (c == '/' && pos + 1 < s.size() && s[pos + 1] == '*') {
                // Block comment support
                pos += 2;
                while (pos + 1 < s.size() && !(s[pos] == '*' && s[pos + 1] == '/')) ++pos;
                if (pos + 1 < s.size()) pos += 2;
            } else {
                break;
            }
        }
    }

    static JsonValue parseValue(std::string_view s, size_t& pos, std::string* error) {
        skipWhitespace(s, pos);
        if (pos >= s.size()) {
            if (error) *error = "Unexpected end of input";
            return JsonValue();
        }

        char c = s[pos];
        if (c == '{') return parseObject(s, pos, error);
        if (c == '[') return parseArray(s, pos, error);
        if (c == '\"') return parseString(s, pos, error);
        if (c == 't' || c == 'f') return parseBool(s, pos, error);
        if (c == 'n') return parseNull(s, pos, error);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(s, pos, error);

        if (error) *error = std::string("Unexpected token '") + c + "' at position " + std::to_string(pos);
        return JsonValue();
    }

    static JsonValue parseObject(std::string_view s, size_t& pos, std::string* error) {
        ++pos; // skip '{'
        Object obj;

        while (true) {
            skipWhitespace(s, pos);
            if (pos >= s.size()) {
                if (error) *error = "Unterminated object";
                return JsonValue();
            }
            if (s[pos] == '}') {
                ++pos;
                break;
            }

            if (s[pos] != '\"') {
                if (error) *error = "Expected string key in object at position " + std::to_string(pos);
                return JsonValue();
            }

            auto keyVal = parseString(s, pos, error);
            if (error && !error->empty()) return JsonValue();
            std::string key = keyVal.asString();

            skipWhitespace(s, pos);
            if (pos >= s.size() || s[pos] != ':') {
                if (error) *error = "Expected ':' after key in object at position " + std::to_string(pos);
                return JsonValue();
            }
            ++pos; // skip ':'

            auto val = parseValue(s, pos, error);
            if (error && !error->empty()) return JsonValue();

            obj.push_back({ std::move(key), std::move(val) });

            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') {
                ++pos;
                // Allow trailing comma: if next is '}', will break at top
            } else if (pos < s.size() && s[pos] == '}') {
                ++pos;
                break;
            } else {
                if (error) *error = "Expected ',' or '}' in object at position " + std::to_string(pos);
                return JsonValue();
            }
        }

        return JsonValue(std::move(obj));
    }

    static JsonValue parseArray(std::string_view s, size_t& pos, std::string* error) {
        ++pos; // skip '['
        Array arr;

        while (true) {
            skipWhitespace(s, pos);
            if (pos >= s.size()) {
                if (error) *error = "Unterminated array";
                return JsonValue();
            }
            if (s[pos] == ']') {
                ++pos;
                break;
            }

            auto val = parseValue(s, pos, error);
            if (error && !error->empty()) return JsonValue();

            arr.push_back(std::move(val));

            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') {
                ++pos;
                // Allow trailing comma
            } else if (pos < s.size() && s[pos] == ']') {
                ++pos;
                break;
            } else {
                if (error) *error = "Expected ',' or ']' in array at position " + std::to_string(pos);
                return JsonValue();
            }
        }

        return JsonValue(std::move(arr));
    }

    static JsonValue parseString(std::string_view s, size_t& pos, std::string* error) {
        ++pos; // skip '\"'
        std::string res;

        while (pos < s.size()) {
            char c = s[pos++];
            if (c == '\"') {
                return JsonValue(std::move(res));
            }
            if (c == '\\' && pos < s.size()) {
                char esc = s[pos++];
                switch (esc) {
                case '\"': res.push_back('\"'); break;
                case '\\': res.push_back('\\'); break;
                case '/':  res.push_back('/');  break;
                case 'b':  res.push_back('\b'); break;
                case 'f':  res.push_back('\f'); break;
                case 'n':  res.push_back('\n'); break;
                case 'r':  res.push_back('\r'); break;
                case 't':  res.push_back('\t'); break;
                case 'u': {
                    if (pos + 4 <= s.size()) {
                        uint32_t codepoint = 0;
                        for (int i = 0; i < 4; ++i) {
                            char h = s[pos++];
                            codepoint <<= 4;
                            if (h >= '0' && h <= '9') codepoint |= (h - '0');
                            else if (h >= 'a' && h <= 'f') codepoint |= (h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') codepoint |= (h - 'A' + 10);
                        }
                        // Simple UTF-8 encode
                        if (codepoint <= 0x7F) {
                            res.push_back(static_cast<char>(codepoint));
                        } else if (codepoint <= 0x7FF) {
                            res.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
                            res.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                        } else {
                            res.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
                            res.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                            res.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                        }
                    }
                    break;
                }
                default:
                    res.push_back(esc);
                    break;
                }
            } else {
                res.push_back(c);
            }
        }

        if (error) *error = "Unterminated string";
        return JsonValue();
    }

    static JsonValue parseNumber(std::string_view s, size_t& pos, std::string* error) {
        size_t start = pos;
        if (pos < s.size() && s[pos] == '-') ++pos;

        bool hasDot = false;
        bool hasExp = false;

        while (pos < s.size()) {
            char c = s[pos];
            if (c >= '0' && c <= '9') {
                ++pos;
            } else if (c == '.' && !hasDot && !hasExp) {
                hasDot = true;
                ++pos;
            } else if ((c == 'e' || c == 'E') && !hasExp) {
                hasExp = true;
                ++pos;
                if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
            } else {
                break;
            }
        }

        std::string_view numStr = s.substr(start, pos - start);
        if (hasDot || hasExp) {
            try {
                double val = std::stod(std::string(numStr));
                return JsonValue(val);
            } catch (...) {}
        } else {
            try {
                int64_t val = std::stoll(std::string(numStr));
                return JsonValue(val);
            } catch (...) {}
        }

        if (error) *error = "Invalid number: " + std::string(numStr);
        return JsonValue();
    }

    static JsonValue parseBool(std::string_view s, size_t& pos, std::string* error) {
        if (s.substr(pos, 4) == "true") {
            pos += 4;
            return JsonValue(true);
        }
        if (s.substr(pos, 5) == "false") {
            pos += 5;
            return JsonValue(false);
        }
        if (error) *error = "Invalid boolean token at position " + std::to_string(pos);
        return JsonValue();
    }

    static JsonValue parseNull(std::string_view s, size_t& pos, std::string* error) {
        if (s.substr(pos, 4) == "null") {
            pos += 4;
            return JsonValue(nullptr);
        }
        if (error) *error = "Invalid null token at position " + std::to_string(pos);
        return JsonValue();
    }

    static std::string escapeString(std::string_view s) {
        std::string res;
        res.reserve(s.size() + 8);
        res.push_back('\"');
        for (char c : s) {
            switch (c) {
            case '\"': res.append("\\\""); break;
            case '\\': res.append("\\\\"); break;
            case '\b': res.append("\\b"); break;
            case '\f': res.append("\\f"); break;
            case '\n': res.append("\\n"); break;
            case '\r': res.append("\\r"); break;
            case '\t': res.append("\\t"); break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream ss;
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    res.append(ss.str());
                } else {
                    res.push_back(c);
                }
                break;
            }
        }
        res.push_back('\"');
        return res;
    }

    void serializeInternal(std::ostringstream& ss, int indent, int depth) const {
        std::string indentStr = (indent > 0) ? std::string(depth * indent, ' ') : "";
        std::string nextIndentStr = (indent > 0) ? std::string((depth + 1) * indent, ' ') : "";
        std::string newline = (indent > 0) ? "\n" : "";
        std::string space = (indent > 0) ? " " : "";

        switch (m_type) {
        case Type::Null: ss << "null"; break;
        case Type::Bool: ss << (m_bool ? "true" : "false"); break;
        case Type::Int: ss << m_int; break;
        case Type::Float: {
            if (std::isnan(m_float)) ss << "null";
            else if (std::isinf(m_float)) ss << (m_float > 0 ? "1e999" : "-1e999");
            else {
                ss << std::setprecision(6) << m_float;
            }
            break;
        }
        case Type::String: ss << escapeString(m_string); break;
        case Type::Array: {
            if (!m_array || m_array->empty()) {
                ss << "[]";
            } else {
                ss << "[" << newline;
                for (size_t i = 0; i < m_array->size(); ++i) {
                    ss << nextIndentStr;
                    (*m_array)[i].serializeInternal(ss, indent, depth + 1);
                    if (i + 1 < m_array->size()) ss << ",";
                    ss << newline;
                }
                ss << indentStr << "]";
            }
            break;
        }
        case Type::Object: {
            if (!m_object || m_object->empty()) {
                ss << "{}";
            } else {
                ss << "{" << newline;
                for (size_t i = 0; i < m_object->size(); ++i) {
                    const auto& [k, v] = (*m_object)[i];
                    ss << nextIndentStr << escapeString(k) << ":" << space;
                    v.serializeInternal(ss, indent, depth + 1);
                    if (i + 1 < m_object->size()) ss << ",";
                    ss << newline;
                }
                ss << indentStr << "}";
            }
            break;
        }
        }
    }
};

} // namespace PerfUI
