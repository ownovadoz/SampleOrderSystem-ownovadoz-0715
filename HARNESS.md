# HARNESS.md — 서브에이전트 기반 개발 하네스

> **핵심 요약**: `docs/superpowers/plans/*.md`의 계획을 Task 단위로 서브에이전트에 위임해 개발한다. 각
> Task는 (1) 실패하는 기본 테스트만 커밋 → (2) 그 테스트를 통과시키는 구현 커밋, 이 두 단계로 진행하며,
> Task가 끝날 때마다 반드시 빌드가 성공해야 한다.

이 문서는 이 저장소에서 코드를 작성하는 모든 에이전트(오케스트레이터 + 서브에이전트)가 따라야 하는 개발
프로세스를 정의한다. 요구사항 자체(무엇을 만들지)는 `PRD.md`와 `docs/requirements/*.md`에 있고, 이 문서는
그것을 **어떻게** 만들지(프로세스)만 다룬다.

## 1. 서브에이전트 기반(subagent-driven) 개발

- 실제 구현 작업은 `docs/superpowers/plans/*.md`에 이미 Task 단위로 쪼개져 있다. 오케스트레이터는 계획을
  직접 구현하지 않고, Task 단위로 서브에이전트(Agent tool)에 위임한다.
- 한 Task를 위임할 때는 해당 Task의 `Files`/`Interfaces`/Step 목록 전체를 서브에이전트 프롬프트에 포함해
  자기완결적으로 만든다 — 서브에이전트는 계획 문서 전체나 이전 대화 맥락을 보지 못한다고 가정한다.
- 같은 모듈 내에서 여러 Task가 같은 파일(`*.vcxproj`, `*.vcxproj.filters`, `main.cpp` 등)을 순차적으로
  수정하므로, 한 모듈 안의 Task는 **순차적으로**(병렬 아님) 진행한다. 서로 다른 모듈(시료/주문/생산담당자/
  생산라인/모니터링)이 건드리는 파일이 겹치지 않는 구간에 한해서만 병렬 위임을 고려한다.
- 서브에이전트가 작업을 마치면, 오케스트레이터는 결과(빌드 로그, 테스트 출력, 실제 diff)를 직접 확인한
  뒤에만 다음 Task로 넘어간다. "서브에이전트가 됐다고 보고했다"는 것과 "실제로 됐다"는 것은 다르다.

## 2. Task 끝 = 항상 빌드 성공

각 Task의 마지막 Step은 항상 빌드 확인이어야 하며, 다음 조건을 만족해야 다음 Task로 진행한다.

- `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64` → `Build succeeded.`
- `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Release /p:Platform=x64` → `Build succeeded.`
- Debug 빌드 실행 파일이 gmock/gtest 테스트를 구동하며, 새로 추가한 테스트가 통과해야 한다
  (`[ PASSED ]`, 종료 코드 0).
- Release 빌드 실행 파일은 여전히 콘솔 메뉴 앱으로 정상 동작해야 한다 (테스트가 아니라 실제 메뉴가 뜸).
- 위 네 가지 중 하나라도 실패한 상태로 다음 Task로 넘어가지 않는다. Win32 구성까지 매 Task마다 확인할
  필요는 없지만, 최소 하나의 모듈이 끝나는 시점에는 Win32/x64 × Debug/Release 4개 구성 모두 빌드되는지
  확인한다.

## 3. TDD — Red 커밋과 Green 커밋을 분리

Task 안에서 테스트가 필요한 부분은 아래 2단계로 나누어 **각각 별도 커밋**을 남긴다.

1. **Red 커밋**: 아직 구현이 없어 컴파일이 실패하거나 assert가 실패하는, 그 기능의 핵심 동작만 검증하는
   "아주 잘 동작하는 기본 테스트"만 작성해 커밋한다. 모든 엣지 케이스를 한 번에 욱여넣지 않는다 — 우선
   해당 기능이 존재한다는 것과 정상 경로(happy path)만 검증하는 최소 테스트로 시작한다.
2. **Green 커밋**: 그 테스트를 통과시키는 최소 구현을 작성하고, 테스트가 실제로 통과하는 것을 빌드 로그로
   확인한 뒤 커밋한다.
