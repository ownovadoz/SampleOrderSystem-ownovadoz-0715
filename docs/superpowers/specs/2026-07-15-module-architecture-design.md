# 모듈 아키텍처 설계

- 작성일: 2026-07-15
- 관련 문서: `PRD.md`, `docs/requirements/*.md`
- 상태: 브레인스토밍 확정안 (구현 계획 수립 전 최종 검토용)

## 1. 배경 및 목표

`docs/requirements/*.md`는 메뉴 단위(시료 관리/주문 접수·승인·거절/생산 라인/모니터링/출고 처리)로 나뉘어
있었지만, 실제 개발은 **역할(에이전트) 단위로 5개 모듈**을 나눠서 진행하기로 했다. 이 문서는 그 5개 모듈의
경계, 데이터 소유권, 모듈 간 인터페이스를 정의해서 각 모듈을 서로 다른 에이전트가 독립적으로 개발할 수 있게
하는 것이 목표다.

## 2. 모듈 구성 및 책임

| 모듈 | 책임 | 대응 메뉴 |
|---|---|---|
| 시료 담당자 | 시료 등록/조회/검색, 재고 수량 관리 | 시료 등록, 시료 조회, 시료 검색 |
| 주문 담당자 | 주문 생성(채번, 시료 정보 조회) 및 저장 | 시료 예약(주문 접수) |
| 생산 담당자 | 주문 승인/거절, 출고 처리 (자체 저장 데이터 없음) | 접수된 주문 목록, 주문 승인, 주문 거절, 출고 처리 |
| 생산 라인 | 생산 큐 관리, 생산 진행/완료 계산 | 생산 현황, 대기 주문 확인 |
| 모니터링 | 주문/재고 현황 조회 (자체 저장 데이터 없음, 읽기 전용) | 주문량 확인, 재고량 확인 |

> 출처: `docs/requirements.pdf` p.6 (역할별 흐름도 — 고객/주문담당자/생산담당자), p.10 (메인 메뉴 표) — 단,
> 5모듈 분할 자체와 "출고 처리를 생산 담당자에 포함"은 이번 설계 논의에서 결정한 사항이며 명세에 직접
> 나와 있지 않다.

## 3. 모듈 의존 관계 (계층)

```
Layer 0 (leaf)  : 시료 담당자   — 아무 모듈도 의존하지 않음
Layer 1         : 주문 담당자   — 시료 담당자에 의존
Layer 2         : 생산 라인     — 시료 담당자, 주문 담당자에 의존
Layer 3         : 생산 담당자   — 시료 담당자, 주문 담당자, 생산 라인에 의존
Layer 4 (read)  : 모니터링      — 시료 담당자, 주문 담당자에 의존 (읽기 전용)
```

모든 의존 화살표가 아래 레이어 → 위 레이어 한 방향으로만 향하도록 설계했다 (비순환, DAG). 이렇게 설계한
이유는, 브레인스토밍 과정에서 재고를 생산 담당자가 소유하는 안을 검토했을 때 "시료 담당자 ↔ 생산 담당자"
상호 의존이 생기는 문제를 발견했기 때문이다 — 재고를 시료 담당자(레이어 0)에 두어 이 문제를 해소했다.

## 4. 모듈별 데이터 구조

원칙: **파생 가능한 값은 저장하지 않고 조회 시점에 계산한다.** 같은 정보를 두 곳에 나눠 저장하면 한쪽만
갱신되는 불일치가 생기기 쉽기 때문이다 (예: 생산 라인의 "현재 작업"과 "대기 큐"를 물리적으로 나눠 저장하지
않고, 큐 하나로 합친 것도 같은 이유).

### 4.1 시료 담당자 — `samples.dat`

```
struct Sample {
    string id;                   // 예: S-001
    string name;
    Duration avgProductionTime;
    double yield;                 // 0~1
    int stock;                    // 원재고 수량 — Sample과 한 몸으로 저장 (재고를 별도 map으로 분리하지 않음)
};
map<string, Sample> samples;      // key: sampleId
```

### 4.2 주문 담당자 — `orders.dat`

```
struct Order {
    string id;               // 예: ORD-20260416-0043, 생성 시 주문 담당자가 채번
    string sampleId;
    string customerName;
    int quantity;
    OrderStatus status;      // RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE
};
map<string, Order> orders;   // key: orderId
```

**저장하지 않고 매번 계산하는 값**: 시료별 선점 재고 합(`getReservedQuantity`) — CONFIRMED/PRODUCING 상태
주문들의 수량 합을 그때그때 `orders`를 순회해 계산한다. 별도 누적 필드로 캐싱하지 않는다.

### 4.3 생산 라인 — `production_queue.dat`

```
struct ProductionJob {
    string orderId;
    string sampleId;
    int shortfallQty;
    int actualQty;              // ceil(shortfallQty / yield) — 큐 등록 시점에 계산해 고정
    Duration totalDuration;     // avgProductionTime * actualQty — 마찬가지로 큐 등록 시점에 고정
    optional<Time> startTime;   // 비어있으면 대기 중. 값이 있으면 "생산 중"
};
deque<ProductionJob> productionQueue;
```

- **현재 생산 중인 작업** = `productionQueue.front()` (단, `startTime`이 채워져 있을 때)
- **대기 큐** = `productionQueue`에서 front를 제외한 나머지
- `actualQty`/`totalDuration`을 큐 등록 시점 값으로 고정하는 이유: 이미 확정된 생산 계획이므로, 이후 해당
  시료의 수율/평균생산시간이 바뀌더라도 이미 큐에 들어간 작업의 계산 결과는 변하면 안 되기 때문이다.
