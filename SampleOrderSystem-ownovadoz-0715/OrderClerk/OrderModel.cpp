#include "OrderModel.h"
#include "../Common/StringUtil.h"
#include <fstream>

namespace orderclerk {

namespace {
common::OrderStatus intToStatus(int v) {
    return static_cast<common::OrderStatus>(v);
}
} // namespace

OrderModel::OrderModel(std::string filePath) : filePath_(std::move(filePath)) {}

void OrderModel::load() {
    orders_.clear();
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto fields = common::splitString(line, '|');
        if (fields.size() != 5) continue;
        common::Order order;
        order.id = fields[0];
        order.sampleId = fields[1];
        order.customerName = fields[2];
        order.quantity = std::stoi(fields[3]);
        order.status = intToStatus(std::stoi(fields[4]));
        orders_[order.id] = order;
    }
}

bool OrderModel::save() const {
    std::ofstream out(filePath_, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    for (const auto& [id, order] : orders_) {
        out << order.id << '|' << order.sampleId << '|' << order.customerName << '|'
            << order.quantity << '|' << static_cast<int>(order.status) << '\n';
    }
    return true;
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
