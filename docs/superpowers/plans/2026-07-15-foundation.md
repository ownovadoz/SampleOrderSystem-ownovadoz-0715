# Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 5개 모듈(시료담당자/주문담당자/생산담당자/생산라인/모니터링)이 공통으로 쓸 도메인 타입, 모킹 가능한
시간 소스, 그리고 gmock/gtest 기반 테스트 실행 경로를 만들어, 이후 모듈별 계획들이 바로 TDD로 진행할 수
있는 토대를 만든다.

**Architecture:** 기존 `SampleOrderSystem-ownovadoz-0715` 콘솔 앱 프로젝트(단일 프로젝트, 별도 Tests
프로젝트 없음)에 `Common/` 폴더로 공유 도메인 헤더(Sample/Order/OrderStatus)와 시간 추상화(Clock)를
추가한다. 테스트 프레임워크는 gmock 1.11.0(NuGet, `packages.config`로 이미 추가됨)을 사용하며, **같은
실행 파일**이 빌드 구성에 따라 다르게 동작한다: `main()`이 `_DEBUG`(Debug 구성)일 때는
`RUN_ALL_TESTS()`를 실행하고, `NDEBUG`(Release 구성)일 때는 실제 콘솔 메뉴 앱을 실행한다.

**Tech Stack:** C++20, MSVC(PlatformToolset v145), Win32/x64 콘솔 애플리케이션. gmock/gtest(NuGet
`gmock.1.11.0`, 이미 저장소에 커밋됨 — `packages/gmock.1.11.0/`, `packages.config`,
`gmock.targets` import).

## Global Constraints

- C++20 표준, `LanguageStandard=stdcpp20` (기존 vcxproj 설정과 동일하게 유지)
- PlatformToolset v145, Win32/x64 × Debug/Release 4개 구성 모두 빌드 가능해야 함
- 테스트는 gmock/gtest 매크로(`TEST`, `EXPECT_EQ`, `ASSERT_TRUE` 등)로 작성한다. 별도 Tests 프로젝트를
  만들지 않는다 — 테스트 파일도 앱 프로젝트에 그대로 추가한다.
- **Debug 빌드 = 테스트 실행, Release 빌드 = 실제 앱 실행.** `main.cpp`가 `#ifdef _DEBUG` /
  `#else`(=Release, `NDEBUG`)로 분기한다 (기존 vcxproj의 `PreprocessorDefinitions`에 이미
  `_DEBUG`/`NDEBUG`가 구성별로 정의되어 있음 — Task 1 Step 6 참고).
- 소스에 한글 문자열/주석이 들어가므로 모든 `ClCompile`에 `/utf-8` 컴파일 옵션을 추가해 인코딩 문제를 방지한다
- 각 모듈이 서로 다른 에이전트에 의해 독립적으로 개발되므로, 공유 헤더(`Common/*`)는 데이터 타입 정의만
  담고 로직을 넣지 않는다 (`docs/superpowers/specs/2026-07-15-module-architecture-design.md` 8장 참고)

---

### Task 1: 공통 도메인 헤더 + 최소 빌드 가능한 main.cpp

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Time.h`
- Create: `SampleOrderSystem-ownovadoz-0715/Common/OrderStatus.h`
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Sample.h`
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Order.h`
- Create: `SampleOrderSystem-ownovadoz-0715/main.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters`

**Interfaces:**
- Produces: `common::Duration`, `common::TimePoint` (`Common/Time.h`); `common::OrderStatus` enum class with
  values `RESERVED, REJECTED, PRODUCING, CONFIRMED, RELEASE` (`Common/OrderStatus.h`); `common::Sample` struct
  with fields `id, name, avgProductionTime, yield, stock` (`Common/Sample.h`); `common::Order` struct with
  fields `id, sampleId, customerName, quantity, status` (`Common/Order.h`). Task 2와 이후 모든 모듈 계획이
  이 타입들을 그대로 사용한다.

- [ ] **Step 1: `Common/Time.h` 작성**

```cpp
#pragma once
#include <chrono>

namespace common {

using Duration = std::chrono::seconds;
using TimePoint = std::chrono::system_clock::time_point;

} // namespace common
```

- [ ] **Step 2: `Common/OrderStatus.h` 작성**

```cpp
#pragma once

