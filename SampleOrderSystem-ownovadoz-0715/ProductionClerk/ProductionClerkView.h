#pragma once
#include <iostream>
#include "ProductionClerkController.h"

namespace productionclerk {

class ProductionClerkView {
public:
    explicit ProductionClerkView(ProductionClerkController& controller);

    void showApprovalScreen(std::istream& in, std::ostream& out);
    void showRejectionScreen(std::istream& in, std::ostream& out);
    void showShipmentScreen(std::istream& in, std::ostream& out);

private:
    ProductionClerkController& controller_;
};

} // namespace productionclerk
