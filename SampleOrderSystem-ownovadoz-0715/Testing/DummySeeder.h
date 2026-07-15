#pragma once
#include "DummyDataGenerator.h"
#include "../SampleClerk/SampleModel.h"
#include "../OrderClerk/OrderModel.h"
#include "../ProductionLine/ProductionQueueModel.h"
#include "../Common/Clock.h"

namespace testing_support {

struct DummySeedResult {
    int samplesCreated;
    int ordersCreated;
    int productionJobsCreated;
};

// samples.json/orders.json/production_queue.json에 서로 앞뒤가 맞는 더미 데이터를 이어서 추가한다.
// - 주문은 반드시 이번에 새로 만들었거나 이미 존재하는 시료의 id를 참조한다.
// - PRODUCING 상태로 뽑힌 주문에는 생산 큐 항목도 함께 만든다(큐가 비어 있었으면 그 항목이 즉시 시작).
// 기존 데이터는 지우지 않고 이어서 누적한다(DummyDataGenerator-ownovadoz-0715 PoC와 동일한 방침).
// PRD.md 3장("실제 실행 시에는 항상 담당자가 직접 데이터를 입력해야 하므로 더미 데이터 생성이 메뉴에서
// 사용되어선 안 된다")에 따라, 이 함수는 일반 메뉴가 아니라 별도의 실행 모드에서만 호출돼야 한다.
DummySeedResult seedDummyData(sampleclerk::SampleModel& sampleModel, orderclerk::OrderModel& orderModel,
                               productionline::ProductionQueueModel& queueModel, common::IClock& clock,
                               int sampleCount, int orderCount, unsigned seed = 42);

} // namespace testing_support
