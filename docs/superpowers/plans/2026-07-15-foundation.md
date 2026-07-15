# Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 5개 모듈(시료담당자/주문담당자/생산담당자/생산라인/모니터링)이 공통으로 쓸 도메인 타입, 모킹 가능한
시간 소스, 그리고 별도 외부 라이브러리 없이 동작하는 테스트 하네스를 만들어, 이후 모듈별 계획들이 바로
TDD로 진행할 수 있는 토대를 만든다.

**Architecture:** 기존 `SampleOrderSystem-ownovadoz-0715` 콘솔 앱 프로젝트에 `Common/` 폴더로 공유 도메인
헤더(Sample/Order/OrderStatus)와 시간 추상화(Clock)를 추가한다. 테스트는 별도 실행 파일 프로젝트
(`SampleOrderSystem-ownovadoz-0715.Tests`)를 새로 만들어 진행하며, 이 프로젝트는 앱 프로젝트의 `Common/*`
소스 파일을 상대 경로로 그대로 컴파일에 포함시킨다(정적 라이브러리 분리 없이 가장 단순한 구조).

**Tech Stack:** C++20, MSVC(PlatformToolset v145), Win32/x64 콘솔 애플리케이션. 외부 테스트 프레임워크 없이
헤더 하나짜리 자체 제작 테스트 하네스(`MicroTest.h`) 사용.

## Global Constraints

- C++20 표준, `LanguageStandard=stdcpp20` (기존 vcxproj 설정과 동일하게 유지)
- PlatformToolset v145, Win32/x64 × Debug/Release 4개 구성 모두 빌드 가능해야 함
- 외부 패키지 매니저/라이브러리 의존성 추가 금지 (테스트 하네스도 자체 제작)
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

- [ ] **Step 5: `main.cpp` 작성 (최소 진입점)**

```cpp
#include <iostream>

int main() {
    std::cout << "반도체 시료 생산주문관리 시스템\n";
    return 0;
}
```

- [ ] **Step 6: 앱 vcxproj에 파일 등록**

`SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj`에서 빈 `<ItemGroup></ItemGroup>`
(파일 끝쪽, `<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />` 바로 앞)을 아래로 교체한다.

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

- [ ] **Step 8: 빌드 확인**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.` (오류 0개)

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.exe`
Expected: 콘솔에 `반도체 시료 생산주문관리 시스템`이 정상 출력 (한글이 깨지지 않아야 함)

- [ ] **Step 9: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/Common SampleOrderSystem-ownovadoz-0715/main.cpp \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters
git commit -m "공통 도메인 헤더(Sample/Order/OrderStatus) 및 최소 진입점 추가"
```

---

### Task 2: 모킹 가능한 Clock 추상화 + 테스트 하네스 + Tests 프로젝트 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Clock.h`
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Clock.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters`
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/Testing/MicroTest.h`
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/TestMain.cpp`
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/ClockTests.cpp`
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`
- Modify: `SampleOrderSystem-ownovadoz-0715.slnx`

**Interfaces:**
- Consumes: `common::Duration`, `common::TimePoint` from `Common/Time.h` (Task 1)
- Produces: `common::IClock` (인터페이스, `now() -> TimePoint`), `common::SystemClock` (실시간),
  `common::FakeClock` (테스트용, `set(TimePoint)`/`advance(Duration)`로 시간 조작 가능). 이후 생산 라인 등
  "지금 시각이 필요한" 모든 모듈이 `IClock&`를 주입받아 사용한다.
- Produces: `TEST_CASE(name) { ... }` / `REQUIRE(expr)` 매크로 (`Testing/MicroTest.h`) — 이후 모든 모듈의
  테스트가 이 매크로를 사용한다.

- [ ] **Step 1: 테스트 하네스 작성 — `Testing/MicroTest.h`**

```cpp
#pragma once
#include <functional>
#include <string>
#include <vector>
#include <iostream>

namespace microtest {

struct Failure {
    std::string testName;
    std::string message;
    int line;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void add(const std::string& name, std::function<void()> fn) {
        tests_.push_back({name, fn});
    }

    void recordFailure(const std::string& message, int line) {
        failures_.push_back({currentTest_, message, line});
    }

    int runAll() {
        int passed = 0;
        for (auto& t : tests_) {
            currentTest_ = t.name;
            size_t failuresBefore = failures_.size();
            t.fn();
            if (failures_.size() == failuresBefore) {
                std::cout << "[PASS] " << t.name << "\n";
                ++passed;
            } else {
                std::cout << "[FAIL] " << t.name << "\n";
            }
        }
        std::cout << passed << "/" << tests_.size() << " tests passed\n";
        for (auto& f : failures_) {
            std::cout << "  FAILURE in " << f.testName << " (line " << f.line << "): " << f.message << "\n";
        }
        return failures_.empty() ? 0 : 1;
    }

private:
    struct TestCase { std::string name; std::function<void()> fn; };
    std::vector<TestCase> tests_;
    std::vector<Failure> failures_;
    std::string currentTest_;
};

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        TestRegistry::instance().add(name, fn);
    }
};

} // namespace microtest

#define TEST_CASE(name) \
    void name(); \
    static microtest::Registrar registrar_##name(#name, name); \
    void name()

#define REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            microtest::TestRegistry::instance().recordFailure(#expr, __LINE__); \
        } \
    } while (0)
```

- [ ] **Step 2: 테스트 실행 진입점 — `TestMain.cpp`**

```cpp
#include "Testing/MicroTest.h"

