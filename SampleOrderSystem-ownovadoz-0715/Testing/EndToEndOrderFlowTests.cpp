// 각 모듈 단위 테스트와 달리, 이 파일은 시료 담당자 -> 주문 담당자 -> 생산 라인 -> 생산 담당자 ->
// 모니터링을 목(mock) 없이 실제로 연결해 전체 주문 흐름이 하나로 이어지는지 검증한다.
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include "../ProductionLine/ProductionLineController.h"
#include "../ProductionClerk/ProductionClerkController.h"
#include "../Monitoring/MonitoringController.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace sampleclerk;
using namespace orderclerk;
using namespace productionline;
using namespace productionclerk;
using namespace monitoring;
using namespace common;

TEST(EndToEndTest, FullOrderLifecycle_ReservationThroughShipment) {
    SampleModel sampleModel("e2e_test_samples.json");
    SampleController sampleController(sampleModel);
    // 수율 1.0 -> 실생산량 = 부족분, 생산시간 60초 -> 소요시간 계산이 단순해짐
    sampleController.registerSample("S-E2E", "E2E 테스트 시료", Duration(60), 1.0);

    OrderModel orderModel("e2e_test_orders.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);

    ProductionQueueModel queueModel("e2e_test_queue.json");
    ProductionLineController productionLineController(queueModel, sampleController, orderController, clock);

    ProductionClerkController productionClerkController(sampleController, orderController,
                                                          productionLineController);
    MonitoringController monitoringController(sampleController, orderController);

    // 1. 재고 0인 상태에서 10개 주문 접수 -> RESERVED
    auto createResult = orderController.createOrder("S-E2E", "삼성전자", 10);
    ASSERT_TRUE(createResult.ok);
    const std::string orderId = createResult.orderId;
    EXPECT_EQ(orderController.getOrder(orderId)->status, OrderStatus::RESERVED);

    // 2. 승인 -> 재고(0) < 수량(10)이라 생산 라인에 투입되고 PRODUCING으로 전환
    productionClerkController.approve(orderId);
    EXPECT_EQ(orderController.getOrder(orderId)->status, OrderStatus::PRODUCING);

    auto currentJob = productionLineController.getCurrentJob();
    ASSERT_TRUE(currentJob.has_value());
    EXPECT_EQ(currentJob->orderId, orderId);
    EXPECT_EQ(currentJob->actualQty, 10); // ceil(10 / 1.0)

    // 3. 이 시점의 모니터링: PRODUCING 1건, 재고는 0이라 "고갈"
    auto countsDuringProduction = monitoringController.getOrderCountsByStatus();
    EXPECT_EQ(countsDuringProduction[OrderStatus::PRODUCING], 1);
    auto overviewDuringProduction = monitoringController.getStockOverview();
    ASSERT_EQ(overviewDuringProduction.size(), 1u);
    EXPECT_EQ(overviewDuringProduction[0].label, "고갈");

    // 4. 생산 소요시간(10개 * 60초 = 600초)만큼 시간 경과 -> 생산 완료 처리
    clock.advance(std::chrono::seconds(600));
    EXPECT_FALSE(productionLineController.getCurrentJob().has_value()); // 큐가 비었어야 함
    EXPECT_EQ(sampleController.getStock("S-E2E"), 10);
    EXPECT_EQ(orderController.getOrder(orderId)->status, OrderStatus::CONFIRMED);

    // 5. 출고 처리 -> RELEASE + 재고 차감
    auto shipResult = productionClerkController.ship(orderId);
    EXPECT_TRUE(shipResult.ok);
    EXPECT_EQ(orderController.getOrder(orderId)->status, OrderStatus::RELEASE);
    EXPECT_EQ(sampleController.getStock("S-E2E"), 0);

    // 6. 최종 모니터링 상태: RELEASE 1건, 나머지 0건
    auto finalCounts = monitoringController.getOrderCountsByStatus();
    EXPECT_EQ(finalCounts[OrderStatus::RELEASE], 1);
    EXPECT_EQ(finalCounts[OrderStatus::RESERVED], 0);
    EXPECT_EQ(finalCounts[OrderStatus::CONFIRMED], 0);
    EXPECT_EQ(finalCounts[OrderStatus::PRODUCING], 0);
}

TEST(EndToEndTest, FullOrderLifecycle_SufficientStock_ConfirmsImmediatelyWithoutProductionLine) {
    SampleModel sampleModel("e2e_test_samples2.json");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-E2E2", "E2E 테스트 시료2", Duration(60), 1.0);
    sampleController.increaseStock("S-E2E2", 100); // 재고 충분

    OrderModel orderModel("e2e_test_orders2.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);

    ProductionQueueModel queueModel("e2e_test_queue2.json");
    ProductionLineController productionLineController(queueModel, sampleController, orderController, clock);
    ProductionClerkController productionClerkController(sampleController, orderController,
                                                          productionLineController);
    MonitoringController monitoringController(sampleController, orderController);

    auto createResult = orderController.createOrder("S-E2E2", "SK하이닉스", 10);
    ASSERT_TRUE(createResult.ok);

    // 재고가 충분하므로 승인 즉시 CONFIRMED, 생산 라인에는 아무것도 투입되지 않는다.
    productionClerkController.approve(createResult.orderId);
    EXPECT_EQ(orderController.getOrder(createResult.orderId)->status, OrderStatus::CONFIRMED);
    EXPECT_FALSE(productionLineController.getCurrentJob().has_value());

    auto shipResult = productionClerkController.ship(createResult.orderId);
    EXPECT_TRUE(shipResult.ok);
    EXPECT_EQ(sampleController.getStock("S-E2E2"), 90);

    auto finalCounts = monitoringController.getOrderCountsByStatus();
    EXPECT_EQ(finalCounts[OrderStatus::RELEASE], 1);
}
