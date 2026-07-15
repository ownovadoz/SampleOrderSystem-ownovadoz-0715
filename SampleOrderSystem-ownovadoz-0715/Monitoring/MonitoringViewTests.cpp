#include "MonitoringView.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace monitoring;
using namespace sampleclerk;
using namespace orderclerk;
using namespace common;

TEST(MonitoringTest, OrderCountsScreen_NoOrders_ShowsZeroCounts) {
    SampleModel sampleModel("monview_test_samples1.json");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("monview_test_orders1.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    MonitoringController monitoringController(sampleController, orderController);
    MonitoringView view(monitoringController);

    std::ostringstream out;
    view.showOrderCountsScreen(out);
    EXPECT_NE(out.str().find("RESERVED"), std::string::npos);
    EXPECT_NE(out.str().find("0"), std::string::npos);
}

TEST(MonitoringTest, StockOverviewScreen_NoSamples_ShowsEmptyMessage) {
    SampleModel sampleModel("monview_test_samples2.json");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("monview_test_orders2.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    MonitoringController monitoringController(sampleController, orderController);
    MonitoringView view(monitoringController);

    std::ostringstream out;
    view.showStockOverviewScreen(out);
    EXPECT_NE(out.str().find("등록된 시료가 없습니다"), std::string::npos);
}

TEST(MonitoringTest, StockSearchScreen_NoResults_ShowsMessage) {
    SampleModel sampleModel("monview_test_samples3.json");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("monview_test_orders3.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    MonitoringController monitoringController(sampleController, orderController);
    MonitoringView view(monitoringController);

    std::istringstream in("존재하지않는이름\n");
    std::ostringstream out;
    view.showStockSearchScreen(in, out);
    EXPECT_NE(out.str().find("검색 결과 없음"), std::string::npos);
}
