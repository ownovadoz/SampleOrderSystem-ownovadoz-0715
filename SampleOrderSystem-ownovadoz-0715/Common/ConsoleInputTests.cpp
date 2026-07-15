#include "ConsoleInput.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace common;

TEST(ConsoleInputTest, ReadLine_NormalText_ReturnsOkNotCancelled) {
    std::istringstream in("실리콘 웨이퍼\n");
    auto result = readLine(in);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.cancelled);
    EXPECT_EQ(result.text, "실리콘 웨이퍼");
}

TEST(ConsoleInputTest, ReadLine_LowercaseQ_IsCancelled) {
    std::istringstream in("q\n");
    auto result = readLine(in);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.cancelled);
}

TEST(ConsoleInputTest, ReadLine_UppercaseQ_IsCancelled) {
    std::istringstream in("Q\n");
    auto result = readLine(in);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.cancelled);
}

TEST(ConsoleInputTest, ReadLine_EndOfStream_NotOk) {
    std::istringstream in("");
    auto result = readLine(in);
    EXPECT_FALSE(result.ok);
}

TEST(ConsoleInputTest, ReadInt_ValidNumber_ReturnsValue) {
    std::istringstream in("42\n");
    auto result = readInt(in);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.cancelled);
    EXPECT_EQ(result.value, 42);
}

TEST(ConsoleInputTest, ReadInt_Garbage_NotOkButDoesNotThrow) {
    std::istringstream in("이상한값\n");
    auto result = readInt(in);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.cancelled);
}

TEST(ConsoleInputTest, ReadInt_Q_IsCancelled) {
    std::istringstream in("q\n");
    auto result = readInt(in);
    EXPECT_TRUE(result.cancelled);
}

TEST(ConsoleInputTest, ReadInt_NextReadStillWorks_AfterGarbage) {
    std::istringstream in("이상한값\n7\n");
    auto first = readInt(in);
    EXPECT_FALSE(first.ok);
    auto second = readInt(in);
    EXPECT_TRUE(second.ok);
    EXPECT_EQ(second.value, 7);
}

TEST(ConsoleInputTest, ReadDouble_ValidNumber_ReturnsValue) {
    std::istringstream in("0.92\n");
    auto result = readDouble(in);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.value, 0.92);
}

TEST(ConsoleInputTest, ReadDouble_Garbage_NotOkButDoesNotThrow) {
    std::istringstream in("이상한값\n");
    auto result = readDouble(in);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.cancelled);
}

TEST(ConsoleInputTest, ReadDouble_Q_IsCancelled) {
    std::istringstream in("q\n");
    auto result = readDouble(in);
    EXPECT_TRUE(result.cancelled);
}
