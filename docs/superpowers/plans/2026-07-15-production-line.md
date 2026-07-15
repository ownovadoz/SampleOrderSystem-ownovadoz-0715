# 생산 라인(Production Line) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 생산 큐(FIFO) 관리, 실 생산량/소요시간 계산, 시간 경과에 따른 완료 처리("생산 현황"/"대기 주문
확인" 메뉴)를 담당하는 생산 라인 모듈을 구현한다. 레이어 2로, 시료 담당자와 주문 담당자의 공개 API에
의존한다.

**Architecture:** `SampleOrderSystem-ownovadoz-0715/ProductionLine/`에 `ProductionQueueModel`(큐 저장) →
`ProductionLineController`(수율/시간 계산, 완료 판정 및 반영) → `ProductionLineView`(콘솔 화면) 3계층.
"현재 생산 중"과 "대기 큐"는 물리적으로 하나의 큐(`deque<ProductionJob>`)이며, front의 `startTime` 유무로
구분한다(`docs/superpowers/specs/2026-07-15-module-architecture-design.md` 4.3절).

**Tech Stack:** C++20. `common::Sample`, `common::OrderStatus`, `common::IClock`/`FakeClock`,
`sampleclerk::SampleController`, `orderclerk::OrderController` 재사용.

> ⚠️ **테스트 하네스 변경 안내**: Foundation이 gmock/gtest(NuGet, 단일 앱 프로젝트)로 바뀌었다
> (`docs/superpowers/plans/2026-07-15-foundation.md` 참고). 아래 스텝에서 `TEST_CASE`/`REQUIRE`/
> `MicroTest.h`/별도 `.Tests` 프로젝트가 나오면 `2026-07-15-sample-clerk.md` 상단의 변경 안내와 동일하게
> `TEST`/`EXPECT_*`/`<gtest/gtest.h>`/단일 앱 프로젝트로 치환해서 적용한다.

## Global Constraints

- 스펙 4.3절(데이터 구조), 5장(공개 API), 6장(생산 완료 처리 흐름), 10.9~10.10절(화면 명세)을 따른다.
- `production_queue.dat` 파일 포맷: `주문ID|시료ID|부족분|실생산량|총생산시간(초)|시작시각(epoch초 또는
  리터럴 "NONE")` 파이프 구분 텍스트, 한 줄당 한 작업, 큐 순서(파일 내 줄 순서) = FIFO 순서.
- 완료 판정/반영(`processCompletions`)은 `getCurrentJob`/`getQueue` 호출 시마다 수행한다(스펙 6장 —
  생산 담당자를 거치지 않고 생산 라인이 직접 `sampleClerk.increaseStock` / `orderClerk.setOrderStatus`
  호출).
- 이 플랜은 생산 라인 자체(큐 처리)만 다룬다. 큐에 작업을 채우는 `enqueue` 호출은 생산 담당자 모듈
  (다음 플랜)의 승인 로직에서 이루어지므로, 이 플랜의 수동 확인 단계에서는 큐가 항상 빈 상태로만 보일 수
  있다 — 실제 생산 흐름 검증은 아래 TDD 테스트로 충분히 커버한다.

---

### Task 1: ProductionJob + ProductionQueueModel — 큐 저장/복원 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionJob.h`
- Create: `SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionQueueModel.h`
- Create: `SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionQueueModel.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+.filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/ProductionLine/ProductionQueueModelTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+.filters)

**Interfaces:**
- Consumes: `common::Duration`, `common::TimePoint` (Foundation)
- Produces: `productionline::ProductionJob{orderId, sampleId, shortfallQty, actualQty, totalDuration,
  startTime}`; `productionline::ProductionQueueModel` — `load()`, `save() -> bool`, `pushBack(job)`,
  `empty() -> bool`, `front() -> ProductionJob&`, `popFront()`, `snapshot() -> vector<ProductionJob>`.
  Task 2(Controller)가 이 타입을 그대로 사용한다.

- [ ] **Step 1: `ProductionJob.h` 작성**

```cpp
#pragma once
#include <optional>
#include <string>
#include "../Common/Time.h"

