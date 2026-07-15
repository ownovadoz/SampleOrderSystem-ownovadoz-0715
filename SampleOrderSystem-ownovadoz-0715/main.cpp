#include <iostream>
#include "SampleClerk/SampleModel.h"
#include "SampleClerk/SampleController.h"
#include "SampleClerk/SampleView.h"
#include "Common/Clock.h"
#include "OrderClerk/OrderModel.h"
#include "OrderClerk/OrderController.h"
#include "OrderClerk/OrderView.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

void setupConsoleEncoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

} // namespace

#ifdef _DEBUG
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    setupConsoleEncoding();
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
#else

int main() {
    setupConsoleEncoding();

    sampleclerk::SampleModel sampleModel("samples.json");
    sampleModel.load();
    sampleclerk::SampleController sampleController(sampleModel);
    sampleclerk::SampleView sampleView(sampleController);

    common::SystemClock systemClock;
    orderclerk::OrderModel orderModel("orders.json");
    orderModel.load();
    orderclerk::OrderController orderController(orderModel, sampleController, systemClock);
    orderclerk::OrderView orderView(orderController);

    while (true) {
        std::cout << "\n반도체 시료 생산주문관리 시스템\n";
        std::cout << "[1] 시료 등록  [2] 시료 예약(주문 접수)  [3] 시료 조회  [4] 시료 검색  [0] 종료\n";
        std::cout << "선택 > ";
        int choice = 0;
        if (!(std::cin >> choice)) break;
        std::cin.ignore();

        if (choice == 1) {
            sampleView.showRegisterScreen(std::cin, std::cout);
        } else if (choice == 2) {
            orderView.showReserveScreen(std::cin, std::cout);
        } else if (choice == 3) {
            sampleView.showListScreen(std::cout);
        } else if (choice == 4) {
            sampleView.showSearchScreen(std::cin, std::cout);
        } else if (choice == 0) {
            break;
        }
    }

    sampleModel.save();
    orderModel.save();
    return 0;
}
#endif
