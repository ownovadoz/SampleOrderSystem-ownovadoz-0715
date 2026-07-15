#pragma once
#include "Time.h"

namespace common {

class IClock {
public:
    virtual ~IClock() = default;
    virtual TimePoint now() const = 0;
};

class SystemClock : public IClock {
public:
    TimePoint now() const override;
};

class FakeClock : public IClock {
public:
    explicit FakeClock(TimePoint initial);
    TimePoint now() const override;
    void set(TimePoint t);
    void advance(Duration d);

private:
    TimePoint current_;
};

} // namespace common
