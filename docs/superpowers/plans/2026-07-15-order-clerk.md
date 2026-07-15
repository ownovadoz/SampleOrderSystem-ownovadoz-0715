# 주문 담당자(Order Clerk) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 시료 예약(주문 접수) 메뉴와 주문(Order) 레코드 저장/조회를 담당하는 주문 담당자 모듈을 구현한다.
의존 계층의 레이어 1로, 시료 담당자(레이어 0, `2026-07-15-sample-clerk.md`에서 구현 완료)의 공개 API에만
의존한다.

**Architecture:** `SampleOrderSystem-ownovadoz-0715/OrderClerk/`에 `OrderModel`(저장) → `OrderController`
(주문번호 채번, 검증, 조회) → `OrderView`(콘솔 화면) 3계층. `OrderController`는 생성자로
`sampleclerk::SampleController&`와 `common::IClock&`를 주입받는다.

**Tech Stack:** C++20. `common::Order`, `common::OrderStatus`, `common::IClock`/`FakeClock`,
`sampleclerk::SampleController`(Sample Clerk 플랜 산출물).

> ⚠️ **테스트 하네스 변경 안내**: Foundation이 gmock/gtest(NuGet, 단일 앱 프로젝트)로 바뀌었다
> (`docs/superpowers/plans/2026-07-15-foundation.md` 참고). 아래 스텝에서 `TEST_CASE`/`REQUIRE`/
> `MicroTest.h`/별도 `.Tests` 프로젝트가 나오면 `2026-07-15-sample-clerk.md` 상단의 변경 안내와 동일하게
> `TEST`/`EXPECT_*`/`<gtest/gtest.h>`/단일 앱 프로젝트로 치환해서 적용한다.

## Global Constraints

- 아키텍처 스펙 4.2절(데이터 구조), 5장(공개 API), 10.4절(화면 명세)을 따른다.
- 주문번호 형식: `ORD-{YYYYMMDD}-{4자리 일련번호}`, 일련번호는 그날 생성된 주문 수만큼 0001부터 증가
  (스펙 10.4절에서 정한 규칙). 날짜는 `common::IClock`에서 얻은 현재 시각을 UTC 기준 `YYYYMMDD`로 변환.
- `orders.json` 파일 포맷: JSON 포맷(`Common/FileRepository.h`의 `FileRepository<T>` +
  `Common::Sample`/`Common::Order`의 `ToJson`/`FromJson`, PRD.md 3장 및 `DataPersistence-ownovadoz-0715`
  PoC 참고). 상태는 `common::OrderStatus`의 선언 순서(`RESERVED=0, REJECTED=1, PRODUCING=2, CONFIRMED=3,
  RELEASE=4`)를 정수로 저장한다.
- 에러 처리는 Sample Clerk 플랜에서 도입한 `common::Result` 패턴을 따르되, 주문 생성은 성공 시 생성된
  주문번호를 돌려줘야 하므로 이 모듈 전용 `OrderController::CreateOrderResult{bool ok; string error;
  string orderId;}`를 추가로 쓴다 (다른 모듈이 재사용할 일이 없어 `Common/`이 아니라 `OrderController.h`에
  정의).

---

### Task 1: 날짜 포맷 유틸리티 + OrderModel — 파일 저장/복원 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/Common/DateFormat.h`
- Create: `SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderModel.h`
- Create: `SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderModel.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+.filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/OrderClerk/OrderModelTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+.filters)

**Interfaces:**
- Consumes: `common::Order`, `common::OrderStatus` (Foundation), `common::TimePoint` (Foundation)
- Produces: `common::formatDateYYYYMMDD(TimePoint) -> string` (`Common/DateFormat.h`); `orderclerk::OrderModel`
  — `load()`, `save() -> bool`, `insert(Order)`, `find(id) -> optional<Order>`, `findAll() -> vector<Order>`,
  `updateStatus(id, OrderStatus)`. Task 2(Controller)가 이 타입을 그대로 사용한다.

- [ ] **Step 1: 날짜 포맷 유틸리티 작성 — `Common/DateFormat.h`**

```cpp
#pragma once
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "Time.h"

