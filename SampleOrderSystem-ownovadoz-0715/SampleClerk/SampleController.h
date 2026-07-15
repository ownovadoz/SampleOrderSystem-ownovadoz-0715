#pragma once
#include <optional>
#include <string>
#include <vector>
#include "SampleModel.h"
#include "../Common/Result.h"

namespace sampleclerk {

class SampleController {
public:
    explicit SampleController(SampleModel& model);

    common::Result registerSample(const std::string& id, const std::string& name,
                                   common::Duration avgProductionTime, double yield);
    std::vector<common::Sample> listSamples() const;
    std::vector<common::Sample> searchSamples(const std::string& keyword) const;
    std::optional<common::Sample> getSample(const std::string& id) const;
    int getStock(const std::string& id) const;
    common::Result increaseStock(const std::string& id, int qty);
    common::Result decreaseStock(const std::string& id, int qty);

private:
    SampleModel& model_;
};

} // namespace sampleclerk
