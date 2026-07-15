#include "ProductionClerkView.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include "../ProductionLine/ProductionLineController.h"
#include "../Testing/DummyDataGenerator.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace productionclerk;
using namespace sampleclerk;
using namespace orderclerk;
using namespace productionline;
using namespace common;
using testing_support::DummyDataGenerator;

TEST(ProductionClerkTest, ApprovalScreen_NoOrders_ShowsEmptyMessage) {
    SampleModel sampleModel("pcview_test_samples1.json");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("pcview_test_orders1.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    ProductionQueueModel queueModel("pcview_test_queue1.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);
    ProductionClerkView view(pcController);

    std::istringstream in("");
    std::ostringstream out;
    view.showApprovalScreen(in, out);
    EXPECT_NE(out.str().find("승인 대기 주문이 없습니다"), std::string::npos);
}

TEST(ProductionClerkTest, ApprovalScreen_SelectOrder_ShowsConfirmedStatus) {
    SampleModel sampleModel("pcview_test_samples2.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);
    sampleController.increaseStock(sample.id, 100);
    OrderModel orderModel("pcview_test_orders2.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder(sample.id, "삼성전자", 10);
    ProductionQueueModel queueModel("pcview_test_queue2.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);
    ProductionClerkView view(pcController);

    std::istringstream in("1\n");
    std::ostringstream out;
    view.showApprovalScreen(in, out);
    EXPECT_NE(out.str().find("CONFIRMED"), std::string::npos);
}

TEST(ProductionClerkTest, ApprovalScreen_CancelledAtSelection_DoesNotApprove) {
    SampleModel sampleModel("pcview_test_samples4.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);
    sampleController.increaseStock(sample.id, 100);
    OrderModel orderModel("pcview_test_orders4.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder(sample.id, "삼성전자", 10);
    ProductionQueueModel queueModel("pcview_test_queue4.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);
    ProductionClerkView view(pcController);

    std::istringstream in("q\n");
    std::ostringstream out;
    view.showApprovalScreen(in, out);
    EXPECT_NE(out.str().find("취소"), std::string::npos);
    auto updated = orderController.getOrder(order.orderId);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->status, OrderStatus::RESERVED);
}

TEST(ProductionClerkTest, ApprovalScreen_InvalidSelection_ShowsErrorWithoutCrashing) {
    SampleModel sampleModel("pcview_test_samples5.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);
    OrderModel orderModel("pcview_test_orders5.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    orderController.createOrder(sample.id, "삼성전자", 10);
    ProductionQueueModel queueModel("pcview_test_queue5.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);
    ProductionClerkView view(pcController);

    std::istringstream in("이상한값\n");
    std::ostringstream out;
    view.showApprovalScreen(in, out);
    EXPECT_NE(out.str().find("잘못된 입력"), std::string::npos);
}

TEST(ProductionClerkTest, ShipmentScreen_NoOrders_ShowsEmptyMessage) {
    SampleModel sampleModel("pcview_test_samples3.json");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("pcview_test_orders3.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    ProductionQueueModel queueModel("pcview_test_queue3.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionClerkController pcController(sampleController, orderController, plController);
    ProductionClerkView view(pcController);

    std::istringstream in("");
    std::ostringstream out;
    view.showShipmentScreen(in, out);
    EXPECT_NE(out.str().find("출고 가능한 주문이 없습니다"), std::string::npos);
}
