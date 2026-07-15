#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "../Common/Order.h"

namespace orderclerk {

class OrderModel {
public:
    explicit OrderModel(std::string filePath);

    void load();
    bool save() const;

    void insert(const common::Order& order);
    std::optional<common::Order> find(const std::string& id) const;
    std::vector<common::Order> findAll() const;
    void updateStatus(const std::string& id, common::OrderStatus status);

private:
    std::string filePath_;
    std::map<std::string, common::Order> orders_;
};

} // namespace orderclerk