namespace common {

inline std::string formatDateYYYYMMDD(TimePoint tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tmStruct{};
    gmtime_s(&tmStruct, &t);
    std::ostringstream oss;
    oss << std::put_time(&tmStruct, "%Y%m%d");
    return oss.str();
}

} // namespace common
```

- [ ] **Step 2: 실패하는 테스트 작성 — `OrderModelTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderModel.h"
#include "../Testing/MicroTest.h"
#include <cstdio>

using namespace orderclerk;
using namespace common;

TEST_CASE(OrderModel_LoadFromMissingFile_StartsEmpty) {
    OrderModel model("nonexistent_orders_test.dat");
    model.load();
    REQUIRE(model.findAll().empty());
}

TEST_CASE(OrderModel_InsertAndFind) {
    OrderModel model("orders_test_insert.dat");
    Order o{"ORD-20260416-0001", "S-001", "삼성전자", 100, OrderStatus::RESERVED};
    model.insert(o);
    auto found = model.find("ORD-20260416-0001");
    REQUIRE(found.has_value());
    REQUIRE(found->customerName == "삼성전자");
    REQUIRE(found->quantity == 100);
}

TEST_CASE(OrderModel_SaveThenLoad_RoundTrips) {
    const std::string path = "orders_test_roundtrip.dat";
    std::remove(path.c_str());
    {
        OrderModel model(path);
        model.insert(Order{"ORD-20260416-0001", "S-001", "삼성전자", 100, OrderStatus::RESERVED});
        model.insert(Order{"ORD-20260416-0002", "S-002", "SK하이닉스", 50, OrderStatus::CONFIRMED});
        REQUIRE(model.save());
    }
    {
        OrderModel model(path);
        model.load();
        REQUIRE(model.findAll().size() == 2);
        auto found = model.find("ORD-20260416-0002");
        REQUIRE(found.has_value());
        REQUIRE(found->status == OrderStatus::CONFIRMED);
    }
    std::remove(path.c_str());
}

TEST_CASE(OrderModel_UpdateStatus_ChangesValue) {
    OrderModel model("orders_test_update.dat");
    model.insert(Order{"ORD-20260416-0001", "S-001", "삼성전자", 100, OrderStatus::RESERVED});
    model.updateStatus("ORD-20260416-0001", OrderStatus::CONFIRMED);
    REQUIRE(model.find("ORD-20260416-0001")->status == OrderStatus::CONFIRMED);
}
```

- [ ] **Step 3: vcxproj/필터 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그(`</ItemGroup>`)
바로 앞에 추가:

```xml
    <ClCompile Include="OrderClerk\OrderModelTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderModel.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 바로 앞에 추가:

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\DateFormat.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Order.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\OrderStatus.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderModel.h" />
```

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="OrderClerk\OrderModelTests.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderModel.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\DateFormat.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Order.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\OrderStatus.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderModel.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
```

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `OrderModel.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 4: `OrderModel.h` 구현**

```cpp
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
```

- [ ] **Step 5: `OrderModel.cpp` 구현**

```cpp
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
```

- [ ] **Step 6: 앱 vcxproj/필터에도 동일 파일 등록**

`SampleOrderSystem-ownovadoz-0715.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="OrderClerk\OrderModel.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="Common\DateFormat.h" />
    <ClInclude Include="OrderClerk\OrderModel.h" />
```

`SampleOrderSystem-ownovadoz-0715.vcxproj.filters`도 동일한 파일들을 각각 "소스 파일"/"헤더 파일" 필터로
추가한다.

- [ ] **Step 7: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: 기존 16개 + 신규 4개(`OrderModel_LoadFromMissingFile_StartsEmpty`, `OrderModel_InsertAndFind`,
`OrderModel_SaveThenLoad_RoundTrips`, `OrderModel_UpdateStatus_ChangesValue`) = `20/20 tests passed`

- [ ] **Step 8: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/Common/DateFormat.h \
        SampleOrderSystem-ownovadoz-0715/OrderClerk \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "주문 담당자: OrderModel (파일 저장/복원) 추가"
```

---

### Task 2: OrderController — 채번/검증/조회 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderController.h`
- Create: `SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderController.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+.filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/OrderClerk/OrderControllerTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+.filters)

**Interfaces:**
- Consumes: `orderclerk::OrderModel` (Task 1), `sampleclerk::SampleController::getSample`(id)`
  (Sample Clerk 플랜), `common::IClock::now()` (Foundation)
