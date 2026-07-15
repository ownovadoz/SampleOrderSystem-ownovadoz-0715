#include "OrderController.h"
#include "../Common/DateFormat.h"
#include <iomanip>
#include <sstream>

namespace orderclerk {

OrderController::OrderController(OrderModel& model, sampleclerk::SampleController& sampleController,
                                  common::IClock& clock)
    : model_(model), sampleController_(sampleController), clock_(clock) {}

std::string OrderController::generateOrderId() const {
    std::string dateStr = common::formatDateYYYYMMDD(clock_.now());
    std::string prefix = "ORD-" + dateStr + "-";
    int count = 0;
    for (const auto& order : model_.findAll()) {
        if (order.id.rfind(prefix, 0) == 0) {
            ++count;
        }
    }
    std::ostringstream oss;
    oss << prefix << std::setw(4) << std::setfill('0') << (count + 1);
    return oss.str();
}

CreateOrderResult OrderController::createOrder(const std::string& sampleId,
                                                const std::string& customerName, int quantity) {
    if (quantity < 1) {
        return CreateOrderResult::failure("주문 수량은 1 이상이어야 합니다");
    }
    if (!sampleController_.getSample(sampleId).has_value()) {
        return CreateOrderResult::failure("존재하지 않는 시료 ID입니다: " + sampleId);
    }
    std::string orderId = generateOrderId();
    common::Order order{orderId, sampleId, customerName, quantity, common::OrderStatus::RESERVED};
    model_.insert(order);
    return CreateOrderResult::success(orderId);
}

std::optional<common::Order> OrderController::getOrder(const std::string& orderId) const {
    return model_.find(orderId);
}

std::vector<common::Order> OrderController::listOrdersByStatus(common::OrderStatus status) const {
    std::vector<common::Order> result;
    for (const auto& order : model_.findAll()) {
        if (order.status == status) {
            result.push_back(order);
        }
    }
    return result;
}

void OrderController::setOrderStatus(const std::string& orderId, common::OrderStatus status) {
    model_.updateStatus(orderId, status);
}

int OrderController::getReservedQuantity(const std::string& sampleId) const {
    int total = 0;
    for (const auto& order : model_.findAll()) {
        if (order.sampleId != sampleId) continue;
        if (order.status == common::OrderStatus::CONFIRMED || order.status == common::OrderStatus::PRODUCING) {
            total += order.quantity;
        }
    }
    return total;
}

std::optional<common::Sample> OrderController::getSampleInfo(const std::string& sampleId) const {
    return sampleController_.getSample(sampleId);
}

} // namespace orderclerk