namespace productionline {

struct ProductionJob {
    std::string orderId;
    std::string sampleId;
    int shortfallQty;
    int actualQty;
    common::Duration totalDuration;
    std::optional<common::TimePoint> startTime;
};

} // namespace productionline
```

- [ ] **Step 2: 실패하는 테스트 작성 — `ProductionQueueModelTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionQueueModel.h"
#include "../Testing/MicroTest.h"
#include <cstdio>
#include <chrono>

using namespace productionline;
using namespace common;

TEST_CASE(ProductionQueueModel_LoadFromMissingFile_StartsEmpty) {
    ProductionQueueModel model("nonexistent_queue_test.dat");
    model.load();
    REQUIRE(model.empty());
}

TEST_CASE(ProductionQueueModel_PushAndFrontAndPop) {
    ProductionQueueModel model("queue_test_push.dat");
    ProductionJob job{"ORD-1", "S-001", 10, 10, Duration(600), std::chrono::system_clock::from_time_t(1000)};
    model.pushBack(job);
    REQUIRE(!model.empty());
    REQUIRE(model.front().orderId == "ORD-1");
    model.popFront();
    REQUIRE(model.empty());
}

TEST_CASE(ProductionQueueModel_SaveThenLoad_RoundTrips) {
    const std::string path = "queue_test_roundtrip.dat";
    std::remove(path.c_str());
    {
        ProductionQueueModel model(path);
        model.pushBack(ProductionJob{"ORD-1", "S-001", 10, 10, Duration(600),
                                      std::chrono::system_clock::from_time_t(1000)});
        model.pushBack(ProductionJob{"ORD-2", "S-002", 5, 5, Duration(300), std::nullopt});
        REQUIRE(model.save());
    }
    {
        ProductionQueueModel model(path);
        model.load();
        auto snap = model.snapshot();
        REQUIRE(snap.size() == 2);
        REQUIRE(snap[0].startTime.has_value());
        REQUIRE(!snap[1].startTime.has_value());
        REQUIRE(snap[1].orderId == "ORD-2");
    }
    std::remove(path.c_str());
}
```

- [ ] **Step 3: vcxproj/필터 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="ProductionLine\ProductionQueueModelTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\ProductionLine\ProductionQueueModel.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\ProductionLine\ProductionJob.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\ProductionLine\ProductionQueueModel.h" />
```

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`에도 위 파일들을 각각 "소스 파일"/"헤더 파일"
필터로 추가한다.

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `ProductionQueueModel.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 4: `ProductionQueueModel.h` 구현**

```cpp
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
```

(참고: 앞쪽 원소를 지우는 `popFront`는 `std::deque`가 더 효율적이지만, 이번 규모에서는 `std::vector` +
`erase(begin())`으로도 충분하다 — 과도한 최적화를 피한다.)

- [ ] **Step 5: `ProductionQueueModel.cpp` 구현**