- Produces: `orderclerk::OrderController` — `createOrder(sampleId, customerName, quantity) ->
  CreateOrderResult{ok, error, orderId}`, `getOrder(id) -> optional<Order>`,
  `listOrdersByStatus(status) -> vector<Order>`, `setOrderStatus(id, status)`,
  `getReservedQuantity(sampleId) -> int`, `getSampleInfo(sampleId) -> optional<Sample>`. 생산 담당자/생산
  라인/모니터링 플랜이 이 시그니처를 그대로 호출한다.

- [ ] **Step 1: 실패하는 테스트 작성 — `OrderControllerTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderController.h"
#include "../../SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleController.h"
#include "../Testing/MicroTest.h"
#include <chrono>

using namespace orderclerk;
using namespace sampleclerk;
using namespace common;

namespace {
TimePoint dayZero() { return std::chrono::system_clock::from_time_t(0); } // 1970-01-01 UTC
}

TEST_CASE(OrderController_CreateOrder_Succeeds) {
    SampleModel sampleModel("order_ctrl_test_samples1.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);

    OrderModel orderModel("order_ctrl_test_orders1.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto result = orderController.createOrder("S-001", "삼성전자", 100);
    REQUIRE(result.ok);
    REQUIRE(result.orderId == "ORD-19700101-0001");

    auto order = orderController.getOrder(result.orderId);
    REQUIRE(order.has_value());
    REQUIRE(order->status == OrderStatus::RESERVED);
    REQUIRE(order->quantity == 100);
}

TEST_CASE(OrderController_CreateOrder_UnknownSample_Fails) {
    SampleModel sampleModel("order_ctrl_test_samples2.dat");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("order_ctrl_test_orders2.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto result = orderController.createOrder("S-999", "삼성전자", 10);
    REQUIRE(!result.ok);
}

TEST_CASE(OrderController_CreateOrder_InvalidQuantity_Fails) {
    SampleModel sampleModel("order_ctrl_test_samples3.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders3.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto zero = orderController.createOrder("S-001", "삼성전자", 0);
    REQUIRE(!zero.ok);
    auto negative = orderController.createOrder("S-001", "삼성전자", -5);
    REQUIRE(!negative.ok);
}

TEST_CASE(OrderController_SequenceIncrementsWithinSameDay) {
    SampleModel sampleModel("order_ctrl_test_samples4.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders4.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto first = orderController.createOrder("S-001", "삼성전자", 10);
    auto second = orderController.createOrder("S-001", "SK하이닉스", 20);
    REQUIRE(first.orderId == "ORD-19700101-0001");
    REQUIRE(second.orderId == "ORD-19700101-0002");
}

TEST_CASE(OrderController_SequenceResetsOnNewDay) {
    SampleModel sampleModel("order_ctrl_test_samples5.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders5.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    orderController.createOrder("S-001", "삼성전자", 10);
    clock.advance(std::chrono::seconds(24 * 60 * 60));
    auto nextDay = orderController.createOrder("S-001", "삼성전자", 5);
    REQUIRE(nextDay.orderId == "ORD-19700102-0001");
}

TEST_CASE(OrderController_GetReservedQuantity_SumsConfirmedAndProducingOnly) {
    SampleModel sampleModel("order_ctrl_test_samples6.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("order_ctrl_test_orders6.dat");
    FakeClock clock(dayZero());
    OrderController orderController(orderModel, sampleController, clock);

    auto reserved = orderController.createOrder("S-001", "A", 10);   // RESERVED - 제외
    auto confirmed = orderController.createOrder("S-001", "B", 20);  // CONFIRMED로 변경 - 합산
    auto producing = orderController.createOrder("S-001", "C", 30);  // PRODUCING으로 변경 - 합산
    auto released = orderController.createOrder("S-001", "D", 40);   // RELEASE로 변경 - 제외

    orderController.setOrderStatus(confirmed.orderId, OrderStatus::CONFIRMED);
    orderController.setOrderStatus(producing.orderId, OrderStatus::PRODUCING);
    orderController.setOrderStatus(released.orderId, OrderStatus::RELEASE);

    REQUIRE(orderController.getReservedQuantity("S-001") == 50); // 20 + 30
}
```

