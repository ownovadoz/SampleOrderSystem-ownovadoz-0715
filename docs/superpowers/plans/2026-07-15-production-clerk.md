# 생산 담당자(Production Clerk) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

> 이 플랜은 시간 절약을 위해 이전 플랜들보다 간결하게 작성했다. 파일 등록(vcxproj/.filters) 방식,
> `Result` 패턴, `MicroTest` 테스트 하네스는 이전 3개 플랜(Sample/Order Clerk, Production Line)과 완전히
> 동일하므로 여기서는 반복하지 않는다 — 그 플랜들의 Step 패턴을 그대로 따라 파일을 추가하면 된다.

**Goal:** 주문 승인/거절, 출고 처리 메뉴. 이 모듈은 자체 저장 데이터가 없고, 시료·주문·생산라인 세 모듈의
공개 API만 호출하는 오케스트레이션 계층이다 (스펙 4.4절/5장/6장/10.5~10.6절).

**Files:**
- `ProductionClerk/ProductionClerkController.h/.cpp` — 승인/거절/출고 로직
- `ProductionClerk/ProductionClerkView.h/.cpp` — 콘솔 화면
- 테스트: `SampleOrderSystem-ownovadoz-0715.Tests/ProductionClerk/*Tests.cpp`

**Interfaces (생산자):**
```cpp
class ProductionClerkController {
public:
    ProductionClerkController(sampleclerk::SampleController&, orderclerk::OrderController&,
                               productionline::ProductionLineController&);

    std::vector<common::Order> listPendingApprovals() const;   // RESERVED만
    void approve(const std::string& orderId);
    void reject(const std::string& orderId);
    std::vector<common::Order> listShippable() const;           // CONFIRMED만
    common::Result ship(const std::string& orderId);
};
```

**핵심 로직 (approve):**

```cpp
void ProductionClerkController::approve(const std::string& orderId) {
    auto order = orderController_.getOrder(orderId);
    if (!order.has_value() || order->status != common::OrderStatus::RESERVED) return;

    int available = sampleController_.getStock(order->sampleId)
                  - orderController_.getReservedQuantity(order->sampleId);

    if (available >= order->quantity) {
        orderController_.setOrderStatus(orderId, common::OrderStatus::CONFIRMED);
    } else {
        int shortfall = order->quantity - available;
        productionLineController_.enqueue(order->sampleId, shortfall, orderId);
        orderController_.setOrderStatus(orderId, common::OrderStatus::PRODUCING);
    }
}
```

`reject`는 `orderController_.setOrderStatus(orderId, REJECTED)`만 호출. `ship`은 주문이 `CONFIRMED`인지
확인 후 `sampleController_.decreaseStock(order->sampleId, order->quantity)` → 성공하면
`orderController_.setOrderStatus(orderId, RELEASE)`, 실패하면 `Result::failure(...)` 반환.

**View 화면 (스펙 10.5/10.6):** 승인 대기(RESERVED) 목록 → 번호 선택 → 승인 시 가용재고/부족분 안내 후
승인·거절 실행. 출고 가능(CONFIRMED) 목록 → 번호 선택 → 즉시 출고. 화면 문구는 시료 담당자/주문 담당자
View와 동일한 스타일(표+프롬프트)로 맞춘다.

---

### Task 1: ProductionClerkController (TDD)

- [ ] `SampleController`+`OrderController`+`ProductionLineController`를 준비해 다음을 검증하는 테스트 작성:
  재고 충분 시 즉시 `CONFIRMED`, 재고 부족 시 `PRODUCING`+`ProductionLineController`에 enqueue됐는지
  (`getCurrentJob()`으로 확인), `reject`가 `REJECTED`로 바꾸는지, `ship`이 `RELEASE`+재고 차감하는지,
  `CONFIRMED`가 아닌 주문을 `ship` 시도하면 실패하는지
- [ ] 컴파일 실패 확인 → 위 코드대로 구현 → 통과 확인
- [ ] 커밋: "생산 담당자: ProductionClerkController (승인/거절/출고) 추가"

### Task 2: ProductionClerkView + 메인 메뉴 연결

- [ ] 승인목록/출고목록 화면에 대해 "목록 없음"/"승인 후 상태 전환 문구 포함" 정도의 최소 View 테스트
      작성 → 구현 → 통과 확인
- [ ] `main.cpp`에 `[7] 주문 승인/거절`, `[8] 출고 처리` 메뉴 추가, 다른 모듈과 같은 방식으로 인스턴스
      생성/연결
- [ ] 실행 파일로 수동 확인: 시료 등록 → 재고 부족한 수량으로 주문 접수 → 승인 → PRODUCING 확인 →
      생산 현황에서 큐에 잡히는지 확인
- [ ] 커밋: "생산 담당자: ProductionClerkView 및 메인 메뉴 연결"

## 완료 후 상태

승인/거절/출고 전체 흐름이 동작. 다음 단계: 모니터링 모듈(레이어 4) — 스펙 5장/10.7~10.8절, 읽기 전용이라
가장 간단하다.
