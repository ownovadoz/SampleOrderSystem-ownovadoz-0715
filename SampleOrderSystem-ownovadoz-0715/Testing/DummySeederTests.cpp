#include "DummySeeder.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <set>

using namespace testing_support;
using namespace sampleclerk;
using namespace orderclerk;
using namespace productionline;
using namespace common;

TEST(DummySeederTest, SeedsRequestedNumberOfSamplesAndOrders) {
    SampleModel sampleModel("seeder_test_samples1.json");
    OrderModel orderModel("seeder_test_orders1.json");
    ProductionQueueModel queueModel("seeder_test_queue1.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));

    auto result = seedDummyData(sampleModel, orderModel, queueModel, clock, 3, 5);

    EXPECT_EQ(result.samplesCreated, 3);
    EXPECT_EQ(result.ordersCreated, 5);
    EXPECT_EQ(sampleModel.findAll().size(), 3u);
    EXPECT_EQ(orderModel.findAll().size(), 5u);
}

TEST(DummySeederTest, EveryOrderReferencesAnExistingSample) {
    SampleModel sampleModel("seeder_test_samples2.json");
    OrderModel orderModel("seeder_test_orders2.json");
    ProductionQueueModel queueModel("seeder_test_queue2.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));

    seedDummyData(sampleModel, orderModel, queueModel, clock, 3, 10);

    std::set<std::string> sampleIds;
    for (const auto& sample : sampleModel.findAll()) {
        sampleIds.insert(sample.id);
    }
    for (const auto& order : orderModel.findAll()) {
        EXPECT_TRUE(sampleIds.count(order.sampleId) > 0) << "order references unknown sample " << order.sampleId;
    }
}

TEST(DummySeederTest, ProducingOrdersHaveMatchingProductionJob) {
    SampleModel sampleModel("seeder_test_samples3.json");
    OrderModel orderModel("seeder_test_orders3.json");
    ProductionQueueModel queueModel("seeder_test_queue3.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));

    seedDummyData(sampleModel, orderModel, queueModel, clock, 3, 20);

    std::set<std::string> queuedOrderIds;
    for (const auto& job : queueModel.snapshot()) {
        queuedOrderIds.insert(job.orderId);
    }
    for (const auto& order : orderModel.findAll()) {
        if (order.status == OrderStatus::PRODUCING) {
            EXPECT_TRUE(queuedOrderIds.count(order.id) > 0)
                << "PRODUCING order " << order.id << " has no production queue entry";
        }
    }
}

TEST(DummySeederTest, ExistingDataIsKeptAndSeedContinuesAfterMaxIndex) {
    SampleModel sampleModel("seeder_test_samples4.json");
    sampleModel.insert(Sample{"S-005", "기존 시료", Duration(600), 0.9, 10});
    OrderModel orderModel("seeder_test_orders4.json");
    ProductionQueueModel queueModel("seeder_test_queue4.json");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));

    seedDummyData(sampleModel, orderModel, queueModel, clock, 2, 3);

    auto all = sampleModel.findAll();
    EXPECT_EQ(all.size(), 3u); // 기존 1개 + 새로 만든 2개
    bool foundOriginal = std::any_of(all.begin(), all.end(), [](const Sample& s) { return s.id == "S-005"; });
    EXPECT_TRUE(foundOriginal);
    bool foundNewAfterMax =
        std::any_of(all.begin(), all.end(), [](const Sample& s) { return s.id == "S-006"; });
    EXPECT_TRUE(foundNewAfterMax);
}
