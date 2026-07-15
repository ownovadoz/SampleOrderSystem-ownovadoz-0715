#pragma once
#include <optional>
#include <string>
#include <vector>
#include "OrderModel.h"
#include "../SampleClerk/SampleController.h"
#include "../Common/Clock.h"

namespace orderclerk {

struct CreateOrderResult {
    bool ok;
    std::string error;
    std::string orderId;

    static CreateOrderResult success(std::string orderId) {
        return CreateOrderResult{true, "", std::move(orderId)};
    }
    static CreateOrderResult failure(std::string err) {
        return CreateOrderResult{false, std::move(err), ""};
    }
};

class OrderController {
public:
    OrderController(OrderModel& model, sampleclerk::SampleController& sampleController, common::IClock& clock);

    CreateOrderResult createOrder(const std::string& sampleId, const std::string& customerName, int quantity);
    std::optional<common::Order> getOrder(const std::string& orderId) const;
    std::vector<common::Order> listOrdersByStatus(common::OrderStatus status) const;
    void setOrderStatus(const std::string& orderId, common::OrderStatus status);
    int getReservedQuantity(const std::string& sampleId) const;
    std::optional<common::Sample> getSampleInfo(const std::string& sampleId) const;

private:
    std::string generateOrderId() const;

    OrderModel& model_;
    sampleclerk::SampleController& sampleController_;
    common::IClock& clock_;
};

} // namespace orderclerk