namespace common {

enum class OrderStatus {
    RESERVED,
    REJECTED,
    PRODUCING,
    CONFIRMED,
    RELEASE
};

} // namespace common
```

- [ ] **Step 3: `Common/Sample.h` 작성**

```cpp
#pragma once
#include <string>
#include "Time.h"

namespace common {

struct Sample {
    std::string id;
    std::string name;
    Duration avgProductionTime;
    double yield;   // 0 초과 1 이하
    int stock;      // 원재고 수량
};

} // namespace common
```

- [ ] **Step 4: `Common/Order.h` 작성**

```cpp
#pragma once
#include <string>
#include "OrderStatus.h"

namespace common {

struct Order {
    std::string id;
    std::string sampleId;
    std::string customerName;
    int quantity;
    OrderStatus status;
};

} // namespace common
```

- [ ] **Step 5: `main.cpp` 작성 (Debug=테스트 실행 / Release=실제 앱, 최소 버전)**

```cpp
#include <iostream>

#ifdef _DEBUG
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
#else

int main() {
    std::cout << "반도체 시료 생산주문관리 시스템\n";
    return 0;
}
#endif
```

- [ ] **Step 6: 앱 vcxproj에 파일 등록**

`SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj`에는 이미 gmock NuGet 패키지가
추가되어 있어 `<ItemGroup><None Include="packages.config" /></ItemGroup>` 바로 다음, `<Import
Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />` 바로 앞에 아래 두 `ItemGroup`을 추가한다.

```xml
  <ItemGroup>
    <ClCompile Include="main.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Common\Time.h" />
    <ClInclude Include="Common\OrderStatus.h" />
    <ClInclude Include="Common\Sample.h" />
    <ClInclude Include="Common\Order.h" />
  </ItemGroup>
```

또한 같은 파일의 4개 `ItemDefinitionGroup`(Debug|Win32, Release|Win32, Debug|x64, Release|x64) 각각의
`<ClCompile>` 블록에 아래 한 줄을 추가한다 (한글 문자열 인코딩 문제 방지):

```xml
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
```

- [ ] **Step 7: 앱 vcxproj.filters에 파일 등록**

`SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters`의 `</Project>` 태그
바로 앞에 아래 `ItemGroup`을 추가한다.

```xml
  <ItemGroup>
    <ClCompile Include="main.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Common\Time.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="Common\OrderStatus.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="Common\Sample.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="Common\Order.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
  </ItemGroup>
```

- [ ] **Step 8: 빌드 확인 (Debug=테스트, Release=실제 앱)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.` (오류 0개, gmock/gtest 소스까지 함께 컴파일됨)

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.exe`
Expected: 아직 등록된 테스트가 없으므로 gtest가 `[==========] 0 tests from 0 test suites ran.` 류의 출력과
함께 종료 코드 0으로 끝남

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Release /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Release\SampleOrderSystem-ownovadoz-0715.exe`
Expected: 콘솔에 `반도체 시료 생산주문관리 시스템`이 정상 출력 (한글이 깨지지 않아야 함)

- [ ] **Step 9: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/Common SampleOrderSystem-ownovadoz-0715/main.cpp \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters
git commit -m "공통 도메인 헤더(Sample/Order/OrderStatus) 및 최소 진입점 추가"
```

---

### Task 2: 모킹 가능한 Clock 추상화 (gmock 기반 TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Clock.h`
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Clock.cpp`
- Create: `SampleOrderSystem-ownovadoz-0715/Common/ClockTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+.filters)

**Interfaces:**
- Consumes: `common::Duration`, `common::TimePoint` (Task 1)
- Produces: `common::IClock`(인터페이스, `now() -> TimePoint`), `common::SystemClock`(실시간),
  `common::FakeClock`(테스트용, `set(TimePoint)`/`advance(Duration)`). 이후 생산 라인 등 "지금 시각이
  필요한" 모든 모듈이 `IClock&`를 주입받아 사용한다.

- [ ] **Step 1: 실패하는 테스트 작성 — `Common/ClockTests.cpp`** (아직 `Clock.h`가 없으므로 컴파일 실패 예상)

