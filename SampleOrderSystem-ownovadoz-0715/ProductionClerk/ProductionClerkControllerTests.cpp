#include "ProductionClerkController.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include "../ProductionLine/ProductionLineController.h"
#include "../Testing/DummyDataGenerator.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace productionclerk;
using namespace sampleclerk;
using namespace orderclerk;
using namespace productionline;
using namespace common;
using testing_support::DummyDataGenerator;

namespace {
TimePoint dayZero() { return std::chrono::system_clock::from_time_t(0); }
}

TEST(ProductionClerkTest, Approve_WithSufficientStock_SetsConfirmed) {
    SampleModel sampleModel("pc_test_samples1.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);
    sampleController.increaseStock(sample.id, 100); // 재고 충분

    OrderModel orderModel("pc_test_orders1.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder(sample.id, "삼성전자", 10);

    ProductionQueueModel queueModel("pc_test_queue1.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);

    ProductionClerkController pcController(sampleController, orderController, plController);
    pcController.approve(order.orderId);

    auto updated = orderController.getOrder(order.orderId);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status, OrderStatus::CONFIRMED);
}

TEST(ProductionClerkTest, Approve_WithInsufficientStock_SetsProducingAndEnqueues) {
    SampleModel sampleModel("pc_test_samples2.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);
    // 재고 0 (부족)

    OrderModel orderModel("pc_test_orders2.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder(sample.id, "삼성전자", 10);

    ProductionQueueModel queueModel("pc_test_queue2.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);

    ProductionClerkController pcController(sampleController, orderController, plController);
    pcController.approve(order.orderId);

    auto updated = orderController.getOrder(order.orderId);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status, OrderStatus::PRODUCING);

    auto currentJob = plController.getCurrentJob();
    ASSERT_TRUE(currentJob.has_value());
    EXPECT_EQ(currentJob->orderId, order.orderId);
}

TEST(ProductionClerkTest, Reject_SetsRejected) {
    SampleModel sampleModel("pc_test_samples3.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);

    OrderModel orderModel("pc_test_orders3.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder(sample.id, "삼성전자", 10);

    ProductionQueueModel queueModel("pc_test_queue3.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);

    pcController.reject(order.orderId);

    auto updated = orderController.getOrder(order.orderId);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status, OrderStatus::REJECTED);
}

TEST(ProductionClerkTest, Ship_ConfirmedOrder_SetsReleaseAndDecreasesStock) {
    SampleModel sampleModel("pc_test_samples4.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);
    sampleController.increaseStock(sample.id, 100);

    OrderModel orderModel("pc_test_orders4.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder(sample.id, "삼성전자", 10);
    orderController.setOrderStatus(order.orderId, OrderStatus::CONFIRMED);

    ProductionQueueModel queueModel("pc_test_queue4.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);

    auto result = pcController.ship(order.orderId);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(sampleController.getStock(sample.id), 90);

    auto updated = orderController.getOrder(order.orderId);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status, OrderStatus::RELEASE);
}

TEST(ProductionClerkTest, Ship_NonConfirmedOrder_Fails) {
    SampleModel sampleModel("pc_test_samples5.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);

    OrderModel orderModel("pc_test_orders5.json");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder(sample.id, "삼성전자", 10); // RESERVED 상태 그대로

    ProductionQueueModel queueModel("pc_test_queue5.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);

    auto result = pcController.ship(order.orderId);
    EXPECT_FALSE(result.ok);
}
