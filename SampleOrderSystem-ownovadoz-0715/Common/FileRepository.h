#pragma once

#include <concepts>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "JsonValue.h"

// JSON으로 저장하려는 타입이 만족해야 하는 컴파일 타임 제약.
// FileRepository<T>는 T를 항상 컴파일 타임에 알고 있으므로(타입별로 저장소를 분리),
// 가상 디스패치가 필요한 상속 인터페이스 대신 concept으로 충분하다.
template<typename T>
concept JsonSerializable = requires(const T & t, const json::JsonValue & v) {
    { t.ToJson() } -> std::same_as<json::JsonValue>;
    { T::FromJson(v) } -> std::same_as<T>;
};

// path에 있는 JSON 파일로 std::vector<T>를 저장/복원한다.
// T당 하나의 파일을 사용하는 것을 전제로 하며(타입 혼합 저장은 지원하지 않음),
// Model/Controller는 이 클래스가 JSON을 어떻게 다루는지 알 필요가 없다.
template<JsonSerializable T>
class FileRepository {
public:
    // 파일이 없거나 JSON이 손상된 경우 빈 벡터를 반환한다.
    std::vector<T> Load(const std::string& path) const {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return {};
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();

        try {
            json::JsonValue root = json::JsonValue::Parse(buffer.str());
            if (root.GetType() != json::JsonType::Array) {
                return {};
            }

            std::vector<T> items;
            items.reserve(root.AsArray().size());
            for (const auto& element : root.AsArray()) {
                items.push_back(T::FromJson(element));
            }
            return items;
        } catch (const std::exception&) {
            return {};
        }
    }

    // 파일을 열 수 없으면 std::runtime_error를 던진다.
    void Save(const std::string& path, const std::vector<T>& items) const {
        json::JsonArray array;
        array.reserve(items.size());
        for (const auto& item : items) {
            array.push_back(item.ToJson());
        }

        std::ofstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("파일을 쓰기 위해 열 수 없습니다: " + path);
        }

        std::string text = json::JsonValue(array).Dump();
        file << text;
    }
};
