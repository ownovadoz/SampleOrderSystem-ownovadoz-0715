#include "ProductionLineController.h"
#include <algorithm>
#include <cmath>

namespace productionline {

ProductionLineController::ProductionLineController(ProductionQueueModel& model,
                                                     sampleclerk::SampleController& sampleController,
                                                     orderclerk::OrderController& orderController,
                                                     common::IClock& clock)
    : model_(model), sampleController_(sampleController), orderController_(orderController), clock_(clock) {}

void ProductionLineController::enqueue(const std::string& sampleId, int shortfallQty,
                                        const std::string& orderId) {
    auto sample = sampleController_.getSample(sampleId);
    if (!sample.has_value()) {
        return;
    }
    int actualQty = static_cast<int>(std::ceil(static_cast<double>(shortfallQty) / sample->yield));
    common::Duration totalDuration = sample->avgProductionTime * actualQty;

    ProductionJob job;
    job.orderId = orderId;
    job.sampleId = sampleId;
    job.shortfallQty = shortfallQty;
    job.actualQty = actualQty;
    job.totalDuration = totalDuration;
    job.startTime = model_.empty() ? std::optional<common::TimePoint>(clock_.now()) : std::nullopt;

    model_.pushBack(job);
}

void ProductionLineController::processCompletions() {
    common::TimePoint now = clock_.now();
    while (!model_.empty() && model_.front().startTime.has_value()) {
        common::TimePoint jobEnd = *model_.front().startTime + model_.front().totalDuration;
        if (now < jobEnd) break;

        ProductionJob finished = model_.front();
        model_.popFront();
        sampleController_.increaseStock(finished.sampleId, finished.actualQty);
        orderController_.setOrderStatus(finished.orderId, common::OrderStatus::CONFIRMED);

        if (!model_.empty()) {
            model_.front().startTime = jobEnd;
        }
    }
}

std::optional<JobView> ProductionLineController::getCurrentJob() {
    processCompletions();
    if (model_.empty()) return std::nullopt;
    const auto& job = model_.front();
    if (!job.startTime.has_value()) return std::nullopt;

    common::TimePoint now = clock_.now();
    common::TimePoint expectedCompletion = *job.startTime + job.totalDuration;
    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - *job.startTime).count();
    double total = std::chrono::duration_cast<std::chrono::duration<double>>(job.totalDuration).count();
    double progress = total > 0.0 ? std::min(100.0, (elapsed / total) * 100.0) : 100.0;

    auto sample = sampleController_.getSample(job.sampleId);
    std::string sampleName = sample.has_value() ? sample->name : job.sampleId;

    return JobView{job.orderId, job.sampleId, sampleName, job.shortfallQty, job.actualQty,
                   *job.startTime, expectedCompletion, progress};
}

std::vector<JobView> ProductionLineController::getQueue() {
    processCompletions();
    auto snapshot = model_.snapshot();
    std::vector<JobView> result;
    if (snapshot.empty()) return result;

    common::TimePoint cursor = snapshot.front().startTime.has_value()
        ? *snapshot.front().startTime + snapshot.front().totalDuration
        : clock_.now();

    for (size_t i = 1; i < snapshot.size(); ++i) {
        const auto& job = snapshot[i];
        common::TimePoint expectedCompletion = cursor + job.totalDuration;
        auto sample = sampleController_.getSample(job.sampleId);
        std::string sampleName = sample.has_value() ? sample->name : job.sampleId;
        result.push_back(JobView{job.orderId, job.sampleId, sampleName, job.shortfallQty, job.actualQty,
                                  cursor, expectedCompletion, 0.0});
        cursor = expectedCompletion;
    }
    return result;
}

} // namespace productionline
