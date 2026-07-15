#pragma once
#include <string>
#include <vector>
#include "../Common/Sample.h"
#include "../Common/Order.h"

namespace testing_support {

// 결정적(deterministic) 테스트 더미 데이터 생성기.
// DummyDataGenerator-ownovadoz-0715 PoC(이름 후보 목록 + 범위 균등분포)를 재사용하되,
// PRD.md 3장("랜덤을 사용하는 경우 시드 값을 이용해 항상 테스트 결과가 일정하도록 해야 한다")에
// 맞춰 시드를 고정할 수 있게 했다. 같은 (seed, index)는 항상 같은 결과를 낸다.
class DummyDataGenerator {
public:
    explicit DummyDataGenerator(unsigned seed = 42);

    common::Sample sample(int index) const;
    common::Order order(int index, const std::string& sampleId, int quantity,
                         common::OrderStatus status = common::OrderStatus::RESERVED) const;

    // [minValue, maxValue] 범위의 정수를 결정적으로 생성한다 (재고량/주문 수량 등에 사용).
    int randomInRange(int index, int minValue, int maxValue) const;

private:
    unsigned seed_;
};

} // namespace testing_support
