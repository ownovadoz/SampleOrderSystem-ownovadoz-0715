#include "DummyDataGenerator.h"
#include <gtest/gtest.h>

using namespace testing_support;

TEST(DummyDataGeneratorTest, SameSeedAndIndex_ProducesIdenticalSample) {
    DummyDataGenerator gen(42);
    auto a = gen.sample(1);
    auto b = gen.sample(1);
    EXPECT_EQ(a.id, b.id);
    EXPECT_EQ(a.name, b.name);
    EXPECT_EQ(a.avgProductionTime, b.avgProductionTime);
    EXPECT_EQ(a.yield, b.yield);
}

TEST(DummyDataGeneratorTest, Sample_YieldWithinValidRange) {
    DummyDataGenerator gen(42);
    for (int i = 1; i <= 20; ++i) {
        auto s = gen.sample(i);
        EXPECT_GT(s.yield, 0.0);
        EXPECT_LE(s.yield, 1.0);
    }
}

TEST(DummyDataGeneratorTest, Sample_IdFollowsSFormat) {
    DummyDataGenerator gen(42);
    EXPECT_EQ(gen.sample(1).id, "S-001");
    EXPECT_EQ(gen.sample(23).id, "S-023");
}

TEST(DummyDataGeneratorTest, SameSeedAndIndex_ProducesIdenticalOrder) {
    DummyDataGenerator gen(42);
    auto a = gen.order(1, "S-001", 10);
    auto b = gen.order(1, "S-001", 10);
    EXPECT_EQ(a.id, b.id);
    EXPECT_EQ(a.customerName, b.customerName);
    EXPECT_EQ(a.sampleId, b.sampleId);
    EXPECT_EQ(a.quantity, b.quantity);
}

TEST(DummyDataGeneratorTest, RandomInRange_SameSeedAndIndex_ProducesIdenticalValue) {
    DummyDataGenerator gen(42);
    EXPECT_EQ(gen.randomInRange(1, 0, 100), gen.randomInRange(1, 0, 100));
}

TEST(DummyDataGeneratorTest, RandomInRange_StaysWithinBounds) {
    DummyDataGenerator gen(42);
    for (int i = 1; i <= 20; ++i) {
        int value = gen.randomInRange(i, 10, 20);
        EXPECT_GE(value, 10);
        EXPECT_LE(value, 20);
    }
}
