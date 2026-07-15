#include "SampleController.h"
#include <algorithm>
#include <cctype>

namespace sampleclerk {

namespace {
std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}
} // namespace

SampleController::SampleController(SampleModel& model) : model_(model) {}

common::Result SampleController::registerSample(const std::string& id, const std::string& name,
                                                  common::Duration avgProductionTime, double yield) {
    if (model_.exists(id)) {
        return common::Result::failure("이미 존재하는 시료 ID입니다: " + id);
    }
    if (yield <= 0.0 || yield > 1.0) {
        return common::Result::failure("수율은 0보다 크고 1 이하이어야 합니다");
    }
    if (avgProductionTime.count() <= 0) {
        return common::Result::failure("평균 생산시간은 0보다 커야 합니다");
    }
    common::Sample sample{id, name, avgProductionTime, yield, 0};
    model_.insert(sample);
    return common::Result::success();
}

std::vector<common::Sample> SampleController::listSamples() const {
    return model_.findAll();
}

std::vector<common::Sample> SampleController::searchSamples(const std::string& keyword) const {
    std::vector<common::Sample> result;
    std::string lowerKeyword = toLower(keyword);
    for (const auto& sample : model_.findAll()) {
        if (toLower(sample.name).find(lowerKeyword) != std::string::npos) {
            result.push_back(sample);
        }
    }
    return result;
}

std::optional<common::Sample> SampleController::getSample(const std::string& id) const {
    return model_.find(id);
}

int SampleController::getStock(const std::string& id) const {
    auto sample = model_.find(id);
    return sample.has_value() ? sample->stock : 0;
}

common::Result SampleController::increaseStock(const std::string& id, int qty) {
    auto sample = model_.find(id);
    if (!sample.has_value()) {
        return common::Result::failure("존재하지 않는 시료 ID입니다: " + id);
    }
    model_.updateStock(id, sample->stock + qty);
    return common::Result::success();
}

common::Result SampleController::decreaseStock(const std::string& id, int qty) {
    auto sample = model_.find(id);
    if (!sample.has_value()) {
        return common::Result::failure("존재하지 않는 시료 ID입니다: " + id);
    }
    if (sample->stock < qty) {
        return common::Result::failure("재고보다 많은 수량을 차감할 수 없습니다");
    }
    model_.updateStock(id, sample->stock - qty);
    return common::Result::success();
}

} // namespace sampleclerk
