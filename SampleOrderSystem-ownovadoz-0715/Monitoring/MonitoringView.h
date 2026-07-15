#pragma once
#include <iostream>
#include "MonitoringController.h"

namespace monitoring {

class MonitoringView {
public:
    explicit MonitoringView(MonitoringController& controller);

    void showOrderCountsScreen(std::ostream& out);
    void showStockOverviewScreen(std::ostream& out);
    void showStockSearchScreen(std::istream& in, std::ostream& out);

private:
    void printStockTable(const std::vector<StockOverviewItem>& items, std::ostream& out);
    MonitoringController& controller_;
};

} // namespace monitoring
