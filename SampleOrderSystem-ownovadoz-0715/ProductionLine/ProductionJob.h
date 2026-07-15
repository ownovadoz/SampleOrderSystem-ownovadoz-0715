#pragma once
#include <chrono>
#include <optional>
#include <string>
#include "../Common/Time.h"
#include "../Common/JsonValue.h"

namespace productionline {

struct ProductionJob {
    std::string orderId;
    std::string sampleId;
    int shortfallQty;
    int actualQty;
    common::Duration totalDuration;
    std::optional<common::TimePoint> startTime;

    json::JsonValue ToJson() const {
        json::JsonObject obj;
        obj["orderId"] = json::JsonValue(orderId);
        obj["sampleId"] = json::JsonValue(sampleId);
        obj["shortfallQty"] = json::JsonValue(static_cast<double>(shortfallQty));
        obj["actualQty"] = json::JsonValue(static_cast<double>(actualQty));
        obj["totalDurationSeconds"] = json::JsonValue(static_cast<double>(totalDuration.count()));
        if (startTime.has_value()) {
            obj["startTimeEpochSeconds"] =
                json::JsonValue(static_cast<double>(std::chrono::system_clock::to_time_t(*startTime)));
        } else {
            obj["startTimeEpochSeconds"] = json::JsonValue(nullptr);
        }
        return json::JsonValue(obj);
    }

    static ProductionJob FromJson(const json::JsonValue& value) {
        const auto& obj = value.AsObject();
        ProductionJob job;
        job.orderId = obj.at("orderId").AsString();
        job.sampleId = obj.at("sampleId").AsString();
        job.shortfallQty = static_cast<int>(obj.at("shortfallQty").AsNumber());
        job.actualQty = static_cast<int>(obj.at("actualQty").AsNumber());
        job.totalDuration = common::Duration(static_cast<long long>(obj.at("totalDurationSeconds").AsNumber()));
        const auto& startField = obj.at("startTimeEpochSeconds");
        if (startField.GetType() == json::JsonType::Null) {
            job.startTime = std::nullopt;
        } else {
            job.startTime = std::chrono::system_clock::from_time_t(static_cast<long long>(startField.AsNumber()));
        }
        return job;
    }
};

} // namespace productionline
