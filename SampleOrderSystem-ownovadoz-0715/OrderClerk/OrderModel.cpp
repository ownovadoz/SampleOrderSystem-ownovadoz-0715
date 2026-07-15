#include "OrderModel.h"
#include "../Common/FileRepository.h"

namespace orderclerk {

OrderModel::OrderModel(std::string filePath) : filePath_(std::move(filePath)) {}

void OrderModel::load() {
    orders_.clear();
    FileRepository<common::Order> repository;
    for (const auto& order : repository.Load(filePath_)) {
        orders_[order.id] = order;
    }
}

bool OrderModel::save() const {
    FileRepository<common::Order> repository;
    std::vector<common::Order> items;
    items.reserve(orders_.size());
    for (const auto& [id, order] : orders_) {
        items.push_back(order);
    }
    try {
        repository.Save(filePath_, items);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void OrderModel::insert(const common::Order& order) {
    orders_[order.id] = order;
}

std::optional<common::Order> OrderModel::find(const std::string& id) const {
    auto it = orders_.find(id);
    if (it == orders_.end()) return std::nullopt;
    return it->second;
}

std::vector<common::Order> OrderModel::findAll() const {
    std::vector<common::Order> result;
    for (const auto& [id, order] : orders_) {
        result.push_back(order);
    }
    return result;
}

void OrderModel::updateStatus(const std::string& id, common::OrderStatus status) {
    auto it = orders_.find(id);
    if (it != orders_.end()) {
        it->second.status = status;
    }
}

} // namespace orderclerk
