#include "Clock.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace common;

TEST(FakeClockTest, ReturnsSetTime) {
    TimePoint t0 = std::chrono::system_clock::from_time_t(1000);
    FakeClock clock(t0);
    EXPECT_EQ(clock.now(), t0);
}

TEST(FakeClockTest, AdvanceMovesTimeForward) {
    TimePoint t0 = std::chrono::system_clock::from_time_t(1000);
    FakeClock clock(t0);
    clock.advance(std::chrono::seconds(90));
    TimePoint expected = std::chrono::system_clock::from_time_t(1090);
    EXPECT_EQ(clock.now(), expected);
}

TEST(SystemClockTest, ReturnsRealTime) {
    SystemClock clock;
    auto before = std::chrono::system_clock::now();
    auto result = clock.now();
    auto after = std::chrono::system_clock::now();
    EXPECT_GE(result, before);
    EXPECT_LE(result, after);
}
