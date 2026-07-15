#include "ProductionLineView.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace productionline;
using namespace sampleclerk;
using namespace orderclerk;
using namespace common;

TEST(ProductionLineTest, ProductionLineView_CurrentJobScreen_EmptyQueueShowsMessage) {
    SampleModel sampleModel("plview_test_samples1.json");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("plview_test_orders1.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    ProductionQueueModel queueModel("plview_test_queue1.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionLineView view(plController);

    std::ostringstream out;
    view.showCurrentJobScreen(out);
    EXPECT_NE(out.str().find("대기 중인 생산 없음"), std::string::npos);
}

TEST(ProductionLineTest, ProductionLineView_CurrentJobScreen_ShowsRunningJob) {
    SampleModel sampleModel("plview_test_samples2.json");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);
    OrderModel orderModel("plview_test_orders2.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder("S-001", "삼성전자", 10);
    orderController.setOrderStatus(order.orderId, OrderStatus::PRODUCING);
    ProductionQueueModel queueModel("plview_test_queue2.json");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 10, order.orderId);
    ProductionLineView view(plController);

    std::ostringstream out;
    view.showCurrentJobScreen(out);
    EXPECT_NE(out.str().find(order.orderId), std::string::npos);
    EXPECT_NE(out.str().find("실리콘 웨이퍼"), std::string::npos);
}
