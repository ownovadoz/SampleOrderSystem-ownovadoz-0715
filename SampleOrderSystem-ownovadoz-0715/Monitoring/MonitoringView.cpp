#include "MonitoringView.h"
#include "../Common/ConsoleInput.h"

namespace monitoring {

MonitoringView::MonitoringView(MonitoringController& controller) : controller_(controller) {}

void MonitoringView::showOrderCountsScreen(std::ostream& out) {
    common::printScreenHeader(out, "주문량 확인", false);
    auto counts = controller_.getOrderCountsByStatus();
    out << "상태\t건수\n";
    out << "RESERVED\t" << counts[common::OrderStatus::RESERVED] << "\n";
    out << "CONFIRMED\t" << counts[common::OrderStatus::CONFIRMED] << "\n";
    out << "PRODUCING\t" << counts[common::OrderStatus::PRODUCING] << "\n";
    out << "RELEASE\t" << counts[common::OrderStatus::RELEASE] << "\n";
}

void MonitoringView::printStockTable(const std::vector<StockOverviewItem>& items, std::ostream& out) {
    out << "시료명\t재고\t상태\n";
    for (const auto& item : items) {
        out << item.sample.name << '\t' << item.sample.stock << '\t' << item.label << '\n';
    }
}

void MonitoringView::showStockOverviewScreen(std::ostream& out) {
    common::printScreenHeader(out, "재고량 확인", false);
    auto overview = controller_.getStockOverview();
    if (overview.empty()) {
        out << "등록된 시료가 없습니다\n";
        return;
    }
    printStockTable(overview, out);
}

void MonitoringView::showStockSearchScreen(std::istream& in, std::ostream& out) {
    common::printScreenHeader(out, "재고 검색");

    out << "검색어 > ";
    auto keywordInput = common::readLine(in);
    if (!keywordInput.ok || keywordInput.cancelled) {
        out << "취소되었습니다\n";
        return;
    }

    auto results = controller_.searchStockOverview(keywordInput.text);
    if (results.empty()) {
        out << "검색 결과 없음\n";
        return;
    }
    printStockTable(results, out);
}

} // namespace monitoring