3. 이후 같은 Task 또는 다음 Task에서 엣지 케이스(경계값, 실패 경로, 동시성 등) 테스트를 추가할 때도 같은
   패턴(실패하는 테스트 커밋 → 통과시키는 구현 커밋)을 반복한다.
4. Red 커밋 없이 구현과 테스트를 한 커밋에 함께 넣지 않는다 — 테스트가 실제로 그 구현 없이는 실패한다는
   것을 이력으로 남기는 것이 목적이다.

## 4. 테스트 커버리지 미달 시 사용자에게 반드시 알릴 것

- 계획 문서(`docs/superpowers/plans/*.md`) 또는 모듈 요구사항 문서(`docs/requirements/*.md`)에 명시된
  항목 중 테스트로 검증하지 못한 항목이 있으면(시간 부족, 환경 제약, 외부 의존성 등 이유 불문) **작업을
  끝냈다고 보고하기 전에** 어떤 항목을 테스트하지 못했는지 사용자에게 명시적으로 알린다.
- "일부만 테스트했지만 잘 될 것 같다"는 식으로 넘어가지 않는다. 커버하지 못한 항목과 그 이유를 목록으로
  제시하고, 사용자가 그대로 진행할지 추가 작업을 할지 판단하게 한다.

## 5. Naming Convention

C++ 표준/커뮤니티에서 널리 쓰이는 방식(Google C++ Style Guide 계열)을 따르며, 기존
`docs/superpowers/plans/`에 이미 작성된 예시 코드(`Common/Clock.h` 등)의 스타일을 그대로 유지한다.

- 타입(클래스/구조체/enum class): `PascalCase` — 예: `Order`, `Sample`, `OrderStatus`, `IClock`,
  `SystemClock`, `FakeClock`
- 인터페이스(순수 가상 클래스): 접두사 `I` + `PascalCase` — 예: `IClock`
- 함수/메서드: `camelCase` — 예: `now()`, `advance()`, `set()`
- 지역 변수/파라미터: `camelCase` — 예: `initial`, `avgProductionTime`
- 멤버 변수(private): `camelCase` + 트레일링 언더스코어 — 예: `current_`
- 네임스페이스: `lower_case` — 예: `common`
- 파일명: `PascalCase.h` / `PascalCase.cpp` (타입 이름과 1:1 대응), 테스트 파일은 `<대상>Tests.cpp`
  — 예: `Clock.h`, `Clock.cpp`, `ClockTests.cpp`
- 매크로/상수 외에는 헝가리안 표기법을 쓰지 않는다.

## 6. PoC 저장소 활용

각 모듈을 구현하는 서브에이전트는 새로 설계를 고민하기 전에 먼저 아래 대응되는 PoC 저장소의 구조를 참고해
동등한 패턴으로 재구현한다 (import가 아니라 구조를 참고해 다시 작성하는 것).

| 대상 | PoC 저장소 |
|---|---|
| MVC 계층 분리(Model/Controller/View) | `ConsoleMVC-ownovadoz-0715` |
| 데이터 영속성(저장/조회) | `DataPersistence-ownovadoz-0715` |
| 데이터 모니터링 도구 | `DataMonitor-ownovadoz-0715` |
| 더미 데이터 생성(테스트 전용) | `DummyDataGenerator-ownovadoz-0715` |

`PRD.md` 3장(아키텍처 공통 규칙)과 상충하지 않는지 항상 함께 확인한다.

## 7. 이 문서와 다른 문서의 관계

- **무엇을 만들지**(도메인 규칙, 모듈별 요구사항): `PRD.md`, `docs/requirements/*.md`
- **어떤 순서로 구현할지**(Task 목록, 파일별 Step): `docs/superpowers/plans/*.md`
- **어떻게 개발할지**(이 문서, 프로세스/컨벤션): `HARNESS.md`

세 문서가 상충하는 것처럼 보이면, 요구사항 원본(`docs/requirements.pdf`, `docs/memo.txt`)을 기준으로
판단하고 그 결정을 계획 문서에 반영한다.
