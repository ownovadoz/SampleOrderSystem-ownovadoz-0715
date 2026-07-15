#include <iostream>
#include "SampleClerk/SampleModel.h"
#include "SampleClerk/SampleController.h"
#include "SampleClerk/SampleView.h"
#include "Common/Clock.h"
#include "Common/ConsoleInput.h"
#include "OrderClerk/OrderModel.h"
#include "OrderClerk/OrderController.h"
#include "OrderClerk/OrderView.h"
#include "ProductionLine/ProductionQueueModel.h"
#include "ProductionLine/ProductionLineController.h"
#include "ProductionLine/ProductionLineView.h"
#include "ProductionClerk/ProductionClerkController.h"
#include "ProductionClerk/ProductionClerkView.h"
#include "Monitoring/MonitoringController.h"
#include "Monitoring/MonitoringView.h"
#include "Testing/DummySeeder.h"
#include <cstdlib>
#include <string>

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

// 메뉴 선택 번호를 안전하게 읽는다. 숫자가 아닌 입력이 들어와도 스트림을 복구해 프로그램이 죽지 않고,
// 잘못된 입력이라는 안내만 보여준 뒤 같은 메뉴를 다시 그리게 한다. 반환값이 없으면(nullopt) 이 메뉴를
// 벗어나야 한다는 뜻이다(취소 또는 스트림 종료).
std::optional<int> readMenuChoice(std::ostream& out) {
    auto result = common::readInt(std::cin);
    if (!result.ok) {
        if (!result.cancelled) {
            out << "잘못된 입력입니다. 다시 선택해주세요.\n";
        }
        return std::nullopt;
    }
    return result.value;
}

void runSampleMenu(sampleclerk::SampleView& sampleView) {
    while (true) {
        std::cout << "\n[시료 관리]\n";
        std::cout << "[1] 시료 등록  [2] 시료 조회  [3] 시료 검색  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        auto choice = readMenuChoice(std::cout);
        if (!choice.has_value()) continue;

        if (*choice == 1) {
            sampleView.showRegisterScreen(std::cin, std::cout);
        } else if (*choice == 2) {
            sampleView.showListScreen(std::cout);
        } else if (*choice == 3) {
            sampleView.showSearchScreen(std::cin, std::cout);
        } else if (*choice == 0) {
            return;
        } else {
            std::cout << "잘못된 메뉴 번호입니다.\n";
        }
    }
}

void runOrderMenu(orderclerk::OrderView& orderView, productionclerk::ProductionClerkView& productionClerkView) {
    while (true) {
        std::cout << "\n[주문(접수/승인/거절)]\n";
        std::cout << "[1] 시료 예약(주문 접수)  [2] 주문 승인  [3] 주문 거절  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        auto choice = readMenuChoice(std::cout);
        if (!choice.has_value()) continue;

        if (*choice == 1) {
            orderView.showReserveScreen(std::cin, std::cout);
        } else if (*choice == 2) {
            productionClerkView.showApprovalScreen(std::cin, std::cout);
        } else if (*choice == 3) {
            productionClerkView.showRejectionScreen(std::cin, std::cout);
        } else if (*choice == 0) {
            return;
        } else {
            std::cout << "잘못된 메뉴 번호입니다.\n";
        }
    }
}

void runProductionLineMenu(productionline::ProductionLineView& productionLineView) {
    while (true) {
        std::cout << "\n[생산 라인]\n";
        std::cout << "[1] 생산 현황  [2] 대기 주문 확인  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        auto choice = readMenuChoice(std::cout);
        if (!choice.has_value()) continue;

        if (*choice == 1) {
            productionLineView.showCurrentJobScreen(std::cout);
        } else if (*choice == 2) {
            productionLineView.showQueueScreen(std::cout);
        } else if (*choice == 0) {
            return;
        } else {
            std::cout << "잘못된 메뉴 번호입니다.\n";
        }
    }
}

void runShipmentMenu(productionclerk::ProductionClerkView& productionClerkView) {
    while (true) {
        std::cout << "\n[출고 처리]\n";
        std::cout << "[1] 출고 처리  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        auto choice = readMenuChoice(std::cout);
        if (!choice.has_value()) continue;

        if (*choice == 1) {
            productionClerkView.showShipmentScreen(std::cin, std::cout);
        } else if (*choice == 0) {
            return;
        } else {
            std::cout << "잘못된 메뉴 번호입니다.\n";
        }
    }
}

