#pragma once
#include <string>
#include "Time.h"

namespace common {

struct Sample {
    std::string id;
    std::string name;
    Duration avgProductionTime;
    double yield;   // 0 초과 1 이하
    int stock;      // 원재고 수량
};

} // namespace common
