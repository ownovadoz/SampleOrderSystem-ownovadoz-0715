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

## 8. 모듈 내부 계층 (MVC)

5개 모듈은 서로에게는 4~5장에서 정의한 공개 API만 노출하지만, **각 모듈 내부는 Controller/Model로 나뉜다**
(PRD 3장의 MVC 규칙을 모듈 하나하나에 적용한 것).

- **Model**: 그 모듈이 소유한 데이터의 실제 파일 읽기/쓰기 + 인메모리 상태 유지를 전담한다. 예: 시료
  담당자의 `SampleModel`이 `samples.dat`를 읽고 쓰며 `map<string, Sample>`을 들고 있는다.
- **Controller**: 검증/승인 판단/상태 전이 같은 업무 로직을 담당하고, 자기 모듈의 Model만 호출한다. 다른
  모듈의 데이터가 필요하면 반드시 그 모듈의 공개 API(Controller가 노출한 함수)를 호출하고, 그 모듈의 Model이나
  파일에 직접 접근하지 않는다.

즉 "DB를 직접 만지지 않는다"는 원칙은 두 겹으로 적용된다: ① 모듈 내부에서는 Controller가 Model을 거치지
않고 파일을 직접 열지 않는다, ② 모듈 외부에서는 다른 모듈이 그 모듈의 Model이나 파일을 직접 건드리지
않고 반드시 Controller의 공개 API를 거친다. 이렇게 해도 3장의 소유권/의존 그래프는 그대로 유지된다 —
새로운 6번째 모듈이 생기는 게 아니라, 기존 5개 모듈 각각의 내부 구조를 명확히 한 것뿐이다.

파일 읽기/쓰기의 실제 구현(직렬화 방식 등)은 각 모듈의 Model이 개별적으로 가지되,
`DataPersistence-ownovadoz-0715` PoC에서 검증한 방식을 참고해 동일하게 구현한다 (PRD 3장 참고). 이 PoC를
공유 라이브러리로 import하는 것은 아니고, 같은 패턴을 각자 다시 구현하는 것이다 (PRD 3장 원칙과 동일).

## 9. View 계층

**View도 모듈별로 소유한다.** 단일 공용 View 클래스를 두지 않는 이유는 8장에서 Model을 모듈별로 나눈
이유와 같다 — View 하나가 5개 모듈의 화면을 전부 갖고 있으면, 5개 에이전트가 결국 같은 파일 하나를
계속 같이 건드리게 되어 독립 개발이 무너진다.

- 각 모듈은 자기 메뉴에 대응하는 View를 갖는다 (예: 시료 담당자 → `SampleView`, 생산 라인 →
  `ProductionLineView`).
- **View는 자기 모듈의 Controller만 호출한다.** 다른 모듈의 Controller를 View가 직접 호출하지 않는다.
  다른 모듈 데이터가 필요하면 자기 Controller가 그 모듈의 공개 API(5장)를 호출해서 받아오고, View는
  Controller가 돌려준 결과를 화면에 그리기만 한다.
- 화면 서식(표 틀, 구분선, 입력 프롬프트 등)을 그리는 공통 포맷팅 함수는 업무 데이터를 갖지 않는 순수
  유틸리티이므로 공유해도 된다 (8장의 영속성 유틸리티와 같은 성격 — 데이터를 소유하지 않는 공유 도구는
  의존 그래프에 새 노드를 추가하지 않는다).
- **메인 메뉴(최상위 1~5번 선택 화면)는 어느 모듈도 소유하지 않는 얇은 진입점**이다. 번호를 받아 해당
  모듈의 화면 진입 함수를 호출하는 것 외에 업무 로직이 없으므로, 여러 에이전트가 공유해도 충돌 위험이
  작다.

## 10. 화면별 상세 명세

메뉴별로 입력값, 출력 필드, 문구를 아래처럼 못박아 구현 시 애매함이 없게 한다. (표기된 예시 문구는
참고용이며 정확한 워딩은 자유롭게 조정 가능 — 단, **표시 항목/판단 기준 자체는 고정**한다.)

### 10.1 시료 등록 (시료 담당자)

- 입력: 시료 ID, 이름, 평균 생산시간, 수율
- 검증 실패 시 사유와 함께 재입력 요구: ID 중복, 수율이 0 이하이거나 1 초과
- 성공 시 출력: "등록 완료" + 등록된 시료 정보 재출력

### 10.2 시료 조회 (시료 담당자)

- 출력 컬럼: 시료 ID, 이름, 평균 생산시간, 수율, 현재 재고(원재고)
- 등록된 시료 전체를 한 번에 출력한다 (페이지네이션 없음)

### 10.3 시료 검색 (시료 담당자)

- 입력: 검색어(이름 부분 문자열, 대소문자 무시)
- 출력: 10.2와 동일한 컬럼의 검색 결과 목록. 결과가 없으면 "검색 결과 없음" 표시

