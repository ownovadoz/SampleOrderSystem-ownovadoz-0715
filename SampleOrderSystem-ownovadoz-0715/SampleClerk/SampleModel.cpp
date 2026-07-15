#include "SampleModel.h"
#include "../Common/FileRepository.h"

namespace sampleclerk {

SampleModel::SampleModel(std::string filePath) : filePath_(std::move(filePath)) {}

void SampleModel::load() {
    samples_.clear();
    FileRepository<common::Sample> repository;
    for (const auto& sample : repository.Load(filePath_)) {
        samples_[sample.id] = sample;
    }
}

bool SampleModel::save() const {
    FileRepository<common::Sample> repository;
    std::vector<common::Sample> items;
    items.reserve(samples_.size());
    for (const auto& [id, sample] : samples_) {
        items.push_back(sample);
    }
    try {
        repository.Save(filePath_, items);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SampleModel::exists(const std::string& id) const {
    return samples_.find(id) != samples_.end();
}

void SampleModel::insert(const common::Sample& sample) {
    samples_[sample.id] = sample;
}

std::optional<common::Sample> SampleModel::find(const std::string& id) const {
    auto it = samples_.find(id);
    if (it == samples_.end()) return std::nullopt;
    return it->second;
}

std::vector<common::Sample> SampleModel::findAll() const {
    std::vector<common::Sample> result;
    for (const auto& [id, sample] : samples_) {
        result.push_back(sample);
    }
    return result;
}

void SampleModel::updateStock(const std::string& id, int newStock) {
    auto it = samples_.find(id);
    if (it != samples_.end()) {
        it->second.stock = newStock;
    }
}

} // namespace sampleclerk
