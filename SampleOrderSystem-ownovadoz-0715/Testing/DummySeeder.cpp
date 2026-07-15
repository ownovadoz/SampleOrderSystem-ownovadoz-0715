#include "DummySeeder.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace testing_support {

namespace {

// "S-003" -> 3, "ORD-DUMMY-0007" -> 7. 마지막 '-' 뒤 숫자를 읽는다. 형식이 다르면 0을 반환한다.
int extractNumericSuffix(const std::string& id) {
    size_t dashPos = id.rfind('-');
    if (dashPos == std::string::npos) {
        return 0;
    }
    try {
        return std::stoi(id.substr(dashPos + 1));
    } catch (const std::exception&) {
        return 0;
    }
}

int nextIndexAfter(const std::vector<std::string>& existingIds) {
    int maxIndex = 0;
    for (const auto& id : existingIds) {
        maxIndex = std::max(maxIndex, extractNumericSuffix(id));
    }
    return maxIndex + 1;
}

} // namespace

DummySeedResult seedDummyData(sampleclerk::SampleModel& sampleModel, orderclerk::OrderModel& orderModel,
                               productionline::ProductionQueueModel& queueModel, common::IClock& clock,
                               int sampleCount, int orderCount, unsigned seed) {
    DummyDataGenerator gen(seed);

    std::vector<std::string> existingSampleIds;
    for (const auto& sample : sampleModel.findAll()) {
        existingSampleIds.push_back(sample.id);
    }
    int nextSampleIndex = nextIndexAfter(existingSampleIds);

    std::vector<common::Sample> createdSamples;
    for (int i = 0; i < sampleCount; ++i) {
        int index = nextSampleIndex + i;
        common::Sample sample = gen.sample(index);
        sample.stock = gen.randomInRange(index, 0, 200);
        sampleModel.insert(sample);
        createdSamples.push_back(sample);
    }
    // 새로 만든 시료가 없으면(=시료 0개 요청) 기존 시료를 재사용해 주문을 만든다.
    if (createdSamples.empty()) {
        createdSamples = sampleModel.findAll();
    }

    std::vector<std::string> existingOrderIds;
    for (const auto& order : orderModel.findAll()) {
        existingOrderIds.push_back(order.id);
    }
    int nextOrderIndex = nextIndexAfter(existingOrderIds);

    static const std::array<common::OrderStatus, 5> kStatuses = {
        common::OrderStatus::RESERVED, common::OrderStatus::CONFIRMED, common::OrderStatus::PRODUCING,
        common::OrderStatus::RELEASE, common::OrderStatus::REJECTED};

    bool productionAlreadyRunning = !queueModel.empty();
    int jobsCreated = 0;

    for (int i = 0; i < orderCount && !createdSamples.empty(); ++i) {
        int index = nextOrderIndex + i;
        const common::Sample& sample =
            createdSamples[gen.randomInRange(index, 0, static_cast<int>(createdSamples.size()) - 1)];
        int quantity = gen.randomInRange(index, 1, 50);
        common::OrderStatus status = kStatuses[gen.randomInRange(index, 0, 4)];

        common::Order order = gen.order(index, sample.id, quantity, status);
        orderModel.insert(order);

        if (status == common::OrderStatus::PRODUCING) {
            productionline::ProductionJob job;
            job.orderId = order.id;
            job.sampleId = sample.id;
            job.shortfallQty = quantity;
            job.actualQty = static_cast<int>(std::ceil(static_cast<double>(quantity) / sample.yield));
            job.totalDuration = sample.avgProductionTime * job.actualQty;
            job.startTime = productionAlreadyRunning ? std::nullopt
                                                      : std::optional<common::TimePoint>(clock.now());
            productionAlreadyRunning = true;
            queueModel.pushBack(job);
            ++jobsCreated;
        }
    }

    return DummySeedResult{static_cast<int>(createdSamples.size()), orderCount, jobsCreated};
}

} // namespace testing_support