### 10.4 시료 예약(주문 접수) (주문 담당자)

- 입력: 시료 ID, 고객명, 주문 수량(1 이상 정수)
- 시료 ID가 존재하지 않으면 접수를 거부하고 사유 표시 (시료담당자.getSample 조회 결과 없음)
- 주문번호 채번 규칙: `ORD-{YYYYMMDD}-{4자리 일련번호}` — 날짜는 주문 생성 시점(모킹 가능한 시간 소스)
  기준이며, 일련번호는 그날 생성된 주문 순서대로 0001부터 시작해 하루 단위로 초기화된다.
  (`docs/requirements.pdf` p.15 예시 UI의 `ORD-20260416-0043` 형식 참고 — 정확한 채번 규칙 자체는 명세에
  없어 이번에 정한 것)
- 성공 시 출력: 주문번호, 시료명, 고객명, 수량, 상태(RESERVED)

### 10.5 접수된 주문 목록 / 승인·거절 (생산 담당자)

- 목록 컬럼: 번호(선택용 인덱스), 주문번호, 고객명, 시료명, 수량, 상태(RESERVED) — 접수된 순서대로 정렬
- 번호 선택 후 승인/거절 선택
- 승인 시: 가용 재고 계산 결과를 먼저 보여주고 최종 확인을 받는다
  - 재고 충분: "재고 충분 → 즉시 출고 대기(CONFIRMED)로 전환됩니다" 표시 후 확인
  - 재고 부족: 부족분, 실 생산량(`ceil(부족분/수율)`), 총 예상 소요 시간을 함께 보여주고
    ("부족분 N ea, 실생산량 M ea, 예상 소요 T분") 확인을 받은 뒤 PRODUCING으로 전환 + 생산 큐 등록
- 거절 시: 확인 없이 즉시 REJECTED로 전환 (되돌릴 수 없음을 문구로 안내)
- 처리 후 공통 출력: 주문번호, 상태 변경 결과(RESERVED → 무엇으로 바뀌었는지)

### 10.6 출고 처리 (생산 담당자)

- 목록 컬럼: 번호, 주문번호, 고객명, 시료명, 수량 — `CONFIRMED` 상태만
- 번호 선택 후 출고 실행, 확인 절차 없이 즉시 처리 (되돌릴 수 없는 작업이 아니라 이미 확정된 주문의
  마무리 단계이므로)
- 처리 후 출력: 주문번호, 출고 수량, 처리 일시(현재 시각), 상태 변경(CONFIRMED → RELEASE)

### 10.7 모니터링 — 주문량 확인

- 출력: 상태별(RESERVED/CONFIRMED/PRODUCING/RELEASE) 건수 표. REJECTED는 표시하지 않는다.

### 10.8 모니터링 — 재고량 확인

- 출력 컬럼: 시료명, 재고(원재고), 상태 라벨
- 상태 라벨 판정 기준(우선순위 순서대로 평가):
  1. 재고 == 0 → **고갈**
  2. 재고 < 미출고 수요 → **부족** (미출고 수요 = 해당 시료에 대해 `RESERVED`+`CONFIRMED`+`PRODUCING`
     상태인 모든 주문의 수량 합 — 아직 출고되지 않은, 유효한 모든 수요)
  3. 그 외 → **여유**
  (`docs/requirements.pdf` p.18에는 "여유/부족/고갈" 라벨만 정의되어 있고 구체적 판정 기준은 없어서,
  이번에 정한 규칙)

### 10.9 생산 현황 (생산 라인)

- 생산 라인이 비어 있을 때: "대기 중인 생산 없음" 표시
- 진행 중일 때 출력 필드: 주문번호, 시료명, 주문량, 부족분, 실 생산량, 진행률(%), 예상 완료 시각
  (진행률/예상 완료 시각은 조회 시점 `now` 기준으로 계산해 표시 — 4.3절 참고)

### 10.10 대기 주문 확인 (생산 라인)

- 출력 컬럼: 순서(큐 내 위치, 1부터), 주문번호, 시료명, 주문량, 부족분, 실 생산량, 예상 완료 시각
- 예상 완료 시각은 앞선 대기 작업들의 남은 소요 시간을 누적해서 계산한다 (현재 작업의 남은 시간 +
  앞 순번 작업들의 총 소요 시간 합)

## 11. 이번 설계에서 다루지 않은 것 (다음 단계)

- 각 모듈의 실제 C++ 클래스/헤더 파일 구조, 네임스페이스, 빌드 타겟 구성
- 파일 저장 포맷(텍스트/바이너리/JSON 등 구체적인 직렬화 방식)
- 에러 처리 방식(예외 vs 리턴 코드), 입력 검증 세부 규칙(10장에 정리한 것 이상의 세부 규칙)
- 테스트 전략(단위 테스트 대상, Harness 구체 설계)

이 항목들은 구현 계획(`writing-plans` 단계)에서 다룬다.
