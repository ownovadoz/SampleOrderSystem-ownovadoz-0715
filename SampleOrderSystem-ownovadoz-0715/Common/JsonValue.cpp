#include "JsonValue.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace json {

    namespace {

        void DumpString(const std::string& value, std::ostringstream& out) {
            out << '"';
            for (unsigned char c : value) {
                switch (c) {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (c < 0x20) {
                        char buffer[7];
                        std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
                        out << buffer;
                    } else {
                        // 0x80 이상(한글 등 UTF-8 멀티바이트)은 그대로 통과시킨다.
                        // JSON은 원문이 UTF-8이면 이스케이프 없이 그대로 담아도 유효하다.
                        out << c;
                    }
                    break;
                }
            }
            out << '"';
        }

        void DumpNumber(double value, std::ostringstream& out) {
            if (value == static_cast<long long>(value)) {
                out << static_cast<long long>(value);
            } else {
                std::ostringstream numberStream;
                // double을 정확히 왕복(round-trip)시키려면 최대 17자리(max_digits10)가 필요하다.
                // 15자리(기존 DataPersistence PoC 값)로는 마지막 몇 비트가 소실되어 저장 전/후 값이
                // 달라질 수 있다 (예: yield 같은 필드가 저장 후 다시 읽으면 미세하게 다른 double이 됨).
                numberStream.precision(std::numeric_limits<double>::max_digits10);
                numberStream << value;
                out << numberStream.str();
            }
        }

        void DumpValue(const JsonValue& value, std::ostringstream& out) {
            switch (value.GetType()) {
            case JsonType::Null:
                out << "null";
                break;
            case JsonType::Boolean:
                out << (value.AsBool() ? "true" : "false");
                break;
            case JsonType::Number:
                DumpNumber(value.AsNumber(), out);
                break;
            case JsonType::String:
                DumpString(value.AsString(), out);
                break;
            case JsonType::Array: {
                out << '[';
                const JsonArray& array = value.AsArray();
                for (size_t i = 0; i < array.size(); ++i) {
                    if (i > 0) {
                        out << ',';
                    }
                    DumpValue(array[i], out);
                }
                out << ']';
                break;
            }
            case JsonType::Object: {
                out << '{';
                const JsonObject& object = value.AsObject();
                bool first = true;
                for (const auto& [key, entry] : object) {
                    if (!first) {
                        out << ',';
                    }
                    first = false;
                    DumpString(key, out);
                    out << ':';
                    DumpValue(entry, out);
                }
                out << '}';
                break;
            }
            }
        }

        // 재귀 하강 파서. text 전체를 소비하며, 위치(pos)를 진행시킨다.
        class Parser {
        public:
            explicit Parser(const std::string& text) : m_text(text), m_pos(0) {}

            JsonValue ParseDocument() {
                SkipWhitespace();
                JsonValue value = ParseValue();
                SkipWhitespace();
                if (m_pos != m_text.size()) {
                    throw std::runtime_error("JSON 파싱 실패: 문서 끝에 불필요한 문자가 있습니다.");
                }
                return value;
            }

        private:
            const std::string& m_text;
            size_t m_pos;

            char Peek() const {
                if (m_pos >= m_text.size()) {
                    throw std::runtime_error("JSON 파싱 실패: 입력이 예기치 않게 끝났습니다.");
                }
                return m_text[m_pos];
            }

            char Next() {
                char c = Peek();
                ++m_pos;
                return c;
            }

            void Expect(char expected) {
                if (Next() != expected) {
                    throw std::runtime_error(std::string("JSON 파싱 실패: '") + expected + "' 문자가 필요합니다.");
                }
            }

            void SkipWhitespace() {
                while (m_pos < m_text.size()) {
                    char c = m_text[m_pos];
                    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                        ++m_pos;
                    } else {
                        break;
                    }
                }
            }

            bool StartsWith(const char* literal) const {
                size_t len = std::char_traits<char>::length(literal);
                return m_text.compare(m_pos, len, literal) == 0;
            }

            JsonValue ParseValue() {
                char c = Peek();
                switch (c) {
                case '"': return JsonValue(ParseString());
                case '{': return ParseObject();
                case '[': return ParseArray();
                case 't':
                    if (StartsWith("true")) { m_pos += 4; return JsonValue(true); }
                    throw std::runtime_error("JSON 파싱 실패: 'true'가 아닙니다.");
                case 'f':
                    if (StartsWith("false")) { m_pos += 5; return JsonValue(false); }
                    throw std::runtime_error("JSON 파싱 실패: 'false'가 아닙니다.");
                case 'n':
                    if (StartsWith("null")) { m_pos += 4; return JsonValue(nullptr); }
                    throw std::runtime_error("JSON 파싱 실패: 'null'이 아닙니다.");
                default:
                    if (c == '-' || (c >= '0' && c <= '9')) {
                        return JsonValue(ParseNumber());
                    }
                    throw std::runtime_error("JSON 파싱 실패: 알 수 없는 값입니다.");
                }
            }

            void AppendUtf8CodePoint(unsigned int codePoint, std::string& out) const {
                if (codePoint <= 0x7F) {
                    out += static_cast<char>(codePoint);
                } else if (codePoint <= 0x7FF) {
                    out += static_cast<char>(0xC0 | (codePoint >> 6));
                    out += static_cast<char>(0x80 | (codePoint & 0x3F));
                } else {
                    out += static_cast<char>(0xE0 | (codePoint >> 12));
                    out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (codePoint & 0x3F));
                }
            }

            std::string ParseString() {
                Expect('"');
                std::string result;
                while (true) {
                    char c = Next();
                    if (c == '"') {
                        break;
                    }
                    if (c == '\\') {
                        char escaped = Next();
                        switch (escaped) {
                        case '"':  result += '"'; break;
                        case '\\': result += '\\'; break;
                        case '/':  result += '/'; break;
                        case 'b':  result += '\b'; break;
                        case 'f':  result += '\f'; break;
                        case 'n':  result += '\n'; break;
                        case 'r':  result += '\r'; break;
                        case 't':  result += '\t'; break;
                        case 'u': {
                            unsigned int codePoint = 0;
                            for (int i = 0; i < 4; ++i) {
                                char hex = Next();
                                codePoint <<= 4;
                                if (hex >= '0' && hex <= '9') codePoint |= (hex - '0');
                                else if (hex >= 'a' && hex <= 'f') codePoint |= (hex - 'a' + 10);
                                else if (hex >= 'A' && hex <= 'F') codePoint |= (hex - 'A' + 10);
                                else throw std::runtime_error("JSON 파싱 실패: 잘못된 \\u 이스케이프입니다.");
                            }
                            AppendUtf8CodePoint(codePoint, result);
                            break;
                        }
                        default:
                            throw std::runtime_error("JSON 파싱 실패: 알 수 없는 이스케이프 문자입니다.");
                        }
                    } else {
                        result += c;
                    }
                }
                return result;
            }

            double ParseNumber() {
                size_t start = m_pos;
                if (Peek() == '-') {
                    ++m_pos;
                }
                while (m_pos < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_pos]))) {
                    ++m_pos;
                }
                if (m_pos < m_text.size() && m_text[m_pos] == '.') {
                    ++m_pos;
                    while (m_pos < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_pos]))) {
                        ++m_pos;
                    }
                }
                if (m_pos < m_text.size() && (m_text[m_pos] == 'e' || m_text[m_pos] == 'E')) {
                    ++m_pos;
                    if (m_pos < m_text.size() && (m_text[m_pos] == '+' || m_text[m_pos] == '-')) {
                        ++m_pos;
                    }
                    while (m_pos < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_pos]))) {
                        ++m_pos;
                    }
                }
                std::string token = m_text.substr(start, m_pos - start);
                try {
                    return std::stod(token);
                } catch (const std::exception&) {
                    throw std::runtime_error("JSON 파싱 실패: 숫자 형식이 올바르지 않습니다.");
                }
            }

            JsonValue ParseArray() {
                Expect('[');
                JsonArray array;
                SkipWhitespace();
                if (Peek() == ']') {
                    ++m_pos;
                    return JsonValue(array);
                }
                while (true) {
                    SkipWhitespace();
                    array.push_back(ParseValue());
                    SkipWhitespace();
                    char c = Next();
                    if (c == ']') {
                        break;
                    }
                    if (c != ',') {
                        throw std::runtime_error("JSON 파싱 실패: 배열에 ','가 필요합니다.");
                    }
                }
                return JsonValue(array);
            }

            JsonValue ParseObject() {
                Expect('{');
                JsonObject object;
                SkipWhitespace();
                if (Peek() == '}') {
                    ++m_pos;
                    return JsonValue(object);
                }
                while (true) {
                    SkipWhitespace();
                    std::string key = ParseString();
                    SkipWhitespace();
                    Expect(':');
                    SkipWhitespace();
                    object[key] = ParseValue();
                    SkipWhitespace();
                    char c = Next();
                    if (c == '}') {
                        break;
                    }
                    if (c != ',') {
                        throw std::runtime_error("JSON 파싱 실패: 객체에 ','가 필요합니다.");
                    }
                }
                return JsonValue(object);
            }
        };

    }

    JsonValue::JsonValue() : m_type(JsonType::Null), m_data(nullptr) {
    }

    JsonValue::JsonValue(std::nullptr_t) : m_type(JsonType::Null), m_data(nullptr) {
    }

    JsonValue::JsonValue(bool value) : m_type(JsonType::Boolean), m_data(value) {
    }

    JsonValue::JsonValue(double value) : m_type(JsonType::Number), m_data(value) {
    }

    JsonValue::JsonValue(const std::string& value) : m_type(JsonType::String), m_data(value) {
    }

    JsonValue::JsonValue(const JsonArray& value) : m_type(JsonType::Array), m_data(value) {
    }

    JsonValue::JsonValue(const JsonObject& value) : m_type(JsonType::Object), m_data(value) {
    }

    JsonValue JsonValue::Parse(const std::string& text) {
        Parser parser(text);
        return parser.ParseDocument();
    }

    std::string JsonValue::Dump() const {
        std::ostringstream out;
        DumpValue(*this, out);
        return out.str();
    }

    JsonType JsonValue::GetType() const {
        return m_type;
    }

    bool JsonValue::AsBool() const {
        return std::get<bool>(m_data);
    }

    double JsonValue::AsNumber() const {
        return std::get<double>(m_data);
    }

    const std::string& JsonValue::AsString() const {
        return std::get<std::string>(m_data);
    }

    const JsonArray& JsonValue::AsArray() const {
        return std::get<JsonArray>(m_data);
    }

    const JsonObject& JsonValue::AsObject() const {
        return std::get<JsonObject>(m_data);
    }

}
