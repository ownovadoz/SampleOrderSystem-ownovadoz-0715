#pragma once
#include <iostream>
#include "ProductionLineController.h"

namespace productionline {

class ProductionLineView {
public:
    explicit ProductionLineView(ProductionLineController& controller);

    void showCurrentJobScreen(std::ostream& out);
    void showQueueScreen(std::ostream& out);

private:
    ProductionLineController& controller_;
};

} // namespace productionline
