#include "MonitoringController.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace monitoring {

namespace {

std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string labelFor(int stock, int demand) {
    if (stock == 0) return "고갈";
    if (stock < demand) return "부족";
    return "여유";
}

constexpr std::array<common::OrderStatus, 4> kTrackedStatuses = {
    common::OrderStatus::RESERVED, common::OrderStatus::CONFIRMED,
    common::OrderStatus::PRODUCING, common::OrderStatus::RELEASE};

constexpr std::array<common::OrderStatus, 3> kDemandStatuses = {
    common::OrderStatus::RESERVED, common::OrderStatus::CONFIRMED, common::OrderStatus::PRODUCING};

} // namespace

MonitoringController::MonitoringController(sampleclerk::SampleController& sampleController,
                                            orderclerk::OrderController& orderController)
    : sampleController_(sampleController), orderController_(orderController) {}

std::map<common::OrderStatus, int> MonitoringController::getOrderCountsByStatus() const {
    std::map<common::OrderStatus, int> counts;
    for (auto status : kTrackedStatuses) {
        counts[status] = static_cast<int>(orderController_.listOrdersByStatus(status).size());
    }
    return counts;
}

std::vector<StockOverviewItem> MonitoringController::getStockOverview() const {
    std::vector<StockOverviewItem> result;
    for (const auto& sample : sampleController_.listSamples()) {
        int demand = 0;
        for (auto status : kDemandStatuses) {
            for (const auto& order : orderController_.listOrdersByStatus(status)) {
                if (order.sampleId == sample.id) {
                    demand += order.quantity;
                }
            }
        }
        result.push_back(StockOverviewItem{sample, labelFor(sample.stock, demand)});
    }
    return result;
}

std::vector<StockOverviewItem> MonitoringController::searchStockOverview(const std::string& query) const {
    std::vector<StockOverviewItem> result;
    std::string lowerQuery = toLower(query);
    for (const auto& item : getStockOverview()) {
        if (toLower(item.sample.name).find(lowerQuery) != std::string::npos ||
            toLower(item.sample.id).find(lowerQuery) != std::string::npos) {
            result.push_back(item);
        }
    }
    return result;
}

} // namespace monitoring