- [ ] **Step 2: vcxproj/필터 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="OrderClerk\OrderControllerTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderController.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleController.cpp" />
```

(주: `SampleController.cpp`/`SampleModel.cpp`/`Clock.cpp` 등 Sample Clerk·Foundation 플랜에서 이미 추가된
항목은 그대로 둔다. 위 두 줄만 새로 추가한다.)

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderController.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Result.h" />
```

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`에도 위 두 소스/두 헤더를 각각 "소스 파일"/"헤더
파일" 필터로 추가한다.

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `OrderController.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 3: `OrderController.h` 구현**

```cpp
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "OrderModel.h"
#include "../SampleClerk/SampleController.h"
#include "../Common/Clock.h"

namespace orderclerk {

struct CreateOrderResult {
    bool ok;
    std::string error;
    std::string orderId;

    static CreateOrderResult success(std::string orderId) {
        return CreateOrderResult{true, "", std::move(orderId)};
    }
    static CreateOrderResult failure(std::string err) {
        return CreateOrderResult{false, std::move(err), ""};
    }
};

class OrderController {
public:
    OrderController(OrderModel& model, sampleclerk::SampleController& sampleController, common::IClock& clock);

    CreateOrderResult createOrder(const std::string& sampleId, const std::string& customerName, int quantity);
    std::optional<common::Order> getOrder(const std::string& orderId) const;
    std::vector<common::Order> listOrdersByStatus(common::OrderStatus status) const;
    void setOrderStatus(const std::string& orderId, common::OrderStatus status);
    int getReservedQuantity(const std::string& sampleId) const;
    std::optional<common::Sample> getSampleInfo(const std::string& sampleId) const;

private:
    std::string generateOrderId() const;

    OrderModel& model_;
    sampleclerk::SampleController& sampleController_;
    common::IClock& clock_;
};

} // namespace orderclerk
```

- [ ] **Step 4: `OrderController.cpp` 구현**

```cpp
#include "OrderController.h"
#include "../Common/DateFormat.h"
#include <iomanip>
#include <sstream>

namespace orderclerk {

OrderController::OrderController(OrderModel& model, sampleclerk::SampleController& sampleController,
                                  common::IClock& clock)
    : model_(model), sampleController_(sampleController), clock_(clock) {}

std::string OrderController::generateOrderId() const {
    std::string dateStr = common::formatDateYYYYMMDD(clock_.now());
    std::string prefix = "ORD-" + dateStr + "-";
    int count = 0;
    for (const auto& order : model_.findAll()) {
        if (order.id.rfind(prefix, 0) == 0) {
            ++count;
        }
    }
    std::ostringstream oss;
    oss << prefix << std::setw(4) << std::setfill('0') << (count + 1);
    return oss.str();
}

CreateOrderResult OrderController::createOrder(const std::string& sampleId,
                                                const std::string& customerName, int quantity) {
    if (quantity < 1) {
        return CreateOrderResult::failure("주문 수량은 1 이상이어야 합니다");
    }
    if (!sampleController_.getSample(sampleId).has_value()) {
        return CreateOrderResult::failure("존재하지 않는 시료 ID입니다: " + sampleId);
    }
    std::string orderId = generateOrderId();
    common::Order order{orderId, sampleId, customerName, quantity, common::OrderStatus::RESERVED};
    model_.insert(order);
    return CreateOrderResult::success(orderId);
}

std::optional<common::Order> OrderController::getOrder(const std::string& orderId) const {
    return model_.find(orderId);
}

std::vector<common::Order> OrderController::listOrdersByStatus(common::OrderStatus status) const {
    std::vector<common::Order> result;
    for (const auto& order : model_.findAll()) {
        if (order.status == status) {
            result.push_back(order);
        }
    }
    return result;
}

void OrderController::setOrderStatus(const std::string& orderId, common::OrderStatus status) {
    model_.updateStatus(orderId, status);
}

int OrderController::getReservedQuantity(const std::string& sampleId) const {
    int total = 0;
    for (const auto& order : model_.findAll()) {
        if (order.sampleId != sampleId) continue;
        if (order.status == common::OrderStatus::CONFIRMED || order.status == common::OrderStatus::PRODUCING) {
            total += order.quantity;
        }
    }
    return total;
}

std::optional<common::Sample> OrderController::getSampleInfo(const std::string& sampleId) const {
    return sampleController_.getSample(sampleId);
}

} // namespace orderclerk
```

