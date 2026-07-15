#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

    class JsonValue;

    using JsonArray = std::vector<JsonValue>;
    using JsonObject = std::map<std::string, JsonValue>;

    enum class JsonType {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    // 범용 JSON 값 타입. 특정 도메인 객체를 알지 못하며,
    // 여러 종류의 객체를 JSON으로 저장/복원할 때 공통으로 재사용된다.
    class JsonValue {
    public:
        JsonValue();
        JsonValue(std::nullptr_t);
        JsonValue(bool value);
        JsonValue(double value);
        JsonValue(const std::string& value);
        JsonValue(const JsonArray& value);
        JsonValue(const JsonObject& value);

        // text가 올바른 JSON이 아니면 std::runtime_error를 던진다.
        static JsonValue Parse(const std::string& text);
        std::string Dump() const;

        JsonType GetType() const;

        bool AsBool() const;
        double AsNumber() const;
        const std::string& AsString() const;
        const JsonArray& AsArray() const;
        const JsonObject& AsObject() const;

    private:
        JsonType m_type;
        std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject> m_data;
    };

}
