#include "SampleModel.h"
#include "../Common/StringUtil.h"
#include <fstream>
#include <iomanip>

namespace sampleclerk {

SampleModel::SampleModel(std::string filePath) : filePath_(std::move(filePath)) {}

void SampleModel::load() {
    samples_.clear();
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        return; // 파일이 없으면 빈 상태로 시작
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto fields = common::splitString(line, '|');
        if (fields.size() != 5) continue;
        common::Sample sample;
        sample.id = fields[0];
        sample.name = fields[1];
        sample.avgProductionTime = common::Duration(std::stoll(fields[2]));
        sample.yield = std::stod(fields[3]);
        sample.stock = std::stoi(fields[4]);
        samples_[sample.id] = sample;
    }
}

bool SampleModel::save() const {
    std::ofstream out(filePath_, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << std::setprecision(10);
    for (const auto& [id, sample] : samples_) {
        out << sample.id << '|' << sample.name << '|' << sample.avgProductionTime.count() << '|'
            << sample.yield << '|' << sample.stock << '\n';
    }
    return true;
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