- [ ] **Step 5: 앱 vcxproj/필터에도 동일 파일 등록**

`SampleOrderSystem-ownovadoz-0715.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="OrderClerk\OrderController.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="OrderClerk\OrderController.h" />
```

`.vcxproj.filters`에도 동일하게 반영한다.

- [ ] **Step 6: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: 기존 20개 + 신규 6개(`OrderController_CreateOrder_Succeeds`,
`OrderController_CreateOrder_UnknownSample_Fails`, `OrderController_CreateOrder_InvalidQuantity_Fails`,
`OrderController_SequenceIncrementsWithinSameDay`, `OrderController_SequenceResetsOnNewDay`,
`OrderController_GetReservedQuantity_SumsConfirmedAndProducingOnly`) = `26/26 tests passed`

- [ ] **Step 7: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/OrderClerk \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "주문 담당자: OrderController (채번/검증/조회) 추가"
```

---

### Task 3: OrderView — 시료 예약 화면 + 메인 메뉴 연결

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderView.h`
- Create: `SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderView.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/main.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+.filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/OrderClerk/OrderViewTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+.filters)

**Interfaces:**
- Consumes: `orderclerk::OrderController` (Task 2)
- Produces: `orderclerk::OrderView::showReserveScreen(istream&, ostream&)`. main.cpp가 메인 메뉴 "2"번에서
  호출한다.

- [ ] **Step 1: 실패하는 테스트 작성 — `OrderViewTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/OrderClerk/OrderView.h"
#include "../Testing/MicroTest.h"
#include <sstream>

using namespace orderclerk;
using namespace sampleclerk;
using namespace common;

TEST_CASE(OrderView_ReserveScreen_SuccessShowsOrderInfo) {
    SampleModel sampleModel("orderview_test_samples1.dat");
    SampleController sampleController(sampleModel);
    sampleController.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.9);
    OrderModel orderModel("orderview_test_orders1.dat");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    OrderView view(orderController);

    std::istringstream in("S-001\n삼성전자 파운드리\n200\n");
    std::ostringstream out;
    view.showReserveScreen(in, out);

    REQUIRE(out.str().find("예약 접수 완료") != std::string::npos);
    REQUIRE(out.str().find("ORD-19700101-0001") != std::string::npos);
    REQUIRE(out.str().find("실리콘 웨이퍼") != std::string::npos);
    REQUIRE(out.str().find("RESERVED") != std::string::npos);
}

TEST_CASE(OrderView_ReserveScreen_UnknownSample_ShowsError) {
    SampleModel sampleModel("orderview_test_samples2.dat");
    SampleController sampleController(sampleModel);
    OrderModel orderModel("orderview_test_orders2.dat");
    FakeClock clock(std::chrono::system_clock::from_time_t(0));
    OrderController orderController(orderModel, sampleController, clock);
    OrderView view(orderController);

    std::istringstream in("S-999\n삼성전자\n10\n");
    std::ostringstream out;
    view.showReserveScreen(in, out);

    REQUIRE(out.str().find("접수 실패") != std::string::npos);
}
```

- [ ] **Step 2: vcxproj/필터 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="OrderClerk\OrderViewTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderView.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\OrderClerk\OrderView.h" />
```

`.vcxproj.filters`에도 동일하게 반영한다.

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `OrderView.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 3: `OrderView.h` 구현**

```cpp
#pragma once
#include <iostream>
#include "OrderController.h"

namespace orderclerk {

class OrderView {
public:
    explicit OrderView(OrderController& controller);

    void showReserveScreen(std::istream& in, std::ostream& out);

private:
    OrderController& controller_;
};

} // namespace orderclerk
```

- [ ] **Step 4: `OrderView.cpp` 구현**

```cpp
#include "OrderView.h"

