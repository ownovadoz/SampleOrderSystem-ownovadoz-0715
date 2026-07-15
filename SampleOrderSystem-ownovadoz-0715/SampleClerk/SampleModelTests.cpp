#include "SampleModel.h"
#include <gtest/gtest.h>
#include <cstdio>

using namespace sampleclerk;
using namespace common;

TEST(SampleClerkTest, SampleModel_LoadFromMissingFile_StartsEmpty) {
    SampleModel model("nonexistent_samples_test.dat");
    model.load();
    EXPECT_TRUE(model.findAll().empty());
}

TEST(SampleClerkTest, SampleModel_InsertAndFind) {
    SampleModel model("samples_test_insert.dat");
    Sample s{"S-001", "테스트 시료", Duration(60), 0.9, 100};
    model.insert(s);
    auto found = model.find("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "테스트 시료");
    EXPECT_EQ(found->stock, 100);
}

TEST(SampleClerkTest, SampleModel_SaveThenLoad_RoundTrips) {
    const std::string path = "samples_test_roundtrip.dat";
    std::remove(path.c_str());
    {
        SampleModel model(path);
        model.insert(Sample{"S-001", "실리콘 웨이퍼", Duration(1800), 0.92, 480});
        model.insert(Sample{"S-002", "GaN 에피택셜", Duration(1080), 0.78, 220});
        EXPECT_TRUE(model.save());
    }
    {
        SampleModel model(path);
        model.load();
        auto all = model.findAll();
        EXPECT_EQ(all.size(), 2u);
        auto found = model.find("S-002");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->yield, 0.78);
        EXPECT_EQ(found->stock, 220);
    }
    std::remove(path.c_str());
}

TEST(SampleClerkTest, SampleModel_UpdateStock_ChangesValue) {
    SampleModel model("samples_test_update.dat");
    model.insert(Sample{"S-001", "테스트", Duration(60), 0.9, 10});
    model.updateStock("S-001", 25);
    EXPECT_EQ(model.find("S-001")->stock, 25);
}