void runMonitoringMenu(monitoring::MonitoringView& monitoringView) {
    while (true) {
        std::cout << "\n[모니터링]\n";
        std::cout << "[1] 주문량 확인  [2] 재고량 확인  [3] 재고 검색  [0] 이전 메뉴\n";
        std::cout << "선택 > ";
        auto choice = readMenuChoice(std::cout);
        if (!choice.has_value()) continue;

        if (*choice == 1) {
            monitoringView.showOrderCountsScreen(std::cout);
        } else if (*choice == 2) {
            monitoringView.showStockOverviewScreen(std::cout);
        } else if (*choice == 3) {
            monitoringView.showStockSearchScreen(std::cin, std::cout);
        } else if (*choice == 0) {
            return;
        } else {
            std::cout << "잘못된 메뉴 번호입니다.\n";
        }
    }
}

// 더미 데이터 생성 모드: `SampleOrderSystem-ownovadoz-0715.exe --seed <시료 개수> [주문 개수]`로 실행하면
// samples.json/orders.json/production_queue.json에 서로 앞뒤가 맞는 더미 데이터를 이어서 추가하고 즉시
// 종료한다. PRD.md 3장에 따라 일반 메뉴에는 노출하지 않는다 — 담당자가 앱을 수동으로 테스트해볼 때만
// 이 인자를 직접 붙여 실행한다.
int runDummySeedMode(int sampleCount, int orderCount) {
    sampleclerk::SampleModel sampleModel("samples.json");
    sampleModel.load();
    orderclerk::OrderModel orderModel("orders.json");
    orderModel.load();
    productionline::ProductionQueueModel queueModel("production_queue.json");
    queueModel.load();
    common::SystemClock clock;

    auto result = testing_support::seedDummyData(sampleModel, orderModel, queueModel, clock, sampleCount,
                                                  orderCount);

    sampleModel.save();
    orderModel.save();
    queueModel.save();

    std::cout << "더미 데이터 생성 완료: 시료 " << result.samplesCreated << "건, 주문 " << result.ordersCreated
              << "건, 생산 큐 " << result.productionJobsCreated << "건 (samples.json/orders.json/"
              << "production_queue.json에 이어서 저장됨)\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    setupConsoleEncoding();

    if (argc > 1 && std::string(argv[1]) == "--seed") {
        int sampleCount = argc > 2 ? std::atoi(argv[2]) : 5;
        int orderCount = argc > 3 ? std::atoi(argv[3]) : sampleCount * 3;
        return runDummySeedMode(sampleCount, orderCount);
    }

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

    productionclerk::ProductionClerkController productionClerkController(sampleController, orderController,
                                                                           productionLineController);
    productionclerk::ProductionClerkView productionClerkView(productionClerkController);

    monitoring::MonitoringController monitoringController(sampleController, orderController);
    monitoring::MonitoringView monitoringView(monitoringController);

    while (true) {
        std::cout << "\n반도체 시료 생산주문관리 시스템\n";
        std::cout << "[1] 시료 관리  [2] 주문(접수/승인/거절)  [3] 모니터링  [4] 출고 처리  [5] 생산 라인  [0] 종료\n";
        std::cout << "선택 > ";
        auto choice = readMenuChoice(std::cout);
        if (!choice.has_value()) {
            if (!std::cin) break; // 입력 스트림이 완전히 끝났으면 프로그램을 종료한다.
            continue;
        }

        if (*choice == 1) {
            runSampleMenu(sampleView);
        } else if (*choice == 2) {
            runOrderMenu(orderView, productionClerkView);
        } else if (*choice == 3) {
            runMonitoringMenu(monitoringView);
        } else if (*choice == 4) {
            runShipmentMenu(productionClerkView);
        } else if (*choice == 5) {
            runProductionLineMenu(productionLineView);
        } else if (*choice == 0) {
            break;
        } else {
            std::cout << "잘못된 메뉴 번호입니다.\n";
        }
    }

    sampleModel.save();
    orderModel.save();
    queueModel.save();
    return 0;
}
#endif
