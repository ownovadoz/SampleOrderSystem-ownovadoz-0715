#pragma once
#include <iostream>
#include "OrderController.h"

namespace orderclerk {

class OrderView {
public:
    explicit OrderView(OrderController& controller);

    void showReserveScreen(std::istream& in, std::ostream& out);

private:
    OrderController& controller_;
};

} // namespace orderclerk