```cpp
#include "ProductionQueueModel.h"
#include "../Common/StringUtil.h"
#include <fstream>
#include <chrono>

namespace productionline {

ProductionQueueModel::ProductionQueueModel(std::string filePath) : filePath_(std::move(filePath)) {}

void ProductionQueueModel::load() {
    queue_.clear();
    std::ifstream in(filePath_);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto fields = common::splitString(line, '|');
        if (fields.size() != 6) continue;
        ProductionJob job;
        job.orderId = fields[0];
        job.sampleId = fields[1];
        job.shortfallQty = std::stoi(fields[2]);
        job.actualQty = std::stoi(fields[3]);
        job.totalDuration = common::Duration(std::stoll(fields[4]));
        if (fields[5] == "NONE") {
            job.startTime = std::nullopt;
        } else {
            job.startTime = std::chrono::system_clock::from_time_t(std::stoll(fields[5]));
        }
        queue_.push_back(job);
    }
}

bool ProductionQueueModel::save() const {
    std::ofstream out(filePath_, std::ios::trunc);
    if (!out.is_open()) return false;
    for (const auto& job : queue_) {
        out << job.orderId << '|' << job.sampleId << '|' << job.shortfallQty << '|' << job.actualQty << '|'
            << job.totalDuration.count() << '|';
        if (job.startTime.has_value()) {
            out << std::chrono::system_clock::to_time_t(*job.startTime);
        } else {
            out << "NONE";
        }
        out << '\n';
    }
    return true;
}

void ProductionQueueModel::pushBack(const ProductionJob& job) {
    queue_.push_back(job);
}

bool ProductionQueueModel::empty() const {
    return queue_.empty();
}

ProductionJob& ProductionQueueModel::front() {
    return queue_.front();
}

void ProductionQueueModel::popFront() {
    queue_.erase(queue_.begin());
}

std::vector<ProductionJob> ProductionQueueModel::snapshot() const {
    return queue_;
}

} // namespace productionline
```

- [ ] **Step 6: 앱 vcxproj/필터에도 동일 파일 등록**

`SampleOrderSystem-ownovadoz-0715.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="ProductionLine\ProductionQueueModel.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="ProductionLine\ProductionJob.h" />
    <ClInclude Include="ProductionLine\ProductionQueueModel.h" />
```

`.vcxproj.filters`에도 동일하게 반영한다.

- [ ] **Step 7: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: 기존 28개 + 신규 3개(`ProductionQueueModel_LoadFromMissingFile_StartsEmpty`,
`ProductionQueueModel_PushAndFrontAndPop`, `ProductionQueueModel_SaveThenLoad_RoundTrips`) =
`31/31 tests passed`

- [ ] **Step 8: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/ProductionLine \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "생산 라인: ProductionQueueModel (큐 저장/복원) 추가"
```

---

### Task 2: ProductionLineController — 수율 계산/완료 반영 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionLineController.h`
- Create: `SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionLineController.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+.filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/ProductionLine/ProductionLineControllerTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+.filters)

**Interfaces:**
- Consumes: `productionline::ProductionQueueModel` (Task 1), `sampleclerk::SampleController::getSample`/
  `increaseStock` (Sample Clerk 플랜), `orderclerk::OrderController::setOrderStatus` (Order Clerk 플랜),
  `common::IClock::now()`
- Produces: `productionline::JobView{orderId, sampleId, sampleName, shortfallQty, actualQty, startTime,
  expectedCompletion, progressPercent}`; `productionline::ProductionLineController` —
  `enqueue(sampleId, shortfallQty, orderId)`, `getCurrentJob() -> optional<JobView>`,
  `getQueue() -> vector<JobView>`. 생산 담당자 플랜이 `enqueue`를 그대로 호출한다.

- [ ] **Step 1: 실패하는 테스트 작성 — `ProductionLineControllerTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionLineController.h"
#include "../../SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleController.h"
#include "../../SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderController.h"
#include "../Testing/MicroTest.h"

using namespace productionline;
using namespace sampleclerk;
using namespace orderclerk;
using namespace common;

namespace {
TimePoint dayZero() { return std::chrono::system_clock::from_time_t(0); }
}

