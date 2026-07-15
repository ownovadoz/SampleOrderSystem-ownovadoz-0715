#include "OrderController.h"
#include "../SampleClerk/SampleController.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace orderclerk;
using namespace sampleclerk;
using namespace common;

namespace {
TimePoint dayZero() { return std::chrono::system_clock::from_time_t(0); } // 1970-01-01 UTC
}

TEST(OrderClerkTest, OrderController_CreateOrder_Succeeds) {
    SampleModel sampleModel("order_ctrl_test_samples1.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);

    OrderModel orderModel("order_ctrl_test_orders1.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto result = orderController.createOrder("S-001", "삼성전자", 100);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.orderId, "ORD-19700101-0001");

    auto order = orderController.getOrder(result.orderId);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->status, OrderStatus::RESERVED);
    EXPECT_EQ(order->quantity, 100);
}

TEST(OrderClerkTest, OrderController_CreateOrder_UnknownSample_Fails) {
    SampleModel sampleModel("order_ctrl_test_samples2.dat");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("order_ctrl_test_orders2.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto result = orderController.createOrder("S-999", "삼성전자", 10);
    EXPECT_FALSE(result.ok);
}

TEST(OrderClerkTest, OrderController_CreateOrder_InvalidQuantity_Fails) {
    SampleModel sampleModel("order_ctrl_test_samples3.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders3.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto zero = orderController.createOrder("S-001", "삼성전자", 0);
    EXPECT_FALSE(zero.ok);
    auto negative = orderController.createOrder("S-001", "삼성전자", -5);
    EXPECT_FALSE(negative.ok);
}

TEST(OrderClerkTest, OrderController_SequenceIncrementsWithinSameDay) {
    SampleModel sampleModel("order_ctrl_test_samples4.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders4.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto first = orderController.createOrder("S-001", "삼성전자", 10);
    auto second = orderController.createOrder("S-001", "SK하이닉스", 20);
    EXPECT_EQ(first.orderId, "ORD-19700101-0001");
    EXPECT_EQ(second.orderId, "ORD-19700101-0002");
}

TEST(OrderClerkTest, OrderController_SequenceResetsOnNewDay) {
    SampleModel sampleModel("order_ctrl_test_samples5.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders5.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    orderController.createOrder("S-001", "삼성전자", 10);
    clock.advance(std::chrono::seconds(24 * 60 * 60));
    auto nextDay = orderController.createOrder("S-001", "삼성전자", 5);
    EXPECT_EQ(nextDay.orderId, "ORD-19700102-0001");
}

TEST(OrderClerkTest, OrderController_GetReservedQuantity_SumsConfirmedAndProducingOnly) {
    SampleModel sampleModel("order_ctrl_test_samples6.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders6.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto reserved = orderController.createOrder("S-001", "A", 10);   // RESERVED - 제외
    auto confirmed = orderController.createOrder("S-001", "B", 20);  // CONFIRMED로 변경 - 합산
    auto producing = orderController.createOrder("S-001", "C", 30);  // PRODUCING으로 변경 - 합산
    auto released = orderController.createOrder("S-001", "D", 40);   // RELEASE로 변경 - 제외

    orderController.setOrderStatus(confirmed.orderId, OrderStatus::CONFIRMED);
    orderController.setOrderStatus(producing.orderId, OrderStatus::PRODUCING);
    orderController.setOrderStatus(released.orderId, OrderStatus::RELEASE);

    EXPECT_EQ(orderController.getReservedQuantity("S-001"), 50); // 20 + 30
}
