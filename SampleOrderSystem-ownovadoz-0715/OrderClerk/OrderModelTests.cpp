#include "OrderModel.h"
#include "../Testing/DummyDataGenerator.h"
#include <gtest/gtest.h>
#include <cstdio>

using namespace orderclerk;
using namespace common;
using testing_support::DummyDataGenerator;

TEST(OrderClerkTest, OrderModel_LoadFromMissingFile_StartsEmpty) {
    OrderModel model("nonexistent_orders_test.json");
    model.load();
    EXPECT_TRUE(model.findAll().empty());
}

TEST(OrderClerkTest, OrderModel_InsertAndFind) {
    OrderModel model("orders_test_insert.json");
    DummyDataGenerator gen;
    auto o = gen.order(1, "S-001", 100);
    model.insert(o);
    auto found = model.find(o.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->customerName, o.customerName);
    EXPECT_EQ(found->quantity, 100);
}

TEST(OrderClerkTest, OrderModel_SaveThenLoad_RoundTrips) {
    const std::string path = "orders_test_roundtrip.json";
    std::remove(path.c_str());
    DummyDataGenerator gen;
    auto o1 = gen.order(1, "S-001", 100, OrderStatus::RESERVED);
    auto o2 = gen.order(2, "S-002", 50, OrderStatus::CONFIRMED);
    {
        OrderModel model(path);
        model.insert(o1);
        model.insert(o2);
        EXPECT_TRUE(model.save());
    }
    {
        OrderModel model(path);
        model.load();
        EXPECT_EQ(model.findAll().size(), 2u);
        auto found = model.find(o2.id);
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->status, OrderStatus::CONFIRMED);
    }
    std::remove(path.c_str());
}

TEST(OrderClerkTest, OrderModel_UpdateStatus_ChangesValue) {
    OrderModel model("orders_test_update.json");
    DummyDataGenerator gen;
    auto o = gen.order(1, "S-001", 100);
    model.insert(o);
    model.updateStatus(o.id, OrderStatus::CONFIRMED);
    EXPECT_EQ(model.find(o.id)->status, OrderStatus::CONFIRMED);
}