TEST_CASE(ProductionLineController_Enqueue_StartsImmediatelyWhenQueueEmpty) {
    SampleModel sampleModel("pl_test_samples1.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders1.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder("S-001", "삼성전자", 10);
    orderController.setOrderStatus(order.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue1.dat");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);

    plController.enqueue("S-001", 10, order.orderId);

    auto current = plController.getCurrentJob();
    REQUIRE(current.has_value());
    REQUIRE(current->orderId == order.orderId);
    REQUIRE(current->actualQty == 10); // ceil(10 / 1.0)
}

TEST_CASE(ProductionLineController_ProcessCompletion_UpdatesStockAndOrderStatus) {
    SampleModel sampleModel("pl_test_samples2.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders2.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder("S-001", "삼성전자", 10);
    orderController.setOrderStatus(order.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue2.dat");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 10, order.orderId); // 실생산량 10, 총 소요 600초

    clock.advance(std::chrono::seconds(600));

    auto current = plController.getCurrentJob();
    REQUIRE(!current.has_value()); // 완료되어 큐가 비었음

    REQUIRE(sampleController.getStock("S-001") == 10);
    REQUIRE(orderController.getOrder(order.orderId)->status == OrderStatus::CONFIRMED);
}

TEST_CASE(ProductionLineController_MultipleCompletions_ProcessedInFifoOrder) {
    SampleModel sampleModel("pl_test_samples3.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders3.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order1 = orderController.createOrder("S-001", "A", 5);  // 실생산량 5, 300초
    auto order2 = orderController.createOrder("S-001", "B", 3);  // 실생산량 3, 180초
    orderController.setOrderStatus(order1.orderId, OrderStatus::PRODUCING);
    orderController.setOrderStatus(order2.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue3.dat");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 5, order1.orderId);
    plController.enqueue("S-001", 3, order2.orderId);

    clock.advance(std::chrono::seconds(300 + 180));

    auto current = plController.getCurrentJob();
    REQUIRE(!current.has_value());
    REQUIRE(sampleController.getStock("S-001") == 8); // 5 + 3
    REQUIRE(orderController.getOrder(order1.orderId)->status == OrderStatus::CONFIRMED);
    REQUIRE(orderController.getOrder(order2.orderId)->status == OrderStatus::CONFIRMED);
}

TEST_CASE(ProductionLineController_GetQueue_ShowsWaitingJobsExcludingCurrent) {
    SampleModel sampleModel("pl_test_samples4.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);

    OrderModel orderModel("pl_test_orders4.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);
    auto order1 = orderController.createOrder("S-001", "A", 5);
    auto order2 = orderController.createOrder("S-001", "B", 3);
    orderController.setOrderStatus(order1.orderId, OrderStatus::PRODUCING);
    orderController.setOrderStatus(order2.orderId, OrderStatus::PRODUCING);

    ProductionQueueModel queueModel("pl_test_queue4.dat");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 5, order1.orderId);
    plController.enqueue("S-001", 3, order2.orderId);

    auto queue = plController.getQueue();
    REQUIRE(queue.size() == 1);
    REQUIRE(queue[0].orderId == order2.orderId);
}
```

- [ ] **Step 2: vcxproj/필터 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="ProductionLine\ProductionLineControllerTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\ProductionLine\ProductionLineController.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderController.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderModel.cpp" />
```

(주: `SampleController.cpp` 등 이전 플랜에서 이미 추가된 항목은 그대로 둔다.)

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\ProductionLine\ProductionLineController.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderController.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderModel.h" />
```

`.vcxproj.filters`에도 위 파일들을 각각 "소스 파일"/"헤더 파일" 필터로 추가한다.

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `ProductionLineController.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 3: `ProductionLineController.h` 구현**

```cpp
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "ProductionQueueModel.h"
#include "../SampleClerk/SampleController.h"
#include "../OrderClerk/OrderController.h"
#include "../Common/Clock.h"

namespace productionline {

struct JobView {
    std::string orderId;
    std::string sampleId;
    std::string sampleName;
    int shortfallQty;
    int actualQty;
    common::TimePoint startTime;
    common::TimePoint expectedCompletion;
    double progressPercent;
};

class ProductionLineController {
public:
    ProductionLineController(ProductionQueueModel& model, sampleclerk::SampleController& sampleController,
                              orderclerk::OrderController& orderController, common::IClock& clock);

    void enqueue(const std::string& sampleId, int shortfallQty, const std::string& orderId);
    std::optional<JobView> getCurrentJob();
    std::vector<JobView> getQueue();

private:
    void processCompletions();

    ProductionQueueModel& model_;
    sampleclerk::SampleController& sampleController_;
    orderclerk::OrderController& orderController_;
    common::IClock& clock_;
};

} // namespace productionline
```

- [ ] **Step 4: `ProductionLineController.cpp` 구현**

```cpp
#include "ProductionLineController.h"
#include <algorithm>
#include <cmath>

namespace productionline {

ProductionLineController::ProductionLineController(ProductionQueueModel& model,
                                                     sampleclerk::SampleController& sampleController,
                                                     orderclerk::OrderController& orderController,
                                                     common::IClock& clock)
    : model_(model), sampleController_(sampleController), orderController_(orderController), clock_(clock) {}

void ProductionLineController::enqueue(const std::string& sampleId, int shortfallQty,
                                        const std::string& orderId) {
    auto sample = sampleController_.getSample(sampleId);
    if (!sample.has_value()) {
        return; // 존재하지 않는 시료 ID — 호출부(생산 담당자)가 승인 전 검증했어야 함
    }
    int actualQty = static_cast<int>(std::ceil(static_cast<double>(shortfallQty) / sample->yield));
    common::Duration totalDuration = sample->avgProductionTime * actualQty;

    ProductionJob job;
    job.orderId = orderId;
    job.sampleId = sampleId;
    job.shortfallQty = shortfallQty;
    job.actualQty = actualQty;
    job.totalDuration = totalDuration;
    job.startTime = model_.empty() ? std::optional<common::TimePoint>(clock_.now()) : std::nullopt;

    model_.pushBack(job);
}

void ProductionLineController::processCompletions() {
    common::TimePoint now = clock_.now();
    while (!model_.empty() && model_.front().startTime.has_value()) {
        common::TimePoint jobEnd = *model_.front().startTime + model_.front().totalDuration;
        if (now < jobEnd) break;

        ProductionJob finished = model_.front();
        model_.popFront();
        sampleController_.increaseStock(finished.sampleId, finished.actualQty);
        orderController_.setOrderStatus(finished.orderId, common::OrderStatus::CONFIRMED);

        if (!model_.empty()) {
            model_.front().startTime = jobEnd;
        }
    }
}

std::optional<JobView> ProductionLineController::getCurrentJob() {
    processCompletions();
    if (model_.empty()) return std::nullopt;
    const auto& job = model_.front();
    if (!job.startTime.has_value()) return std::nullopt;

    common::TimePoint now = clock_.now();
    common::TimePoint expectedCompletion = *job.startTime + job.totalDuration;
    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - *job.startTime).count();
    double total = std::chrono::duration_cast<std::chrono::duration<double>>(job.totalDuration).count();
    double progress = total > 0.0 ? std::min(100.0, (elapsed / total) * 100.0) : 100.0;

    auto sample = sampleController_.getSample(job.sampleId);
    std::string sampleName = sample.has_value() ? sample->name : job.sampleId;

    return JobView{job.orderId, job.sampleId, sampleName, job.shortfallQty, job.actualQty,
                   *job.startTime, expectedCompletion, progress};
}

std::vector<JobView> ProductionLineController::getQueue() {
    processCompletions();
    auto snapshot = model_.snapshot();
    std::vector<JobView> result;
    if (snapshot.empty()) return result;

    common::TimePoint cursor = snapshot.front().startTime.has_value()
        ? *snapshot.front().startTime + snapshot.front().totalDuration
        : clock_.now();

    for (size_t i = 1; i < snapshot.size(); ++i) {
        const auto& job = snapshot[i];
        common::TimePoint expectedCompletion = cursor + job.totalDuration;
        auto sample = sampleController_.getSample(job.sampleId);
        std::string sampleName = sample.has_value() ? sample->name : job.sampleId;
        result.push_back(JobView{job.orderId, job.sampleId, sampleName, job.shortfallQty, job.actualQty,
                                  cursor, expectedCompletion, 0.0});
        cursor = expectedCompletion;
    }
    return result;
}

} // namespace productionline
```

- [ ] **Step 5: 앱 vcxproj/필터에도 동일 파일 등록**

`SampleOrderSystem-ownovadoz-0715.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="ProductionLine\ProductionLineController.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="ProductionLine\ProductionLineController.h" />
```

`.vcxproj.filters`에도 동일하게 반영한다.

- [ ] **Step 6: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: 기존 31개 + 신규 4개(`ProductionLineController_Enqueue_StartsImmediatelyWhenQueueEmpty`,
`ProductionLineController_ProcessCompletion_UpdatesStockAndOrderStatus`,
`ProductionLineController_MultipleCompletions_ProcessedInFifoOrder`,
`ProductionLineController_GetQueue_ShowsWaitingJobsExcludingCurrent`) = `35/35 tests passed`

- [ ] **Step 7: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/ProductionLine \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "생산 라인: ProductionLineController (수율 계산/완료 반영) 추가"
```

---

### Task 3: ProductionLineView — 콘솔 화면 + 메인 메뉴 연결

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionLineView.h`
- Create: `SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionLineView.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/main.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+.filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/ProductionLine/ProductionLineViewTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+.filters)

**Interfaces:**
- Consumes: `productionline::ProductionLineController` (Task 2)
- Produces: `productionline::ProductionLineView::showCurrentJobScreen(ostream&)`,
  `showQueueScreen(ostream&)`. main.cpp가 메인 메뉴 "5"/"6"번에서 호출한다.

- [ ] **Step 1: 실패하는 테스트 작성 — `ProductionLineViewTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/ProductionLine/ProductionLineView.h"
#include "../../SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleController.h"
#include "../../SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderController.h"
#include "../Testing/MicroTest.h"
#include <sstream>

using namespace productionline;
using namespace sampleclerk;
using namespace orderclerk;
using namespace common;

TEST_CASE(ProductionLineView_CurrentJobScreen_EmptyQueueShowsMessage) {
    SampleModel sampleModel("plview_test_samples1.dat");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("plview_test_orders1.dat");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    ProductionQueueModel queueModel("plview_test_queue1.dat");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    ProductionLineView view(plController);

    std::ostringstream out;
    view.showCurrentJobScreen(out);
    REQUIRE(out.str().find("대기 중인 생산 없음") != std::string::npos);
}

TEST_CASE(ProductionLineView_CurrentJobScreen_ShowsRunningJob) {
    SampleModel sampleModel("plview_test_samples2.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(60), 1.0);
    OrderModel orderModel("plview_test_orders2.dat");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    auto order = orderController.createOrder("S-001", "삼성전자", 10);
    orderController.setOrderStatus(order.orderId, OrderStatus::PRODUCING);
    ProductionQueueModel queueModel("plview_test_queue2.dat");
    ProductionLineController plController(queueModel, sampleController, orderController, clock);
    plController.enqueue("S-001", 10, order.orderId);
    ProductionLineView view(plController);

    std::ostringstream out;
    view.showCurrentJobScreen(out);
    REQUIRE(out.str().find(order.orderId) != std::string::npos);
    REQUIRE(out.str().find("실리콘 웨이퍼") != std::string::npos);
}
```

- [ ] **Step 2: vcxproj/필터 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="ProductionLine\ProductionLineViewTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\ProductionLine\ProductionLineView.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\ProductionLine\ProductionLineView.h" />
```

`.vcxproj.filters`에도 동일하게 반영한다.

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `ProductionLineView.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 3: `ProductionLineView.h` 구현**

```cpp
#pragma once
#include <iostream>
#include "ProductionLineController.h"

namespace productionline {

class ProductionLineView {
public:
    explicit ProductionLineView(ProductionLineController& controller);

    void showCurrentJobScreen(std::ostream& out);
    void showQueueScreen(std::ostream& out);

private:
    ProductionLineController& controller_;
};

} // namespace productionline
```

- [ ] **Step 4: `ProductionLineView.cpp` 구현**

```cpp
#include "ProductionLineView.h"

namespace productionline {

ProductionLineView::ProductionLineView(ProductionLineController& controller) : controller_(controller) {}

void ProductionLineView::showCurrentJobScreen(std::ostream& out) {
    auto job = controller_.getCurrentJob();
    if (!job.has_value()) {
        out << "대기 중인 생산 없음\n";
        return;
    }
    out << "주문번호 " << job->orderId << "\n";
    out << "시료 " << job->sampleName << "\n";
    out << "부족분 " << job->shortfallQty << " ea\n";
    out << "실생산량 " << job->actualQty << " ea\n";
    out << "진행률 " << static_cast<int>(job->progressPercent) << "%\n";
}

void ProductionLineView::showQueueScreen(std::ostream& out) {
    auto queue = controller_.getQueue();
    if (queue.empty()) {
        out << "대기 중인 주문 없음\n";
        return;
    }
    out << "순서\t주문번호\t시료\t부족분\t실생산량\n";
    int index = 1;
    for (const auto& job : queue) {
        out << index++ << '\t' << job.orderId << '\t' << job.sampleName << '\t' << job.shortfallQty
            << '\t' << job.actualQty << '\n';
    }
}

} // namespace productionline
```

- [ ] **Step 5: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: 기존 35개 + 신규 2개(`ProductionLineView_CurrentJobScreen_EmptyQueueShowsMessage`,
`ProductionLineView_CurrentJobScreen_ShowsRunningJob`) = `37/37 tests passed`

- [ ] **Step 6: main.cpp에 메뉴 "5"/"6" 연결**

`main.cpp` 상단 include에 추가:

```cpp
#include "ProductionLine/ProductionQueueModel.h"
#include "ProductionLine/ProductionLineController.h"
#include "ProductionLine/ProductionLineView.h"
```

`main()` 함수 안, `orderclerk::OrderView orderView(orderController);` 다음 줄에 추가:

```cpp
    productionline::ProductionQueueModel queueModel("production_queue.dat");
    queueModel.load();
    productionline::ProductionLineController productionLineController(queueModel, sampleController,
                                                                        orderController, systemClock);
    productionline::ProductionLineView productionLineView(productionLineController);
```

메뉴 안내 문구 줄을 아래로 교체:

```cpp
        std::cout << "[1] 시료 등록  [2] 시료 예약(주문 접수)  [3] 시료 조회  [4] 시료 검색\n";
        std::cout << "[5] 생산 현황  [6] 대기 주문 확인  [0] 종료\n";
```

`else if (choice == 4) { ... }`와 `else if (choice == 0)` 사이에 추가:

```cpp
        } else if (choice == 5) {
            productionLineView.showCurrentJobScreen(std::cout);
        } else if (choice == 6) {
            productionLineView.showQueueScreen(std::cout);
```

`orderModel.save();` 다음 줄에 추가:

```cpp
    queueModel.save();
```

- [ ] **Step 7: 실행 파일로 수동 확인**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.exe`
- `5`, `6` 선택 시 큐가 비어 있으므로 "대기 중인 생산 없음"/"대기 중인 주문 없음"이 출력되는지 확인
  (실제로 큐를 채우는 승인 로직은 다음 플랜인 생산 담당자에서 연결되므로, 지금은 빈 상태 확인까지만
  가능하다 — 실제 생산 처리 로직은 위 TDD 테스트로 이미 검증됨)

- [ ] **Step 8: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/ProductionLine SampleOrderSystem-ownovadoz-0715/main.cpp \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "생산 라인: ProductionLineView 및 메인 메뉴 연결"
```

---

## 완료 후 상태

- 생산 큐 로직(등록/완료 판정/재고·주문 상태 반영/FIFO 처리)이 전부 TDD로 검증됨
- `productionline::ProductionLineController::enqueue`가 다음 모듈(생산 담당자) 플랜에서 승인 로직의
  일부로 호출됨
- 다음 단계: 생산 담당자 모듈(레이어 3) 플랜 — 스펙 5장/6장/10.5~10.6절 참고. `SampleController`,
  `OrderController`, `ProductionLineController` 셋을 모두 참조로 주입받아 승인/거절/출고 로직을 구현