int main() {
    return microtest::TestRegistry::instance().runAll();
}
```

- [ ] **Step 3: 실패하는 테스트 작성 — `ClockTests.cpp`** (아직 `Clock.h`가 없으므로 컴파일 실패 예상)

```cpp
#include "../SampleOrderSystem-ownovadoz-0715/Common/Clock.h"
#include "Testing/MicroTest.h"
#include <chrono>

using namespace common;

TEST_CASE(FakeClock_ReturnsSetTime) {
    TimePoint t0 = std::chrono::system_clock::from_time_t(1000);
    FakeClock clock(t0);
    REQUIRE(clock.now() == t0);
}

TEST_CASE(FakeClock_AdvanceMovesTimeForward) {
    TimePoint t0 = std::chrono::system_clock::from_time_t(1000);
    FakeClock clock(t0);
    clock.advance(std::chrono::seconds(90));
    TimePoint expected = std::chrono::system_clock::from_time_t(1090);
    REQUIRE(clock.now() == expected);
}

TEST_CASE(SystemClock_ReturnsRealTime) {
    SystemClock clock;
    auto before = std::chrono::system_clock::now();
    auto result = clock.now();
    auto after = std::chrono::system_clock::now();
    REQUIRE(result >= before);
    REQUIRE(result <= after);
}
```

- [ ] **Step 4: Tests 프로젝트 생성 — `SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|Win32">
      <Configuration>Debug</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|Win32">
      <Configuration>Release</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>18.0</VCProjectVersion>
    <Keyword>Win32Proj</Keyword>
    <ProjectGuid>{a3d4e5f6-1b2c-4d3e-8f9a-0b1c2d3e4f5a}</ProjectGuid>
    <RootNamespace>SampleOrderSystemTests</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="TestMain.cpp" />
    <ClCompile Include="ClockTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Testing\MicroTest.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Time.h" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
```

- [ ] **Step 5: Tests 프로젝트 필터 파일 — `SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
    <Filter Include="소스 파일">
      <UniqueIdentifier>{b1c2d3e4-5f6a-4b7c-8d9e-0f1a2b3c4d5e}</UniqueIdentifier>
      <Extensions>cpp;c;cc;cxx;c++;cppm;ixx;def;odl;idl;hpj;bat;asm;asmx</Extensions>
    </Filter>
    <Filter Include="헤더 파일">
      <UniqueIdentifier>{c2d3e4f5-6a7b-4c8d-9e0f-1a2b3c4d5e6f}</UniqueIdentifier>
      <Extensions>h;hh;hpp;hxx;h++;hm;inl;inc;ipp;xsd</Extensions>
    </Filter>
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="TestMain.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="ClockTests.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Testing\MicroTest.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Time.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
  </ItemGroup>
</Project>
```

- [ ] **Step 6: 솔루션에 Tests 프로젝트 추가**

`SampleOrderSystem-ownovadoz-0715.slnx`의 기존
`<Project Path="SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj" />` 줄 바로
다음 줄에 추가:

```xml
  <Project Path="SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj" />
```

- [ ] **Step 7: 테스트가 실패(컴파일 에러)하는지 확인 (Red)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `SampleOrderSystem-ownovadoz-0715.Tests` 프로젝트에서 `Clock.h: No such file or directory` 류의
컴파일 오류로 빌드 실패 (Clock.h가 아직 없으므로)

- [ ] **Step 8: `Clock.h` 구현**

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

- [ ] **Step 9: `Clock.cpp` 구현**

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

- [ ] **Step 10: 앱 vcxproj/.filters에 Clock 파일 등록**

`SampleOrderSystem-ownovadoz-0715.vcxproj`의 Task 1에서 추가한 `<ItemGroup>` 두 개를 아래처럼 확장:

```xml
  <ItemGroup>
    <ClCompile Include="main.cpp" />
    <ClCompile Include="Common\Clock.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Common\Time.h" />
    <ClInclude Include="Common\OrderStatus.h" />
    <ClInclude Include="Common\Sample.h" />
    <ClInclude Include="Common\Order.h" />
    <ClInclude Include="Common\Clock.h" />
  </ItemGroup>
```

`SampleOrderSystem-ownovadoz-0715.vcxproj.filters`에도 동일하게 `Common\Clock.cpp`(소스 파일 필터),
`Common\Clock.h`(헤더 파일 필터) 항목을 추가한다.

- [ ] **Step 11: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.` (두 프로젝트 모두)

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected:
```
[PASS] FakeClock_ReturnsSetTime
[PASS] FakeClock_AdvanceMovesTimeForward
[PASS] SystemClock_ReturnsRealTime
3/3 tests passed
```
종료 코드 0

(빌드 산출물 경로가 다르게 나오면 `msbuild` 로그의 마지막 링크 단계 출력 경로를 확인해 그 경로에서 실행)

- [ ] **Step 12: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/Common/Clock.h SampleOrderSystem-ownovadoz-0715/Common/Clock.cpp \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests SampleOrderSystem-ownovadoz-0715.slnx
git commit -m "모킹 가능한 Clock 추상화와 자체 테스트 하네스(MicroTest), Tests 프로젝트 추가"
```

---

## 완료 후 상태

- 앱 프로젝트가 빌드되고 실행되며, 5개 모듈이 공유할 `common::Sample`/`common::Order`/`common::OrderStatus`
  타입과 `common::IClock`/`SystemClock`/`FakeClock`이 준비됨
- `SampleOrderSystem-ownovadoz-0715.Tests` 프로젝트와 `TEST_CASE`/`REQUIRE` 매크로로 이후 모든 모듈이 바로
  TDD를 시작할 수 있음
- 다음 단계: 시료 담당자 모듈(레이어 0)부터 계획 수립 — `docs/superpowers/specs/2026-07-15-module-architecture-design.md`
  4.1절/5장/10.1~10.3절 참고
