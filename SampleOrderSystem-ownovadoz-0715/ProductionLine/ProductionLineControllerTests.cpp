#include "ProductionLineController.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace productionline;
using namespace sampleclerk;
using namespace orderclerk;
using namespace common;

namespace {
TimePoint dayZero() { return std::chrono::system_clock::from_time_t(0); }
}

TEST(ProductionLineTest, ProductionLineController_Enqueue_StartsImmediatelyWhenQueueEmpty) {
    SampleModel sampleModel("pl_test_samples1.json");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders1.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder("S-001", "삼성전자", 10);
    orderController.setOrderStatus(order.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue1.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);

    plController.enqueue("S-001", 10, order.orderId);

    auto current = plController.getCurrentJob();
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->orderId, order.orderId);
    EXPECT_EQ(current->actualQty, 10); // ceil(10 / 1.0)
}

TEST(ProductionLineTest, ProductionLineController_ProcessCompletion_UpdatesStockAndOrderStatus) {
    SampleModel sampleModel("pl_test_samples2.json");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders2.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder("S-001", "삼성전자", 10);
    orderController.setOrderStatus(order.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue2.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 10, order.orderId); // 실생산량 10, 총 소요 600초

    clock.advance(std::chrono::seconds(600));

    auto current = plController.getCurrentJob();
    EXPECT_FALSE(current.has_value()); // 완료되어 큐가 비었음

    EXPECT_EQ(sampleController.getStock("S-001"), 10);
    EXPECT_EQ(orderController.getOrder(order.orderId)->status, OrderStatus::CONFIRMED);
}

TEST(ProductionLineTest, ProductionLineController_MultipleCompletions_ProcessedInFifoOrder) {
    SampleModel sampleModel("pl_test_samples3.json");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders3.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order1 = orderController.createOrder("S-001", "A", 5);  // 실생산량 5, 300초
    auto order2 = orderController.createOrder("S-001", "B", 3);  // 실생산량 3, 180초
    orderController.setOrderStatus(order1.orderId, OrderStatus::PRODUCING);
    orderController.setOrderStatus(order2.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue3.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 5, order1.orderId);
    plController.enqueue("S-001", 3, order2.orderId);

    clock.advance(std::chrono::seconds(300 + 180));

    auto current = plController.getCurrentJob();
    EXPECT_FALSE(current.has_value());
    EXPECT_EQ(sampleController.getStock("S-001"), 8); // 5 + 3
    EXPECT_EQ(orderController.getOrder(order1.orderId)->status, OrderStatus::CONFIRMED);
    EXPECT_EQ(orderController.getOrder(order2.orderId)->status, OrderStatus::CONFIRMED);
}

TEST(ProductionLineTest, ProductionLineController_GetQueue_ShowsWaitingJobsExcludingCurrent) {
    SampleModel sampleModel("pl_test_samples4.json");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders4.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order1 = orderController.createOrder("S-001", "A", 5);
    auto order2 = orderController.createOrder("S-001", "B", 3);
    orderController.setOrderStatus(order1.orderId, OrderStatus::PRODUCING);
    orderController.setOrderStatus(order2.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue4.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 5, order1.orderId);
    plController.enqueue("S-001", 3, order2.orderId);

    auto queue = plController.getQueue();
    ASSERT_EQ(queue.size(), 1u);
    EXPECT_EQ(queue[0].orderId, order2.orderId);
}
