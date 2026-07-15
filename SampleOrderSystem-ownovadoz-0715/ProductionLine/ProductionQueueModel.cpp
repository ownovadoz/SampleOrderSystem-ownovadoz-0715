#include "ProductionQueueModel.h"
#include "../Common/FileRepository.h"

namespace productionline {

ProductionQueueModel::ProductionQueueModel(std::string filePath) : filePath_(std::move(filePath)) {}

void ProductionQueueModel::load() {
    FileRepository<ProductionJob> repository;
    queue_ = repository.Load(filePath_);
}

bool ProductionQueueModel::save() const {
    FileRepository<ProductionJob> repository;
    try {
        repository.Save(filePath_, queue_);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void ProductionQueueModel::pushBack(const ProductionJob& job) {
    queue_.push_back(job);
}

bool ProductionQueueModel::empty() const {
    return queue_.empty();
}

ProductionJob& ProductionQueueModel::front() {
    return queue_.front();
}

void ProductionQueueModel::popFront() {
    queue_.erase(queue_.begin());
}

std::vector<ProductionJob> ProductionQueueModel::snapshot() const {
    return queue_;
}

} // namespace productionline
