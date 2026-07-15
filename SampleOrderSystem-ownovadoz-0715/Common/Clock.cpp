#include "Clock.h"

namespace common {

TimePoint SystemClock::now() const {
    return std::chrono::system_clock::now();
}

FakeClock::FakeClock(TimePoint initial) : current_(initial) {}

TimePoint FakeClock::now() const {
    return current_;
}

void FakeClock::set(TimePoint t) {
    current_ = t;
}

void FakeClock::advance(Duration d) {
    current_ += d;
}

} // namespace common
