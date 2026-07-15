#pragma once
#include <string>
#include "OrderStatus.h"

namespace common {

struct Order {
    std::string id;
    std::string sampleId;
    std::string customerName;
    int quantity;
    OrderStatus status;
};

} // namespace common