```cpp
#include "Clock.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace common;

TEST(FakeClockTest, ReturnsSetTime) {
    TimePoint t0 = std::chrono::system_clock::from_time_t(1000);
    FakeClock clock(t0);
    EXPECT_EQ(clock.now(), t0);
}

TEST(FakeClockTest, AdvanceMovesTimeForward) {
    TimePoint t0 = std::chrono::system_clock::from_time_t(1000);
    FakeClock clock(t0);
    clock.advance(std::chrono::seconds(90));
    TimePoint expected = std::chrono::system_clock::from_time_t(1090);
    EXPECT_EQ(clock.now(), expected);
}

TEST(SystemClockTest, ReturnsRealTime) {
    SystemClock clock;
    auto before = std::chrono::system_clock::now();
    auto result = clock.now();
    auto after = std::chrono::system_clock::now();
    EXPECT_GE(result, before);
    EXPECT_LE(result, after);
}
```

- [ ] **Step 2: vcxproj/필터에 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.vcxproj`의 `ClCompile` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClCompile Include="Common\Clock.cpp" />
    <ClCompile Include="Common\ClockTests.cpp" />
```

`ClInclude` `<ItemGroup>` 닫는 태그 앞에:

```xml
    <ClInclude Include="Common\Clock.h" />
```

`.vcxproj.filters`에도 위 파일들을 각각 "소스 파일"/"헤더 파일" 필터로 추가한다.

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Clock.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 3: `Clock.h` 구현**

```cpp
#pragma once
#include "Time.h"

namespace common {

class IClock {
public:
    virtual ~IClock() = default;
    virtual TimePoint now() const = 0;
};

class SystemClock : public IClock {
public:
    TimePoint now() const override;
};

class FakeClock : public IClock {
public:
    explicit FakeClock(TimePoint initial);
    TimePoint now() const override;
    void set(TimePoint t);
    void advance(Duration d);

private:
    TimePoint current_;
};

} // namespace common
```

- [ ] **Step 4: `Clock.cpp` 구현**

```cpp
#include "Clock.h"

namespace common {

TimePoint SystemClock::now() const {
    return std::chrono::system_clock::now();
}

FakeClock::FakeClock(TimePoint initial) : current_(initial) {}

TimePoint FakeClock::now() const {
    return current_;
}

void FakeClock::set(TimePoint t) {
    current_ = t;
}

void FakeClock::advance(Duration d) {
    current_ += d;
}

} // namespace common
```

- [ ] **Step 5: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.exe`
Expected: `[==========] 3 tests from 2 test suites ran.` / `[  PASSED  ] 3 tests.` (종료 코드 0)

- [ ] **Step 6: Release 빌드가 여전히 실제 앱으로 동작하는지 확인**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Release /p:Platform=x64`
Run: `x64\Release\SampleOrderSystem-ownovadoz-0715.exe`
Expected: 테스트가 아니라 `반도체 시료 생산주문관리 시스템` 배너가 출력됨 (Release는 `NDEBUG`라서
`main.cpp`의 `#else` 분기를 타기 때문)

- [ ] **Step 7: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/Common/Clock.h SampleOrderSystem-ownovadoz-0715/Common/Clock.cpp \
        SampleOrderSystem-ownovadoz-0715/Common/ClockTests.cpp \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters
git commit -m "모킹 가능한 Clock 추상화와 gmock 테스트 추가"
```

---

## 완료 후 상태

- 앱 프로젝트가 Debug/Release 모두 빌드되며, Debug는 gmock 테스트 실행, Release는 실제 콘솔 앱 실행으로
  동작 분기됨
- 5개 모듈이 공유할 `common::Sample`/`common::Order`/`common::OrderStatus` 타입과
  `common::IClock`/`SystemClock`/`FakeClock`이 준비됨
- 이후 모든 모듈은 자기 테스트 파일(`*Tests.cpp`)을 같은 앱 프로젝트에 추가하고 `TEST`/`EXPECT_*` 매크로로
  작성하면 됨 (별도 Tests 프로젝트 없음)
- 다음 단계: 시료 담당자 모듈(레이어 0)부터 계획 수립 — `docs/superpowers/specs/2026-07-15-module-architecture-design.md`
  4.1절/5장/10.1~10.3절 참고
