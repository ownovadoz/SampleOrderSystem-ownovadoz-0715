#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "../Common/Sample.h"

namespace sampleclerk {

class SampleModel {
public:
    explicit SampleModel(std::string filePath);

    void load();
    bool save() const;

    bool exists(const std::string& id) const;
    void insert(const common::Sample& sample);
    std::optional<common::Sample> find(const std::string& id) const;
    std::vector<common::Sample> findAll() const;
    void updateStock(const std::string& id, int newStock);

private:
    std::string filePath_;
    std::map<std::string, common::Sample> samples_;
};

} // namespace sampleclerk
