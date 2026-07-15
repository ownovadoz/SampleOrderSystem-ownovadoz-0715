#pragma once
#include <map>
#include <string>
#include <vector>
#include "../Common/Sample.h"
#include "../Common/OrderStatus.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"

namespace monitoring {

struct StockOverviewItem {
    common::Sample sample;
    std::string label; // "여유" | "부족" | "고갈"
};

class MonitoringController {
public:
    MonitoringController(sampleclerk::SampleController& sampleController,
                          orderclerk::OrderController& orderController);

    std::map<common::OrderStatus, int> getOrderCountsByStatus() const;
    std::vector<StockOverviewItem> getStockOverview() const;
    std::vector<StockOverviewItem> searchStockOverview(const std::string& query) const;

private:
    sampleclerk::SampleController& sampleController_;
    orderclerk::OrderController& orderController_;
};

} // namespace monitoring
