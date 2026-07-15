#pragma once
#include <string>
#include "OrderStatus.h"
#include "JsonValue.h"

namespace common {

struct Order {
    std::string id;
    std::string sampleId;
    std::string customerName;
    int quantity;
    OrderStatus status;

    json::JsonValue ToJson() const {
        json::JsonObject obj;
        obj["id"] = json::JsonValue(id);
        obj["sampleId"] = json::JsonValue(sampleId);
        obj["customerName"] = json::JsonValue(customerName);
        obj["quantity"] = json::JsonValue(static_cast<double>(quantity));
        obj["status"] = json::JsonValue(static_cast<double>(static_cast<int>(status)));
        return json::JsonValue(obj);
    }

    static Order FromJson(const json::JsonValue& value) {
        const auto& obj = value.AsObject();
        Order order;
        order.id = obj.at("id").AsString();
        order.sampleId = obj.at("sampleId").AsString();
        order.customerName = obj.at("customerName").AsString();
        order.quantity = static_cast<int>(obj.at("quantity").AsNumber());
        order.status = static_cast<OrderStatus>(static_cast<int>(obj.at("status").AsNumber()));
        return order;
    }
};

} // namespace common
