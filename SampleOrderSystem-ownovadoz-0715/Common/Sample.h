#pragma once
#include <string>
#include "Time.h"
#include "JsonValue.h"

namespace common {

struct Sample {
    std::string id;
    std::string name;
    Duration avgProductionTime;
    double yield;   // 0 초과 1 이하
    int stock;      // 원재고 수량

    json::JsonValue ToJson() const {
        json::JsonObject obj;
        obj["id"] = json::JsonValue(id);
        obj["name"] = json::JsonValue(name);
        obj["avgProductionTimeSeconds"] = json::JsonValue(static_cast<double>(avgProductionTime.count()));
        obj["yield"] = json::JsonValue(yield);
        obj["stock"] = json::JsonValue(static_cast<double>(stock));
        return json::JsonValue(obj);
    }

    static Sample FromJson(const json::JsonValue& value) {
        const auto& obj = value.AsObject();
        Sample sample;
        sample.id = obj.at("id").AsString();
        sample.name = obj.at("name").AsString();
        sample.avgProductionTime = Duration(static_cast<long long>(obj.at("avgProductionTimeSeconds").AsNumber()));
        sample.yield = obj.at("yield").AsNumber();
        sample.stock = static_cast<int>(obj.at("stock").AsNumber());
        return sample;
    }
};

} // namespace common
