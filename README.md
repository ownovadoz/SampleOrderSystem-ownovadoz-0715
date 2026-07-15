# SampleOrderSystem-ownovadoz-0715

반도체 시료 생산주문관리 시스템 — 콘솔 기반 C++20 애플리케이션. 시료 등록/조회/검색, 주문 접수/승인/거절,
생산 라인 운용, 출고 처리, 모니터링(주문량/재고량 확인)을 하나의 메뉴 트리에서 처리한다.

## 문서 안내

- **`PRD.md`**: 모든 모듈 공통 요구사항(도메인 모델, 주문 상태 전이, 재고 선점 규칙, 아키텍처 공통 규칙).
- **`docs/requirements/*.md`**: 모듈별(시료 관리/주문 접수·승인·거절/생산 라인/모니터링/출고 처리) 상세 요구사항.
- **`CLAUDE.md`**: 이 저장소에서 에이전트가 작업할 때 참고하는 안내 문서.
- **`HARNESS.md`**: 서브에이전트 기반 개발 프로세스(Task 단위 위임, TDD Red/Green 커밋, 네이밍 컨벤션 등).

## 빌드

Visual Studio 2022+ (PlatformToolset v145), C++20 필요. gmock/gtest는 NuGet(`packages.config`)으로 이미
포함돼 있다.

```
msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64
msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Release /p:Platform=x64
```

또는 `SampleOrderSystem-ownovadoz-0715.slnx`를 Visual Studio로 열어 빌드해도 된다. Win32/x64 × Debug/Release
4개 구성을 모두 지원한다.

같은 실행 파일이 빌드 구성에 따라 다르게 동작한다:

- **Debug**: `main()`이 gmock/gtest의 `RUN_ALL_TESTS()`를 실행한다 (별도 Tests 프로젝트 없이 앱 프로젝트
  하나에 테스트가 함께 들어 있다).
- **Release**: 실제 콘솔 메뉴 앱으로 동작한다.

## 실행

Release 빌드 실행 파일을 그대로 실행하면 메인 메뉴가 뜬다.

```
x64\Release\SampleOrderSystem-ownovadoz-0715.exe
```

메인 메뉴는 2단계 구조다 — 대분류를 고르면 그 모듈의 세부 메뉴로 들어간다.

```
[1] 시료 관리              시료 등록 / 조회 / 검색
[2] 주문(접수/승인/거절)    시료 예약(주문 접수) / 주문 승인 / 주문 거절
[3] 모니터링                주문량 확인 / 재고량 확인 / 재고 검색
[4] 출고 처리               출고 처리
[5] 생산 라인               생산 현황 / 대기 주문 확인
[0] 종료
```

여러 필드를 입력하는 화면(시료 등록, 시료 예약 등)은 첫 프롬프트에서 `q`를 입력하면 그 화면 전체를
취소하고 이전 메뉴로 돌아간다. 숫자를 기대하는 곳에 잘못된 값을 넣어도 프로그램이 종료되지 않고 오류
안내만 보여준다.

데이터는 실행 파일과 같은 작업 디렉터리의 `samples.json`/`orders.json`/`production_queue.json`에
저장된다(JSON, `Common/FileRepository.h` 기반). 실행 시 불러오고 종료 시 저장한다.

## 더미 데이터 생성 (수동 테스트용)

담당자가 화면을 눌러가며 여러 데이터를 미리 채워보고 싶을 때, 일반 메뉴가 아니라 별도의 명령행 인자로
실행한다 (PRD상 실제 운영 메뉴에는 더미 데이터 생성 기능이 노출되면 안 되기 때문).

```
SampleOrderSystem-ownovadoz-0715.exe --seed <시료 개수> [주문 개수]
```

- 시료 개수만 넘기면 주문 개수는 `시료 개수 × 3`으로 자동 계산된다.
- 새로 만드는 주문은 항상 이번에 생성했거나 기존에 있던 시료 id만 참조한다.
- `PRODUCING` 상태로 뽑힌 주문에는 생산 큐(`production_queue.json`) 항목도 함께 생성해, 생산 라인 화면에서
  바로 확인할 수 있다.
- 기존 데이터를 지우지 않고 이어서 누적한다 — 여러 번 실행해도 안전하다.

```
SampleOrderSystem-ownovadoz-0715.exe --seed 6 15
```

## 테스트

Debug 빌드 실행 파일이 곧 테스트 실행 파일이다.

```
x64\Debug\SampleOrderSystem-ownovadoz-0715.exe
```

특정 테스트만 돌리려면 gtest 필터를 쓴다.

```
x64\Debug\SampleOrderSystem-ownovadoz-0715.exe --gtest_filter=SampleClerkTest.*
```

## 프로젝트 구조

기능별로 폴더를 나누고, 각 폴더 안에 Model/Controller/View와 그 테스트 파일을 함께 둔다.

```
Common/           공유 도메인 타입(Sample/Order/OrderStatus), Clock, JSON 직렬화, 콘솔 입력 유틸리티
SampleClerk/      시료 담당자 (레이어 0)
OrderClerk/       주문 담당자 (레이어 1)
ProductionLine/   생산 라인 (레이어 2)
ProductionClerk/  생산 담당자 — 승인/거절/출고, 자체 저장 데이터 없음 (레이어 3)
Monitoring/       모니터링 — 읽기 전용 (레이어 4)
Testing/          DummyDataGenerator(결정적 더미 데이터 생성기), DummySeeder(시료/주문/생산 큐 일괄 생성)
```

의존 방향은 위 목록 순서(레이어 0 → 4) 한 방향으로만 흐른다. 자세한 모듈 경계와 공개 API는
`docs/superpowers/specs/2026-07-15-module-architecture-design.md`를 참고.

## 참고 PoC 저장소

MVC 계층 분리, JSON 영속성, 모니터링 검색, 더미 데이터 생성 방식은 아래 4개 PoC 저장소의 구조를 참고해
다시 구현했다 (라이브러리로 import한 것이 아니다).

| 항목 | 저장소 |
|---|---|
| MVC 스켈레톤 | `ConsoleMVC-ownovadoz-0715` |
| 데이터 영속성(JSON) | `DataPersistence-ownovadoz-0715` |
| 데이터 모니터링 | `DataMonitor-ownovadoz-0715` |
| 더미 데이터 생성 | `DummyDataGenerator-ownovadoz-0715` |
