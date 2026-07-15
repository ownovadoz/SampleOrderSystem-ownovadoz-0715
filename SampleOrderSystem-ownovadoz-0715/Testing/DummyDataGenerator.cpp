#include "DummyDataGenerator.h"
#include <iomanip>
#include <random>
#include <sstream>

namespace testing_support {

namespace {

const std::vector<std::string> kSampleNamePool = {
    "실리콘 웨이퍼-8인치",
    "GaN 에피탁셜-4인치",
    "SiC 파워기판-6인치",
    "포토레지스트-PR7",
    "산화막 웨이퍼-SiO2",
    "질화막 웨이퍼-Si3N4",
    "게르마늄 기판-4인치",
    "사파이어 기판-2인치",
};

const std::vector<std::string> kCustomerNamePool = {
    "삼성전자",
    "SK하이닉스",
    "DB하이텍",
    "매그나칩반도체",
    "키파운드리",
};

std::string formatSampleId(int index) {
    std::ostringstream oss;
    oss << "S-" << std::setw(3) << std::setfill('0') << index;
    return oss.str();
}

std::string formatOrderId(int index) {
    std::ostringstream oss;
    oss << "ORD-DUMMY-" << std::setw(4) << std::setfill('0') << index;
    return oss.str();
}

} // namespace

DummyDataGenerator::DummyDataGenerator(unsigned seed) : seed_(seed) {}

common::Sample DummyDataGenerator::sample(int index) const {
    std::mt19937 rng(seed_ + static_cast<unsigned>(index));
    std::uniform_int_distribution<size_t> nameDist(0, kSampleNamePool.size() - 1);
    std::uniform_real_distribution<double> minutesDist(0.2, 60.0);
    std::uniform_real_distribution<double> yieldDist(0.7, 0.99);

    common::Sample s;
    s.id = formatSampleId(index);
    s.name = kSampleNamePool[nameDist(rng)];
    s.avgProductionTime = common::Duration(static_cast<long long>(minutesDist(rng) * 60));
    s.yield = yieldDist(rng);
    s.stock = 0;
    return s;
}

common::Order DummyDataGenerator::order(int index, const std::string& sampleId, int quantity,
                                         common::OrderStatus status) const {
    std::mt19937 rng(seed_ + 10000u + static_cast<unsigned>(index));
    std::uniform_int_distribution<size_t> customerDist(0, kCustomerNamePool.size() - 1);

    common::Order o;
    o.id = formatOrderId(index);
    o.sampleId = sampleId;
    o.customerName = kCustomerNamePool[customerDist(rng)];
    o.quantity = quantity;
    o.status = status;
    return o;
}

int DummyDataGenerator::randomInRange(int index, int minValue, int maxValue) const {
    std::mt19937 rng(seed_ + 20000u + static_cast<unsigned>(index));
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng);
}

} // namespace testing_support
