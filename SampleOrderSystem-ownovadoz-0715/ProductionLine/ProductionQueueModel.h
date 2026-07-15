#pragma once
#include <string>
#include <vector>
#include "ProductionJob.h"

namespace productionline {

class ProductionQueueModel {
public:
    explicit ProductionQueueModel(std::string filePath);

    void load();
    bool save() const;

    void pushBack(const ProductionJob& job);
    bool empty() const;
    ProductionJob& front();
    void popFront();
    std::vector<ProductionJob> snapshot() const;

private:
    std::string filePath_;
    std::vector<ProductionJob> queue_;
};

} // namespace productionline
