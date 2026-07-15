#pragma once
#include <optional>
#include <string>
#include <vector>
#include "../Common/Order.h"
#include "../Common/Result.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include "../ProductionLine/ProductionLineController.h"

namespace productionclerk {

class ProductionClerkController {
public:
    ProductionClerkController(sampleclerk::SampleController& sampleController,
                               orderclerk::OrderController& orderController,
                               productionline::ProductionLineController& productionLineController);

    std::vector<common::Order> listPendingApprovals() const;
    void approve(const std::string& orderId);
    void reject(const std::string& orderId);
    std::vector<common::Order> listShippable() const;
    common::Result ship(const std::string& orderId);
    std::optional<common::Order> getOrder(const std::string& orderId) const;

private:
    sampleclerk::SampleController& sampleController_;
    orderclerk::OrderController& orderController_;
    productionline::ProductionLineController& productionLineController_;
};

} // namespace productionclerk
