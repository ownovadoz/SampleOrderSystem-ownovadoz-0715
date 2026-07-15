#pragma once
#include <optional>
#include <string>
#include <vector>
#include "ProductionQueueModel.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include "../Common/Clock.h"

namespace productionline {

struct JobView {
    std::string orderId;
    std::string sampleId;
    std::string sampleName;
    int shortfallQty;
    int actualQty;
    common::TimePoint startTime;
    common::TimePoint expectedCompletion;
    double progressPercent;
};

class ProductionLineController {
public:
    ProductionLineController(ProductionQueueModel& model, sampleclerk::SampleController& sampleController,
                              orderclerk::OrderController& orderController, common::IClock& clock);

    void enqueue(const std::string& sampleId, int shortfallQty, const std::string& orderId);
    std::optional<JobView> getCurrentJob();
    std::vector<JobView> getQueue();

private:
    void processCompletions();

    ProductionQueueModel& model_;
    sampleclerk::SampleController& sampleController_;
    orderclerk::OrderController& orderController_;
    common::IClock& clock_;
};

} // namespace productionline
