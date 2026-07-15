#include "ProductionClerkView.h"
#include "../Common/ConsoleInput.h"
#include <optional>
#include <string>
#include <vector>

namespace productionclerk {

namespace {

void printOrderTable(const std::vector<common::Order>& orders, std::ostream& out) {
    out << "번호\t주문번호\t시료ID\t고객명\t수량\n";
    for (size_t i = 0; i < orders.size(); ++i) {
        out << (i + 1) << '\t' << orders[i].id << '\t' << orders[i].sampleId << '\t'
            << orders[i].customerName << '\t' << orders[i].quantity << '\n';
    }
}

// 목록에서 번호를 하나 고르게 한다. 취소/잘못된 입력/범위 밖 번호를 모두 안전하게 처리하고,
// 실제로 유효한 번호를 골랐을 때만 그 인덱스(0-base)를 반환한다.
std::optional<size_t> selectFromList(std::istream& in, std::ostream& out, size_t listSize,
                                      const std::string& prompt) {
    out << prompt << " > ";
    auto input = common::readInt(in);
    if (!input.ok) {
        out << (input.cancelled ? "취소되었습니다\n" : "잘못된 입력입니다\n");
        return std::nullopt;
    }
    if (input.value < 1 || static_cast<size_t>(input.value) > listSize) {
        out << "잘못된 번호입니다\n";
        return std::nullopt;
    }
    return static_cast<size_t>(input.value) - 1;
}

} // namespace

ProductionClerkView::ProductionClerkView(ProductionClerkController& controller) : controller_(controller) {}

void ProductionClerkView::showApprovalScreen(std::istream& in, std::ostream& out) {
    common::printScreenHeader(out, "주문 승인");
    auto pending = controller_.listPendingApprovals();
    if (pending.empty()) {
        out << "승인 대기 주문이 없습니다\n";
        return;
    }
    printOrderTable(pending, out);
    auto selected = selectFromList(in, out, pending.size(), "승인할 번호");
    if (!selected.has_value()) {
        return;
    }
    std::string orderId = pending[*selected].id;
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
    common::printScreenHeader(out, "주문 거절");
    auto pending = controller_.listPendingApprovals();
    if (pending.empty()) {
        out << "거절 대기 주문이 없습니다\n";
        return;
    }
    printOrderTable(pending, out);
    auto selected = selectFromList(in, out, pending.size(), "거절할 번호");
    if (!selected.has_value()) {
        return;
    }
    controller_.reject(pending[*selected].id);
    out << "거절 완료 (REJECTED)\n";
}

void ProductionClerkView::showShipmentScreen(std::istream& in, std::ostream& out) {
    common::printScreenHeader(out, "출고 처리");
    auto shippable = controller_.listShippable();
    if (shippable.empty()) {
        out << "출고 가능한 주문이 없습니다\n";
        return;
    }
    printOrderTable(shippable, out);
    auto selected = selectFromList(in, out, shippable.size(), "출고할 번호");
    if (!selected.has_value()) {
        return;
    }
    auto result = controller_.ship(shippable[*selected].id);
    if (result.ok) {
        out << "출고 완료 (RELEASE)\n";
    } else {
        out << "출고 실패: " << result.error << "\n";
    }
}

} // namespace productionclerk
