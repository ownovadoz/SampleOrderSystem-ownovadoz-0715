#include "ProductionLineView.h"
#include "../Common/ConsoleInput.h"

namespace productionline {

ProductionLineView::ProductionLineView(ProductionLineController& controller) : controller_(controller) {}

void ProductionLineView::showCurrentJobScreen(std::ostream& out) {
    common::printScreenHeader(out, "생산 현황", false);
    auto job = controller_.getCurrentJob();
    if (!job.has_value()) {
        out << "대기 중인 생산 없음\n";
        return;
    }
    out << "주문번호 " << job->orderId << "\n";
    out << "시료 " << job->sampleName << "\n";
    out << "부족분 " << job->shortfallQty << " ea\n";
    out << "실생산량 " << job->actualQty << " ea\n";
    out << "진행률 " << static_cast<int>(job->progressPercent) << "%\n";
}

void ProductionLineView::showQueueScreen(std::ostream& out) {
    common::printScreenHeader(out, "대기 주문 확인", false);
    auto queue = controller_.getQueue();
    if (queue.empty()) {
        out << "대기 중인 주문 없음\n";
        return;
    }
    out << "순서\t주문번호\t시료\t부족분\t실생산량\n";
    int index = 1;
    for (const auto& job : queue) {
        out << index++ << '\t' << job.orderId << '\t' << job.sampleName << '\t' << job.shortfallQty
            << '\t' << job.actualQty << '\n';
    }
}

} // namespace productionline
