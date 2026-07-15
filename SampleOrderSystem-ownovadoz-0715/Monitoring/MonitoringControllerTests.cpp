#include "MonitoringController.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include "../Testing/DummyDataGenerator.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace monitoring;
using namespace sampleclerk;
using namespace orderclerk;
using namespace common;
using testing_support::DummyDataGenerator;

namespace {
TimePoint dayZero() { return std::chrono::system_clock::from_time_t(0); }
}

TEST(MonitoringTest, GetOrderCountsByStatus_ExcludesRejectedAndCountsCorrectly) {
    SampleModel sampleModel("mon_test_samples1.dat");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);

    OrderModel orderModel("mon_test_orders1.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto reserved = orderController.createOrder(sample.id, "A", 1);
    auto confirmed = orderController.createOrder(sample.id, "B", 1);
    auto producing = orderController.createOrder(sample.id, "C", 1);
    auto released = orderController.createOrder(sample.id, "D", 1);
    auto rejected = orderController.createOrder(sample.id, "E", 1);
    orderController.setOrderStatus(confirmed.orderId, OrderStatus::CONFIRMED);
    orderController.setOrderStatus(producing.orderId, OrderStatus::PRODUCING);
    orderController.setOrderStatus(released.orderId, OrderStatus::RELEASE);
    orderController.setOrderStatus(rejected.orderId, OrderStatus::REJECTED);

    MonitoringController monitoringController(sampleController, orderController);
    auto counts = monitoringController.getOrderCountsByStatus();

    EXPECT_EQ(counts[OrderStatus::RESERVED], 1);
    EXPECT_EQ(counts[OrderStatus::CONFIRMED], 1);
    EXPECT_EQ(counts[OrderStatus::PRODUCING], 1);
    EXPECT_EQ(counts[OrderStatus::RELEASE], 1);
    EXPECT_EQ(counts.find(OrderStatus::REJECTED), counts.end());
}

TEST(MonitoringTest, GetStockOverview_LabelsDepletedShortageAndSufficient) {
    SampleModel sampleModel("mon_test_samples2.dat");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;

    auto depleted = gen.sample(1);
    sampleController.registerSample(depleted.id, depleted.name, depleted.avgProductionTime, depleted.yield);
    // 재고 0 그대로 (고갈)

    auto shortage = gen.sample(2);
    sampleController.registerSample(shortage.id, shortage.name, shortage.avgProductionTime, shortage.yield);
    sampleController.increaseStock(shortage.id, 5); // 재고 5

    auto sufficient = gen.sample(3);
    sampleController.registerSample(sufficient.id, sufficient.name, sufficient.avgProductionTime, sufficient.yield);
    sampleController.increaseStock(sufficient.id, 100); // 재고 100

    OrderModel orderModel("mon_test_orders2.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    orderController.createOrder(shortage.id, "고객", 10); // 수요 10 > 재고 5 -> 부족
    orderController.createOrder(sufficient.id, "고객", 10); // 수요 10 < 재고 100 -> 여유

    MonitoringController monitoringController(sampleController, orderController);
    auto overview = monitoringController.getStockOverview();

    auto findLabel = [&](const std::string& id) -> std::string {
        for (const auto& item : overview) {
            if (item.sample.id == id) return item.label;
        }
        return "";
    };
    EXPECT_EQ(findLabel(depleted.id), "고갈");
    EXPECT_EQ(findLabel(shortage.id), "부족");
    EXPECT_EQ(findLabel(sufficient.id), "여유");
}

TEST(MonitoringTest, SearchStockOverview_CaseInsensitiveSubstringOnNameOrId) {
    SampleModel sampleModel("mon_test_samples3.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-777", "Silicon Wafer", Duration(1800), 0.9);
    sampleController.registerSample("S-888", "GaN Epitaxial", Duration(1200), 0.8);

    OrderModel orderModel("mon_test_orders3.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    MonitoringController monitoringController(sampleController, orderController);

    auto byName = monitoringController.searchStockOverview("silicon");
    ASSERT_EQ(byName.size(), 1u);
    EXPECT_EQ(byName[0].sample.id, "S-777");

    auto byId = monitoringController.searchStockOverview("888");
    ASSERT_EQ(byId.size(), 1u);
    EXPECT_EQ(byId[0].sample.id, "S-888");

    auto none = monitoringController.searchStockOverview("존재하지않음");
    EXPECT_TRUE(none.empty());
}
