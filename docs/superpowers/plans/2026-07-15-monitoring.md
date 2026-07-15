# 모니터링(Monitoring) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

> 이전 플랜들과 동일하게 간결하게 작성. 파일 등록/테스트 하네스 방식은 반복하지 않는다.

**Goal:** 주문량/재고량 확인 메뉴. 자체 저장 데이터 없이 시료 담당자·주문 담당자의 공개 API만 읽는
읽기 전용 모듈 (스펙 4.5절/5장/10.7~10.8절). 5개 모듈 중 가장 단순하다.

**Files:**
- `Monitoring/MonitoringController.h/.cpp`
- `Monitoring/MonitoringView.h/.cpp`
- 테스트: `SampleOrderSystem-ownovadoz-0715.Tests/Monitoring/*Tests.cpp`

**Interfaces (생산자):**

```cpp
struct StockOverviewItem {
    common::Sample sample;
    std::string label; // "여유" | "부족" | "고갈"
};

class MonitoringController {
public:
    MonitoringController(sampleclerk::SampleController&, orderclerk::OrderController&);

    std::map<common::OrderStatus, int> getOrderCountsByStatus() const; // REJECTED 제외
    std::vector<StockOverviewItem> getStockOverview() const;
};
```

**핵심 로직:**

- `getOrderCountsByStatus`: `RESERVED/CONFIRMED/PRODUCING/RELEASE` 각각
  `orderController_.listOrdersByStatus(status).size()`로 집계 (`REJECTED`는 애초에 조회하지 않음).
- `getStockOverview`: 시료마다 `stock = sampleController_.getStock(id)`, 수요 =
  `RESERVED+CONFIRMED+PRODUCING` 세 상태의 `listOrdersByStatus(status)`를 순회하며 `sampleId`가 일치하는
  주문의 `quantity` 합산. 라벨 판정(스펙 10.8, 우선순위 순서): `stock == 0` → "고갈", `stock < 수요` →
  "부족", 그 외 → "여유".

**View 화면:** 주문량 확인은 상태별 건수 표. 재고량 확인은 시료명/재고/라벨 표. 문구 스타일은 다른 모듈
View와 통일.

---

### Task 1: MonitoringController (TDD)

- [ ] 테스트: `REJECTED` 주문이 있어도 `getOrderCountsByStatus`에 안 잡히는지, 각 상태 건수가 정확한지;
      재고 0인 시료는 "고갈", 재고 있지만 수요(세 상태 합)보다 적으면 "부족", 수요 이상이면 "여유"인지
      (세 케이스 모두 최소 1개씩)
- [ ] 컴파일 실패 확인 → 구현 → 통과 확인
- [ ] 커밋: "모니터링: MonitoringController 추가"

### Task 2: MonitoringView + 메인 메뉴 연결

- [ ] 최소 View 테스트(빈 데이터/정상 데이터 각 1개) → 구현 → 통과 확인
- [ ] `main.cpp`에 `[9] 주문량 확인`, `[10] 재고량 확인` 메뉴 추가
- [ ] 실행 파일로 수동 확인: 시료/주문 몇 개 만든 뒤 모니터링 화면에서 정확히 반영되는지 확인
- [ ] 커밋: "모니터링: MonitoringView 및 메인 메뉴 연결"

## 완료 후 상태

5개 모듈(시료담당자/주문담당자/생산라인/생산담당자/모니터링) 전부 구현 완료. 이후에는 통합 테스트(전체
주문 흐름 end-to-end)와 CLAUDE.md/README 등 문서 마무리만 남는다.
