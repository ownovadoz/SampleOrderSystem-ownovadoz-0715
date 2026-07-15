#include <iostream>
#include "SampleClerk/SampleModel.h"
#include "SampleClerk/SampleController.h"
#include "SampleClerk/SampleView.h"
#include "Common/Clock.h"
#include "OrderClerk/OrderModel.h"
#include "OrderClerk/OrderController.h"
#include "OrderClerk/OrderView.h"
#include "ProductionLine/ProductionQueueModel.h"
#include "ProductionLine/ProductionLineController.h"
#include "ProductionLine/ProductionLineView.h"

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

namespace {

void runSampleMenu(sampleclerk::SampleView& sampleView) {
    while (true) {
        std::cout << "\n[시료 관리]\n";
        std::cout << "[1] 시료 등록  [2] 시료 조회  [3] 시료 검색  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        int choice = 0;
        if (!(std::cin >> choice)) return;
        std::cin.ignore();

        if (choice == 1) {
            sampleView.showRegisterScreen(std::cin, std::cout);
        } else if (choice == 2) {
            sampleView.showListScreen(std::cout);
        } else if (choice == 3) {
            sampleView.showSearchScreen(std::cin, std::cout);
        } else if (choice == 0) {
            return;
        }
    }
}

void runOrderMenu(orderclerk::OrderView& orderView) {
    while (true) {
        std::cout << "\n[주문(접수/승인/거절)]\n";
        std::cout << "[1] 시료 예약(주문 접수)  [2] 주문 승인 (준비 중)  [3] 주문 거절 (준비 중)  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        int choice = 0;
        if (!(std::cin >> choice)) return;
        std::cin.ignore();

        if (choice == 1) {
            orderView.showReserveScreen(std::cin, std::cout);
        } else if (choice == 0) {
            return;
        }
    }
}

void runProductionLineMenu(productionline::ProductionLineView& productionLineView) {
    while (true) {
        std::cout << "\n[생산 라인]\n";
        std::cout << "[1] 생산 현황  [2] 대기 주문 확인  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        int choice = 0;
        if (!(std::cin >> choice)) return;
        std::cin.ignore();

        if (choice == 1) {
            productionLineView.showCurrentJobScreen(std::cout);
        } else if (choice == 2) {
            productionLineView.showQueueScreen(std::cout);
        } else if (choice == 0) {
            return;
        }
    }
}

} // namespace

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

    productionline::ProductionQueueModel queueModel("production_queue.json");
    queueModel.load();
    productionline::ProductionLineController productionLineController(queueModel, sampleController,
                                                                        orderController, systemClock);
    productionline::ProductionLineView productionLineView(productionLineController);

    while (true) {
        std::cout << "\n반도체 시료 생산주문관리 시스템\n";
        std::cout << "[1] 시료 관리  [2] 주문(접수/승인/거절)  [3] 모니터링  [4] 출고 처리  [5] 생산 라인  [0] 종료\n";
        std::cout << "선택 > ";
        int choice = 0;
        if (!(std::cin >> choice)) break;
        std::cin.ignore();

        if (choice == 1) {
            runSampleMenu(sampleView);
        } else if (choice == 2) {
            runOrderMenu(orderView);
        } else if (choice == 3) {
            std::cout << "모니터링 메뉴는 아직 구현되지 않았습니다\n";
        } else if (choice == 4) {
            std::cout << "출고 처리 메뉴는 아직 구현되지 않았습니다\n";
        } else if (choice == 5) {
            runProductionLineMenu(productionLineView);
        } else if (choice == 0) {
            break;
        }
    }

    sampleModel.save();
    orderModel.save();
    queueModel.save();
    return 0;
}
#endif
