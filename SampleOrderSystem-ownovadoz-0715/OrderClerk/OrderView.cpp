#include "OrderView.h"

namespace orderclerk {

OrderView::OrderView(OrderController& controller) : controller_(controller) {}

void OrderView::showReserveScreen(std::istream& in, std::ostream& out) {
    out << "시료 ID > ";
    std::string sampleId;
    std::getline(in, sampleId);

    out << "고객명 > ";
    std::string customerName;
    std::getline(in, customerName);

    out << "주문 수량 > ";
    int quantity = 0;
    in >> quantity;
    in.ignore();

    auto result = controller_.createOrder(sampleId, customerName, quantity);
    if (!result.ok) {
        out << "접수 실패: " << result.error << "\n";
        return;
    }

    auto sample = controller_.getSampleInfo(sampleId);
    out << "예약 접수 완료\n";
    out << "주문번호 " << result.orderId << "\n";
    out << "시료 " << (sample.has_value() ? sample->name : sampleId) << "\n";
    out << "고객 " << customerName << "\n";
    out << "수량 " << quantity << " ea\n";
    out << "현재 상태 RESERVED\n";
}

} // namespace orderclerk