namespace orderclerk {

OrderView::OrderView(OrderController& controller) : controller_(controller) {}

void OrderView::showReserveScreen(std::istream& in, std::ostream& out) {
    out << "시료 ID > ";
    std::string sampleId;
    std::getline(in, sampleId);

    out << "고객명 > ";
    std::string customerName;
    std::getline(in, customerName);

    out << "주문 수량 > ";
    int quantity = 0;
    in >> quantity;
    in.ignore();

    auto result = controller_.createOrder(sampleId, customerName, quantity);
    if (!result.ok) {
        out << "접수 실패: " << result.error << "\n";
        return;
    }

    auto sample = controller_.getSampleInfo(sampleId);
    out << "예약 접수 완료\n";
    out << "주문번호 " << result.orderId << "\n";
    out << "시료 " << (sample.has_value() ? sample->name : sampleId) << "\n";
    out << "고객 " << customerName << "\n";
    out << "수량 " << quantity << " ea\n";
    out << "현재 상태 RESERVED\n";
}

} // namespace orderclerk
```

- [ ] **Step 5: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: 기존 26개 + 신규 2개(`OrderView_ReserveScreen_SuccessShowsOrderInfo`,
`OrderView_ReserveScreen_UnknownSample_ShowsError`) = `28/28 tests passed`

- [ ] **Step 6: main.cpp에 메뉴 "2" 연결**

`main.cpp` 상단 include에 추가:

```cpp
#include "OrderClerk/OrderModel.h"
#include "OrderClerk/OrderController.h"
#include "OrderClerk/OrderView.h"
#include "Common/Clock.h"
```

`main()` 함수 안, `sampleclerk::SampleView sampleView(sampleController);` 다음 줄에 추가:

```cpp
    common::SystemClock systemClock;
    orderclerk::OrderModel orderModel("orders.dat");
    orderModel.load();
    orderclerk::OrderController orderController(orderModel, sampleController, systemClock);
    orderclerk::OrderView orderView(orderController);
```

메뉴 안내 문구 줄을 아래로 교체:

```cpp
        std::cout << "[1] 시료 등록  [2] 시료 예약(주문 접수)  [3] 시료 조회  [4] 시료 검색  [0] 종료\n";
```

`if (choice == 1) { ... } else if (choice == 2) { ... }` 분기를 아래로 교체 (기존 2/3번 메뉴가
3/4번으로 밀림):

```cpp
        if (choice == 1) {
            sampleView.showRegisterScreen(std::cin, std::cout);
        } else if (choice == 2) {
            orderView.showReserveScreen(std::cin, std::cout);
        } else if (choice == 3) {
            sampleView.showListScreen(std::cout);
        } else if (choice == 4) {
            sampleView.showSearchScreen(std::cin, std::cout);
        } else if (choice == 0) {
            break;
        }
```

`sampleModel.save();` 다음 줄에 추가:

```cpp
    orderModel.save();
```

- [ ] **Step 7: 실행 파일로 수동 확인**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.exe`
- `1`로 시료 하나 등록 → `2`로 그 시료에 대해 주문 접수 → "예약 접수 완료"와 `ORD-{오늘날짜}-0001` 형식의
  주문번호가 나오는지 확인
- 같은 시료로 한 번 더 `2` 실행 → 주문번호 일련번호가 `0002`로 증가하는지 확인
- `0`으로 종료 후 재실행 → `orders.dat` 파일이 저장/복원되는지 확인(직접 파일 내용을 열어 확인 가능)

- [ ] **Step 8: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/OrderClerk SampleOrderSystem-ownovadoz-0715/main.cpp \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "주문 담당자: OrderView 및 메인 메뉴 연결"
```

---

## 완료 후 상태

- 시료 예약(주문 접수)이 실제로 동작하고, `orders.dat`에 저장/복원됨
- `orderclerk::OrderController`의 공개 API(`getOrder`, `listOrdersByStatus`, `setOrderStatus`,
  `getReservedQuantity`)가 다음 모듈(생산 라인, 생산 담당자, 모니터링) 플랜에서 재사용됨
- 다음 단계: 생산 라인 모듈(레이어 2) 플랜 — 스펙 4.3절/5장/6장/10.9~10.10절 참고. `OrderController`와
  `SampleController`를 참조로 주입받아 완료 시 상태 반영에 사용
