#include "SampleModel.h"
#include "../Testing/DummyDataGenerator.h"
#include <gtest/gtest.h>
#include <cstdio>

using namespace sampleclerk;
using namespace common;
using testing_support::DummyDataGenerator;

TEST(SampleClerkTest, SampleModel_LoadFromMissingFile_StartsEmpty) {
    SampleModel model("nonexistent_samples_test.json");
    model.load();
    EXPECT_TRUE(model.findAll().empty());
}

TEST(SampleClerkTest, SampleModel_InsertAndFind) {
    SampleModel model("samples_test_insert.json");
    DummyDataGenerator gen;
    auto s = gen.sample(1);
    s.stock = 100;
    model.insert(s);
    auto found = model.find(s.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, s.name);
    EXPECT_EQ(found->stock, 100);
}

TEST(SampleClerkTest, SampleModel_SaveThenLoad_RoundTrips) {
    const std::string path = "samples_test_roundtrip.json";
    std::remove(path.c_str());
    DummyDataGenerator gen;
    auto s1 = gen.sample(1);
    auto s2 = gen.sample(2);
    s2.stock = 220;
    {
        SampleModel model(path);
        model.insert(s1);
        model.insert(s2);
        EXPECT_TRUE(model.save());
    }
    {
        SampleModel model(path);
        model.load();
        auto all = model.findAll();
        EXPECT_EQ(all.size(), 2u);
        auto found = model.find(s2.id);
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->yield, s2.yield);
        EXPECT_EQ(found->stock, 220);
    }
    std::remove(path.c_str());
}

TEST(SampleClerkTest, SampleModel_UpdateStock_ChangesValue) {
    SampleModel model("samples_test_update.json");
    DummyDataGenerator gen;
    auto s = gen.sample(1);
    s.stock = 10;
    model.insert(s);
    model.updateStock(s.id, 25);
    EXPECT_EQ(model.find(s.id)->stock, 25);
}
