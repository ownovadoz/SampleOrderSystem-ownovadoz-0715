#include "ProductionQueueModel.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <chrono>

using namespace productionline;
using namespace common;

TEST(ProductionLineTest, ProductionQueueModel_LoadFromMissingFile_StartsEmpty) {
    ProductionQueueModel model("nonexistent_queue_test.json");
    model.load();
    EXPECT_TRUE(model.empty());
}

TEST(ProductionLineTest, ProductionQueueModel_PushAndFrontAndPop) {
    ProductionQueueModel model("queue_test_push.json");
    ProductionJob job{"ORD-1", "S-001", 10, 10, Duration(600), std::chrono::system_clock::from_time_t(1000)};
    model.pushBack(job);
    EXPECT_FALSE(model.empty());
    EXPECT_EQ(model.front().orderId, "ORD-1");
    model.popFront();
    EXPECT_TRUE(model.empty());
}

TEST(ProductionLineTest, ProductionQueueModel_SaveThenLoad_RoundTrips) {
    const std::string path = "queue_test_roundtrip.json";
    std::remove(path.c_str());
    {
        ProductionQueueModel model(path);
        model.pushBack(ProductionJob{"ORD-1", "S-001", 10, 10, Duration(600),
                                      std::chrono::system_clock::from_time_t(1000)});
        model.pushBack(ProductionJob{"ORD-2", "S-002", 5, 5, Duration(300), std::nullopt});
        EXPECT_TRUE(model.save());
    }
    {
        ProductionQueueModel model(path);
        model.load();
        auto snap = model.snapshot();
        ASSERT_EQ(snap.size(), 2u);
        EXPECT_TRUE(snap[0].startTime.has_value());
        EXPECT_FALSE(snap[1].startTime.has_value());
        EXPECT_EQ(snap[1].orderId, "ORD-2");
    }
    std::remove(path.c_str());
}
