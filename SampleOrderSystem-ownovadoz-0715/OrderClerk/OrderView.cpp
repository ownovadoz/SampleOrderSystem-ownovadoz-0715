#include "OrderView.h"
#include "../Common/ConsoleInput.h"

namespace orderclerk {

OrderView::OrderView(OrderController& controller) : controller_(controller) {}

void OrderView::showReserveScreen(std::istream& in, std::ostream& out) {
    common::printScreenHeader(out, "시료 예약(주문 접수)");

    out << "시료 ID > ";
    auto sampleIdInput = common::readLine(in);
    if (!sampleIdInput.ok || sampleIdInput.cancelled) {
        out << "취소되었습니다\n";
        return;
    }

    out << "고객명 > ";
    auto customerNameInput = common::readLine(in);
    if (!customerNameInput.ok || customerNameInput.cancelled) {
        out << "취소되었습니다\n";
        return;
    }

    out << "주문 수량 > ";
    auto quantityInput = common::readInt(in);
    if (!quantityInput.ok) {
        out << (quantityInput.cancelled ? "취소되었습니다\n" : "잘못된 입력입니다\n");
        return;
    }

    auto result = controller_.createOrder(sampleIdInput.text, customerNameInput.text, quantityInput.value);
    if (!result.ok) {
        out << "접수 실패: " << result.error << "\n";
        return;
    }

    auto sample = controller_.getSampleInfo(sampleIdInput.text);
    out << "예약 접수 완료\n";
    out << "주문번호 " << result.orderId << "\n";
    out << "시료 " << (sample.has_value() ? sample->name : sampleIdInput.text) << "\n";
    out << "고객 " << customerNameInput.text << "\n";
    out << "수량 " << quantityInput.value << " ea\n";
    out << "현재 상태 RESERVED\n";
}

} // namespace orderclerk