- **저장하지 않고 매번 계산하는 값**: 진행률, 완료 여부, 예상 완료 시각 — 전부 조회 시점의 `now`를 대입해
  계산한다.

### 4.4 생산 담당자 — 저장 데이터 없음

승인/거절/출고는 전부 다른 모듈의 데이터를 읽고 그 모듈의 API로 갱신을 요청하는 오케스트레이션 로직만
가진다.

### 4.5 모니터링 — 저장 데이터 없음

상태별 주문 건수, 여유/부족/고갈 라벨 모두 조회 시점에 시료 담당자/주문 담당자 데이터를 읽어 계산하는
파생값이다.

## 5. 모듈 간 공개 API

```
// 시료 담당자
registerSample(id, name, avgTime, yield)
listSamples() -> [SampleView{id, name, avgTime, yield, stock}]
searchSamples(keyword) -> [SampleView]
getSample(sampleId) -> Sample
getStock(sampleId) -> int                 // 원재고
increaseStock(sampleId, qty)              // 생산 라인이 완료 시 호출
decreaseStock(sampleId, qty)              // 생산 담당자가 출고 시 호출

// 주문 담당자
createOrder(sampleId, customerName, qty) -> orderId   // 내부에서 시료담당자.getSample() 호출해 정보 표시
getOrder(orderId) -> Order
listOrdersByStatus(status) -> [Order]
setOrderStatus(orderId, status)           // 생산 담당자/생산 라인이 호출
getReservedQuantity(sampleId) -> int      // CONFIRMED+PRODUCING 주문 수량 합

// 생산 라인
enqueue(sampleId, shortfallQty, orderId)  // 생산 담당자가 호출
getCurrentJob(now) -> JobView
getQueue(now) -> [JobView]
// 조회 시점마다 내부적으로: 시료담당자.getSample()로 수율/평균시간 확인,
// 완료된 작업이 있으면 시료담당자.increaseStock() + 주문담당자.setOrderStatus(CONFIRMED) 직접 호출

// 생산 담당자
listPendingApprovals() -> RESERVED 목록
approve(orderId, now)   // 시료담당자.getStock() - 주문담당자.getReservedQuantity() = 가용 재고 계산
                        // 충분: 주문담당자.setOrderStatus(CONFIRMED)
                        // 부족: 생산라인.enqueue(...) + 주문담당자.setOrderStatus(PRODUCING)
reject(orderId)         // 주문담당자.setOrderStatus(REJECTED)
listShippable() -> CONFIRMED 목록
ship(orderId)           // 주문담당자.setOrderStatus(RELEASE) + 시료담당자.decreaseStock(qty)

// 모니터링
getOrderCountsByStatus() -> {RESERVED, CONFIRMED, PRODUCING, RELEASE 건수}  // REJECTED 제외
getStockOverview() -> [SampleView + 여유/부족/고갈 라벨]
```

## 6. 생산 완료 처리 흐름

1. 생산 담당자가 승인 처리 중 재고 부족을 판단하면 `생산라인.enqueue(sampleId, shortfallQty, orderId)` 호출.
2. 생산 라인의 `getCurrentJob`/`getQueue`가 호출될 때마다(= 누군가 생산 라인 관련 화면을 조회할 때마다),
   내부적으로 `now` 기준 완료된 작업이 있는지 계산한다.
3. 완료된 작업이 있으면 큐 순서(FIFO)대로 하나씩: `시료담당자.increaseStock(sampleId, actualQty)` +
   `주문담당자.setOrderStatus(orderId, CONFIRMED)`를 호출하고 큐에서 제거한다. 여러 개가 한 번에 밀려있으면
   순서대로 반복 처리한다 (catch-up).
4. 이 흐름에서 생산 담당자는 관여하지 않는다 — 생산 라인이 시료 담당자/주문 담당자를 직접 호출하며,
   의존 방향이 위(3장 계층)에서 아래로만 향하기 때문에 순환 의존이 생기지 않는다.

## 7. 영속성 정책

- 각 모듈(시료 담당자/주문 담당자/생산 라인)은 자기 데이터를 자기 파일 하나에 저장한다. 생산 담당자/
  모니터링은 저장할 데이터가 없다.
- 실행 시 파일에서 로드해 메모리에 유지하고, 종료 시 파일에 저장한다. 매 호출마다 파일을 다시 읽지 않는다.
- 여러 모듈의 데이터가 함께 필요한 계산(예: 가용 재고)은 데이터를 합쳐서 저장하지 않고, 필요한 모듈들의
  API를 호출한 뒤 호출한 쪽(생산 담당자)이 조합해서 계산한다.

> 출처: `PRD.md` 2.5절 (시간 및 영속성 공통 정책), `docs/memo.txt` 3~4번째 줄

## 8. 이번 설계에서 다루지 않은 것 (다음 단계)

- 각 모듈의 실제 C++ 클래스/헤더 파일 구조, 네임스페이스, 빌드 타겟 구성
- 파일 저장 포맷(텍스트/바이너리/JSON 등 구체적인 직렬화 방식)
- 에러 처리 방식(예외 vs 리턴 코드), 입력 검증 세부 규칙
- 테스트 전략(단위 테스트 대상, Harness 구체 설계)

이 항목들은 구현 계획(`writing-plans` 단계)에서 다룬다.
