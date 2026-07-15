#include "OrderView.h"
#include "../SampleClerk/SampleController.h"
#include "../Testing/DummyDataGenerator.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace orderclerk;
using namespace sampleclerk;
using namespace common;
using testing_support::DummyDataGenerator;

TEST(OrderClerkTest, OrderView_ReserveScreen_SuccessShowsOrderInfo) {
    SampleModel sampleModel("orderview_test_samples1.json");
    SampleController sampleController(sampleModel);
    DummyDataGenerator gen;
    auto sample = gen.sample(1);
    sampleController.registerSample(sample.id, sample.name, sample.avgProductionTime, sample.yield);
    OrderModel orderModel("orderview_test_orders1.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    OrderView view(orderController);

    std::istringstream in(sample.id + "\n삼성전자 파운드리\n200\n");
    std::ostringstream out;
    view.showReserveScreen(in, out);

    EXPECT_NE(out.str().find("예약 접수 완료"), std::string::npos);
    EXPECT_NE(out.str().find("ORD-19700101-0001"), std::string::npos);
    EXPECT_NE(out.str().find(sample.name), std::string::npos);
    EXPECT_NE(out.str().find("RESERVED"), std::string::npos);
}

TEST(OrderClerkTest, OrderView_ReserveScreen_UnknownSample_ShowsError) {
    SampleModel sampleModel("orderview_test_samples2.json");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("orderview_test_orders2.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    OrderView view(orderController);

    std::istringstream in("S-999\n삼성전자\n10\n");
    std::ostringstream out;
    view.showReserveScreen(in, out);

    EXPECT_NE(out.str().find("접수 실패"), std::string::npos);
}
