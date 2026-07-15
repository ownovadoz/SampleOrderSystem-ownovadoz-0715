#include "SampleController.h"
#include "../Testing/DummyDataGenerator.h"
#include <gtest/gtest.h>

using namespace sampleclerk;
using namespace common;
using testing_support::DummyDataGenerator;

TEST(SampleClerkTest, SampleController_RegisterSample_Succeeds) {
    SampleModel model("controller_test_register.dat");
    SampleController controller(model);
    DummyDataGenerator gen;
    auto s = gen.sample(1);
    auto result = controller.registerSample(s.id, s.name, s.avgProductionTime, s.yield);
    EXPECT_TRUE(result.ok);
    auto sample = controller.getSample(s.id);
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(sample->stock, 0);
}

TEST(SampleClerkTest, SampleController_RegisterDuplicateId_Fails) {
    SampleModel model("controller_test_dup.dat");
    SampleController controller(model);
    DummyDataGenerator gen;
    auto s1 = gen.sample(1);
    auto s2 = gen.sample(2);
    controller.registerSample(s1.id, s1.name, s1.avgProductionTime, s1.yield);
    auto result = controller.registerSample(s1.id, s2.name, s2.avgProductionTime, s2.yield);
    EXPECT_FALSE(result.ok);
}

TEST(SampleClerkTest, SampleController_RegisterInvalidYield_Fails) {
    SampleModel model("controller_test_yield.dat");
    SampleController controller(model);
    auto tooHigh = controller.registerSample("S-001", "샘플", Duration(1800), 1.5);
    EXPECT_FALSE(tooHigh.ok);
    auto zero = controller.registerSample("S-002", "샘플2", Duration(1800), 0.0);
    EXPECT_FALSE(zero.ok);
}

TEST(SampleClerkTest, SampleController_SearchSamples_CaseInsensitiveSubstring) {
    SampleModel model("controller_test_search.dat");
    SampleController controller(model);
    controller.registerSample("S-001", "Silicon Wafer", Duration(1800), 0.9);
    controller.registerSample("S-002", "GaN Epitaxial", Duration(1200), 0.8);
    auto results = controller.searchSamples("silicon");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "S-001");
}

TEST(SampleClerkTest, SampleController_IncreaseAndDecreaseStock) {
    SampleModel model("controller_test_stock.dat");
    SampleController controller(model);
    DummyDataGenerator gen;
    auto s = gen.sample(1);
    controller.registerSample(s.id, s.name, s.avgProductionTime, s.yield);
    EXPECT_TRUE(controller.increaseStock(s.id, 50).ok);
    EXPECT_EQ(controller.getStock(s.id), 50);
    EXPECT_TRUE(controller.decreaseStock(s.id, 20).ok);
    EXPECT_EQ(controller.getStock(s.id), 30);
}

TEST(SampleClerkTest, SampleController_DecreaseStockBelowZero_Fails) {
    SampleModel model("controller_test_stock_fail.dat");
    SampleController controller(model);
    DummyDataGenerator gen;
    auto s = gen.sample(1);
    controller.registerSample(s.id, s.name, s.avgProductionTime, s.yield);
    controller.increaseStock(s.id, 10);
    auto result = controller.decreaseStock(s.id, 20);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(controller.getStock(s.id), 10);
}
