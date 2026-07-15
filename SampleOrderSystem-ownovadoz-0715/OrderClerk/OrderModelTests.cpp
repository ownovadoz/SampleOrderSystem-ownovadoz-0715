#include "OrderModel.h"
#include <gtest/gtest.h>
#include <cstdio>

using namespace orderclerk;
using namespace common;

TEST(OrderClerkTest, OrderModel_LoadFromMissingFile_StartsEmpty) {
    OrderModel model("nonexistent_orders_test.dat");
    model.load();
    EXPECT_TRUE(model.findAll().empty());
}

TEST(OrderClerkTest, OrderModel_InsertAndFind) {
    OrderModel model("orders_test_insert.dat");
    Order o{"ORD-20260416-0001", "S-001", "삼성전자", 100, OrderStatus::RESERVED};
    model.insert(o);
    auto found = model.find("ORD-20260416-0001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->customerName, "삼성전자");
    EXPECT_EQ(found->quantity, 100);
}

TEST(OrderClerkTest, OrderModel_SaveThenLoad_RoundTrips) {
    const std::string path = "orders_test_roundtrip.dat";
    std::remove(path.c_str());
    {
        OrderModel model(path);
        model.insert(Order{"ORD-20260416-0001", "S-001", "삼성전자", 100, OrderStatus::RESERVED});
        model.insert(Order{"ORD-20260416-0002", "S-002", "SK하이닉스", 50, OrderStatus::CONFIRMED});
        EXPECT_TRUE(model.save());
    }
    {
        OrderModel model(path);
        model.load();
        EXPECT_EQ(model.findAll().size(), 2u);
        auto found = model.find("ORD-20260416-0002");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->status, OrderStatus::CONFIRMED);
    }
    std::remove(path.c_str());
}

TEST(OrderClerkTest, OrderModel_UpdateStatus_ChangesValue) {
    OrderModel model("orders_test_update.dat");
    model.insert(Order{"ORD-20260416-0001", "S-001", "삼성전자", 100, OrderStatus::RESERVED});
    model.updateStatus("ORD-20260416-0001", OrderStatus::CONFIRMED);
    EXPECT_EQ(model.find("ORD-20260416-0001")->status, OrderStatus::CONFIRMED);
}
