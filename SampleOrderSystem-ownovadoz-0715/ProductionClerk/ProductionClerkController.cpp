#include "ProductionClerkController.h"

namespace productionclerk {

ProductionClerkController::ProductionClerkController(
    sampleclerk::SampleController& sampleController, orderclerk::OrderController& orderController,
    productionline::ProductionLineController& productionLineController)
    : sampleController_(sampleController), orderController_(orderController),
      productionLineController_(productionLineController) {}

std::vector<common::Order> ProductionClerkController::listPendingApprovals() const {
    return orderController_.listOrdersByStatus(common::OrderStatus::RESERVED);
}

void ProductionClerkController::approve(const std::string& orderId) {
    auto order = orderController_.getOrder(orderId);
    if (!order.has_value() || order->status != common::OrderStatus::RESERVED) return;

    int available = sampleController_.getStock(order->sampleId)
                  - orderController_.getReservedQuantity(order->sampleId);

    if (available >= order->quantity) {
        orderController_.setOrderStatus(orderId, common::OrderStatus::CONFIRMED);
    } else {
        int shortfall = order->quantity - available;
        productionLineController_.enqueue(order->sampleId, shortfall, orderId);
        orderController_.setOrderStatus(orderId, common::OrderStatus::PRODUCING);
    }
}

void ProductionClerkController::reject(const std::string& orderId) {
    orderController_.setOrderStatus(orderId, common::OrderStatus::REJECTED);
}

std::vector<common::Order> ProductionClerkController::listShippable() const {
    return orderController_.listOrdersByStatus(common::OrderStatus::CONFIRMED);
}

common::Result ProductionClerkController::ship(const std::string& orderId) {
    auto order = orderController_.getOrder(orderId);
    if (!order.has_value() || order->status != common::OrderStatus::CONFIRMED) {
        return common::Result::failure("출고 가능한 상태(CONFIRMED)가 아닙니다: " + orderId);
    }
    auto result = sampleController_.decreaseStock(order->sampleId, order->quantity);
    if (!result.ok) return result;
    orderController_.setOrderStatus(orderId, common::OrderStatus::RELEASE);
    return common::Result::success();
}

std::optional<common::Order> ProductionClerkController::getOrder(const std::string& orderId) const {
    return orderController_.getOrder(orderId);
}

} // namespace productionclerk
