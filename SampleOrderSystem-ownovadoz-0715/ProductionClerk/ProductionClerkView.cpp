#include "ProductionClerkView.h"

namespace productionclerk {

ProductionClerkView::ProductionClerkView(ProductionClerkController& controller) : controller_(controller) {}

void ProductionClerkView::showApprovalScreen(std::istream& in, std::ostream& out) {
    auto pending = controller_.listPendingApprovals();
    if (pending.empty()) {
        out << "승인 대기 주문이 없습니다\n";
        return;
    }
    out << "번호\t주문번호\t시료ID\t고객명\t수량\n";
    for (size_t i = 0; i < pending.size(); ++i) {
        out << (i + 1) << '\t' << pending[i].id << '\t' << pending[i].sampleId << '\t'
            << pending[i].customerName << '\t' << pending[i].quantity << '\n';
    }
    out << "승인할 번호 > ";
    size_t index = 0;
    in >> index;
    in.ignore();
    if (index < 1 || index > pending.size()) {
        out << "잘못된 번호입니다\n";
        return;
    }
    std::string orderId = pending[index - 1].id;
    controller_.approve(orderId);
    auto updated = controller_.getOrder(orderId);
    if (updated.has_value() && updated->status == common::OrderStatus::CONFIRMED) {
        out << "승인 완료 (재고 충분 - CONFIRMED)\n";
    } else if (updated.has_value() && updated->status == common::OrderStatus::PRODUCING) {
        out << "승인 완료 (재고 부족 - 생산 라인 투입, PRODUCING)\n";
    } else {
        out << "승인 처리 실패\n";
    }
}

void ProductionClerkView::showRejectionScreen(std::istream& in, std::ostream& out) {
    auto pending = controller_.listPendingApprovals();
    if (pending.empty()) {
        out << "거절 대기 주문이 없습니다\n";
        return;
    }
    out << "번호\t주문번호\t시료ID\t고객명\t수량\n";
    for (size_t i = 0; i < pending.size(); ++i) {
        out << (i + 1) << '\t' << pending[i].id << '\t' << pending[i].sampleId << '\t'
            << pending[i].customerName << '\t' << pending[i].quantity << '\n';
    }
    out << "거절할 번호 > ";
    size_t index = 0;
    in >> index;
    in.ignore();
    if (index < 1 || index > pending.size()) {
        out << "잘못된 번호입니다\n";
        return;
    }
    std::string orderId = pending[index - 1].id;
    controller_.reject(orderId);
    out << "거절 완료 (REJECTED)\n";
}

void ProductionClerkView::showShipmentScreen(std::istream& in, std::ostream& out) {
    auto shippable = controller_.listShippable();
    if (shippable.empty()) {
        out << "출고 가능한 주문이 없습니다\n";
        return;
    }
    out << "번호\t주문번호\t시료ID\t고객명\t수량\n";
    for (size_t i = 0; i < shippable.size(); ++i) {
        out << (i + 1) << '\t' << shippable[i].id << '\t' << shippable[i].sampleId << '\t'
            << shippable[i].customerName << '\t' << shippable[i].quantity << '\n';
    }
    out << "출고할 번호 > ";
    size_t index = 0;
    in >> index;
    in.ignore();
    if (index < 1 || index > shippable.size()) {
        out << "잘못된 번호입니다\n";
        return;
    }
    std::string orderId = shippable[index - 1].id;
    auto result = controller_.ship(orderId);
    if (result.ok) {
        out << "출고 완료 (RELEASE)\n";
    } else {
        out << "출고 실패: " << result.error << "\n";
    }
}

} // namespace productionclerk
