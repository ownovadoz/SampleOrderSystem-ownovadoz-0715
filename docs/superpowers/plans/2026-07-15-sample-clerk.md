# 시료 담당자(Sample Clerk) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 시료 등록/조회/검색 메뉴와 재고 관리를 담당하는 시료 담당자 모듈을 구현한다. 이 모듈은 의존
계층의 레이어 0(아무 모듈도 의존하지 않는 leaf)이므로, 다른 모듈 없이 독립적으로 완성하고 테스트할 수
있다.

**Architecture:** `SampleOrderSystem-ownovadoz-0715/SampleClerk/` 아래에 Model(`SampleModel`) → Controller
(`SampleController`) → View(`SampleView`) 3계층으로 구현한다. Model이 `samples.dat` 파일 입출력과
인메모리 상태를 전담하고, Controller는 검증/업무 로직만 담당하며 자기 Model만 호출한다. View는 콘솔
입출력을 스트림(`std::istream&`/`std::ostream&`)으로 주입받아 테스트 가능하게 만든다.

**Tech Stack:** C++20, Foundation 단계에서 만든 `common::Sample`, `common::Duration` 재사용.

> ⚠️ **테스트 하네스 변경 안내**: 이 플랜 작성 이후 Foundation이 자체 제작 `MicroTest.h` + 별도 Tests
> 프로젝트에서 **gmock/gtest(NuGet, 앱 프로젝트에 이미 추가됨) + 단일 프로젝트**로 바뀌었다
> (`docs/superpowers/plans/2026-07-15-foundation.md` 참고). 아래 스텝을 그대로 따르되 다음 3가지만
> 바꿔서 적용한다:
> 1. `#include "../Testing/MicroTest.h"` → `#include <gtest/gtest.h>`
> 2. `TEST_CASE(이름) { ... }` → `TEST(SampleClerkTest, 이름) { ... }`, `REQUIRE(expr)` → `EXPECT_TRUE(expr)`
>    (동등 비교는 `EXPECT_EQ`가 더 적합하면 그것을 사용)
> 3. 테스트 파일과 그 안의 include 경로는 별도 `.Tests` 프로젝트가 아니라 **이 앱 프로젝트 안에 그대로**
>    둔다 (예: `SampleOrderSystem-ownovadoz-0715.Tests/SampleClerk/SampleModelTests.cpp` →
>    `SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleModelTests.cpp`, `#include
>    "../../SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleModel.h"` → `#include "SampleModel.h"`).
>    `.Tests.vcxproj`/`.Tests.vcxproj.filters` 관련 스텝은 전부 건너뛰고, 같은 파일들을 앱 프로젝트의
>    `.vcxproj`/`.vcxproj.filters`에만 한 번 등록한다.

## Global Constraints

- 아키텍처 스펙 `docs/superpowers/specs/2026-07-15-module-architecture-design.md` 4.1절(데이터 구조),
  5장(공개 API), 8장(MVC 내부 계층), 9장(View 계층), 10.1~10.3절(화면 명세)을 그대로 따른다.
- 스펙 11장에서 "다음 단계에서 정한다"고 미뤄둔 항목 중 이 계획에서 확정하는 것:
  - **에러 처리 방식**: 예외 대신 `common::Result{bool ok; std::string error;}` 반환값 사용 (실패 이유를
    호출부가 그대로 화면에 보여줘야 하는 화면 흐름이 많아, 예외보다 반환값이 다루기 쉽다)
  - **`samples.json` 파일 포맷**: JSON 포맷(`Common/FileRepository.h`의 `FileRepository<T>` +
    `Common::Sample`/`Common::Order`의 `ToJson`/`FromJson`, PRD.md 3장 및 `DataPersistence-ownovadoz-0715`
    PoC 참고)
- 새로 등록된 시료의 초기 재고는 0이다 (명세에 없어 이번에 정함 — 생산 이력이 없으니 0이 자연스러움)
- 평균 생산시간은 사용자에게는 "분" 단위로 입력받아 내부적으로 `common::Duration`(초)으로 변환해 저장한다

---

### Task 1: SampleModel — 파일 저장/복원 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/Common/StringUtil.h`
- Create: `SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleModel.h`
- Create: `SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleModel.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters`
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/SampleClerk/SampleModelTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`

**Interfaces:**
- Consumes: `common::Sample`, `common::Duration` (`Common/Sample.h`, `Common/Time.h`, Foundation 단계 산출물)
- Produces: `sampleclerk::SampleModel` — `load()`, `save() -> bool`, `exists(id) -> bool`,
  `insert(Sample)`, `find(id) -> optional<Sample>`, `findAll() -> vector<Sample>`,
  `updateStock(id, newStock)`. Task 2(Controller)가 이 타입을 그대로 사용한다.

- [ ] **Step 1: 공용 문자열 유틸리티 작성 — `Common/StringUtil.h`**

```cpp
#pragma once
#include <string>
#include <vector>

namespace common {

inline std::vector<std::string> splitString(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    for (char c : s) {
        if (c == delimiter) {
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    result.push_back(current);
    return result;
}

} // namespace common
```

- [ ] **Step 2: 실패하는 테스트 작성 — `SampleModelTests.cpp`** (아직 `SampleModel.h`가 없어 컴파일 실패 예상)

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleModel.h"
#include "../Testing/MicroTest.h"
#include <cstdio>

using namespace sampleclerk;
using namespace common;

TEST_CASE(SampleModel_LoadFromMissingFile_StartsEmpty) {
    SampleModel model("nonexistent_samples_test.dat");
    model.load();
    REQUIRE(model.findAll().empty());
}

TEST_CASE(SampleModel_InsertAndFind) {
    SampleModel model("samples_test_insert.dat");
    Sample s{"S-001", "테스트 시료", Duration(60), 0.9, 100};
    model.insert(s);
    auto found = model.find("S-001");
    REQUIRE(found.has_value());
    REQUIRE(found->name == "테스트 시료");
    REQUIRE(found->stock == 100);
}

TEST_CASE(SampleModel_SaveThenLoad_RoundTrips) {
    const std::string path = "samples_test_roundtrip.dat";
    std::remove(path.c_str());
    {
        SampleModel model(path);
        model.insert(Sample{"S-001", "실리콘 웨이퍼", Duration(1800), 0.92, 480});
        model.insert(Sample{"S-002", "GaN 에피택셜", Duration(1080), 0.78, 220});
        REQUIRE(model.save());
    }
    {
        SampleModel model(path);
        model.load();
        auto all = model.findAll();
        REQUIRE(all.size() == 2);
        auto found = model.find("S-002");
        REQUIRE(found.has_value());
        REQUIRE(found->yield == 0.78);
        REQUIRE(found->stock == 220);
    }
    std::remove(path.c_str());
}

TEST_CASE(SampleModel_UpdateStock_ChangesValue) {
    SampleModel model("samples_test_update.dat");
    model.insert(Sample{"S-001", "테스트", Duration(60), 0.9, 10});
    model.updateStock("S-001", 25);
    REQUIRE(model.find("S-001")->stock == 25);
}
```

- [ ] **Step 3: Tests 프로젝트에 파일 등록 (아직 실패 상태로)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 기존 두 `ItemGroup`(ClCompile/ClInclude)을 아래로 교체:

```xml
  <ItemGroup>
    <ClCompile Include="TestMain.cpp" />
    <ClCompile Include="ClockTests.cpp" />
    <ClCompile Include="SampleClerk\SampleModelTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Testing\MicroTest.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Time.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Sample.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\StringUtil.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.h" />
  </ItemGroup>
```

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`의 두 `ItemGroup`(ClCompile/ClInclude)을 아래로
교체:

```xml
  <ItemGroup>
    <ClCompile Include="TestMain.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="ClockTests.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="SampleClerk\SampleModelTests.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.cpp">
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
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Sample.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\StringUtil.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
  </ItemGroup>
```

- [ ] **Step 4: 컴파일 실패 확인 (Red)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `SampleModel.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 5: `SampleModel.h` 구현**

```cpp
#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "../Common/Sample.h"

namespace sampleclerk {

class SampleModel {
public:
    explicit SampleModel(std::string filePath);

    void load();
    bool save() const;

    bool exists(const std::string& id) const;
    void insert(const common::Sample& sample);
    std::optional<common::Sample> find(const std::string& id) const;
    std::vector<common::Sample> findAll() const;
    void updateStock(const std::string& id, int newStock);

private:
    std::string filePath_;
    std::map<std::string, common::Sample> samples_;
};

} // namespace sampleclerk
```

- [ ] **Step 6: `SampleModel.cpp` 구현**

```cpp
#include "SampleModel.h"
#include "../Common/StringUtil.h"
#include <fstream>
#include <iomanip>

namespace sampleclerk {

SampleModel::SampleModel(std::string filePath) : filePath_(std::move(filePath)) {}

void SampleModel::load() {
    samples_.clear();
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        return; // 파일이 없으면 빈 상태로 시작
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto fields = common::splitString(line, '|');
        if (fields.size() != 5) continue;
        common::Sample sample;
        sample.id = fields[0];
        sample.name = fields[1];
        sample.avgProductionTime = common::Duration(std::stoll(fields[2]));
        sample.yield = std::stod(fields[3]);
        sample.stock = std::stoi(fields[4]);
        samples_[sample.id] = sample;
    }
}

bool SampleModel::save() const {
    std::ofstream out(filePath_, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << std::setprecision(10);
    for (const auto& [id, sample] : samples_) {
        out << sample.id << '|' << sample.name << '|' << sample.avgProductionTime.count() << '|'
            << sample.yield << '|' << sample.stock << '\n';
    }
    return true;
}

bool SampleModel::exists(const std::string& id) const {
    return samples_.find(id) != samples_.end();
}

void SampleModel::insert(const common::Sample& sample) {
    samples_[sample.id] = sample;
}

std::optional<common::Sample> SampleModel::find(const std::string& id) const {
    auto it = samples_.find(id);
    if (it == samples_.end()) return std::nullopt;
    return it->second;
}

std::vector<common::Sample> SampleModel::findAll() const {
    std::vector<common::Sample> result;
    for (const auto& [id, sample] : samples_) {
        result.push_back(sample);
    }
    return result;
}

void SampleModel::updateStock(const std::string& id, int newStock) {
    auto it = samples_.find(id);
    if (it != samples_.end()) {
        it->second.stock = newStock;
    }
}

} // namespace sampleclerk
```

- [ ] **Step 7: 앱 vcxproj/.filters에도 SampleModel 등록**

`SampleOrderSystem-ownovadoz-0715.vcxproj`의 두 `ItemGroup`을 아래로 교체:

```xml
  <ItemGroup>
    <ClCompile Include="main.cpp" />
    <ClCompile Include="Common\Clock.cpp" />
    <ClCompile Include="SampleClerk\SampleModel.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Common\Time.h" />
    <ClInclude Include="Common\OrderStatus.h" />
    <ClInclude Include="Common\Sample.h" />
    <ClInclude Include="Common\Order.h" />
    <ClInclude Include="Common\Clock.h" />
    <ClInclude Include="Common\StringUtil.h" />
    <ClInclude Include="SampleClerk\SampleModel.h" />
  </ItemGroup>
```

`SampleOrderSystem-ownovadoz-0715.vcxproj.filters`의 두 `ItemGroup`을 아래로 교체:

```xml
  <ItemGroup>
    <ClCompile Include="main.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="Common\Clock.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="SampleClerk\SampleModel.cpp">
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
    <ClInclude Include="Common\Clock.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="Common\StringUtil.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="SampleClerk\SampleModel.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
  </ItemGroup>
```

- [ ] **Step 8: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: 기존 Clock 테스트 3개 + 이번에 추가한 4개 = `7/7 tests passed` (아래 4개 항목이 `[PASS]`로 새로
포함됨: `SampleModel_LoadFromMissingFile_StartsEmpty`, `SampleModel_InsertAndFind`,
`SampleModel_SaveThenLoad_RoundTrips`, `SampleModel_UpdateStock_ChangesValue`)

- [ ] **Step 9: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/Common/StringUtil.h \
        SampleOrderSystem-ownovadoz-0715/SampleClerk \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "시료 담당자: SampleModel (파일 저장/복원) 추가"
```

---

### Task 2: SampleController — 등록 검증/재고 관리 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/Common/Result.h`
- Create: `SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleController.h`
- Create: `SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleController.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/SampleClerk/SampleControllerTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+filters)

**Interfaces:**
- Consumes: `sampleclerk::SampleModel` (Task 1)
- Produces: `sampleclerk::SampleController` — `registerSample(id, name, avgProductionTime, yield) -> Result`,
  `listSamples() -> vector<Sample>`, `searchSamples(keyword) -> vector<Sample>`,
  `getSample(id) -> optional<Sample>`, `getStock(id) -> int`, `increaseStock(id, qty) -> Result`,
  `decreaseStock(id, qty) -> Result`. 다른 모듈(생산 담당자, 생산 라인, 모니터링)의 계획이 이 시그니처를
  그대로 호출한다.

- [ ] **Step 1: `Common/Result.h` 작성**

```cpp
#pragma once
#include <string>
#include <utility>

namespace common {

struct Result {
    bool ok;
    std::string error;

    static Result success() { return Result{true, ""}; }
    static Result failure(std::string err) { return Result{false, std::move(err)}; }
};

} // namespace common
```

- [ ] **Step 2: 실패하는 테스트 작성 — `SampleControllerTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleController.h"
#include "../Testing/MicroTest.h"

using namespace sampleclerk;
using namespace common;

TEST_CASE(SampleController_RegisterSample_Succeeds) {
    SampleModel model("controller_test_register.dat");
    SampleController controller(model);
    auto result = controller.registerSample("S-001", "실리콘 웨이퍼", Duration(1800), 0.92);
    REQUIRE(result.ok);
    auto sample = controller.getSample("S-001");
    REQUIRE(sample.has_value());
    REQUIRE(sample->stock == 0);
}

TEST_CASE(SampleController_RegisterDuplicateId_Fails) {
    SampleModel model("controller_test_dup.dat");
    SampleController controller(model);
    controller.registerSample("S-001", "샘플1", Duration(1800), 0.9);
    auto result = controller.registerSample("S-001", "샘플2", Duration(1200), 0.8);
    REQUIRE(!result.ok);
}

TEST_CASE(SampleController_RegisterInvalidYield_Fails) {
    SampleModel model("controller_test_yield.dat");
    SampleController controller(model);
    auto tooHigh = controller.registerSample("S-001", "샘플", Duration(1800), 1.5);
    REQUIRE(!tooHigh.ok);
    auto zero = controller.registerSample("S-002", "샘플2", Duration(1800), 0.0);
    REQUIRE(!zero.ok);
}

TEST_CASE(SampleController_SearchSamples_CaseInsensitiveSubstring) {
    SampleModel model("controller_test_search.dat");
    SampleController controller(model);
    controller.registerSample("S-001", "Silicon Wafer", Duration(1800), 0.9);
    controller.registerSample("S-002", "GaN Epitaxial", Duration(1200), 0.8);
    auto results = controller.searchSamples("silicon");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].id == "S-001");
}

TEST_CASE(SampleController_IncreaseAndDecreaseStock) {
    SampleModel model("controller_test_stock.dat");
    SampleController controller(model);
    controller.registerSample("S-001", "샘플", Duration(1800), 0.9);
    REQUIRE(controller.increaseStock("S-001", 50).ok);
    REQUIRE(controller.getStock("S-001") == 50);
    REQUIRE(controller.decreaseStock("S-001", 20).ok);
    REQUIRE(controller.getStock("S-001") == 30);
}

TEST_CASE(SampleController_DecreaseStockBelowZero_Fails) {
    SampleModel model("controller_test_stock_fail.dat");
    SampleController controller(model);
    controller.registerSample("S-001", "샘플", Duration(1800), 0.9);
    controller.increaseStock("S-001", 10);
    auto result = controller.decreaseStock("S-001", 20);
    REQUIRE(!result.ok);
    REQUIRE(controller.getStock("S-001") == 10);
}
```

- [ ] **Step 3: Tests 프로젝트/앱 프로젝트 vcxproj·filters에 파일 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 두 `ItemGroup`을 아래로 교체:

```xml
  <ItemGroup>
    <ClCompile Include="TestMain.cpp" />
    <ClCompile Include="ClockTests.cpp" />
    <ClCompile Include="SampleClerk\SampleModelTests.cpp" />
    <ClCompile Include="SampleClerk\SampleControllerTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleController.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Testing\MicroTest.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Time.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Sample.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\StringUtil.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Result.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleController.h" />
  </ItemGroup>
```

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`의 두 `ItemGroup`에 각각 아래 항목을 추가한다
(기존 항목은 그대로 유지):

```xml
    <ClCompile Include="SampleClerk\SampleControllerTests.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleController.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
```

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Result.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleController.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
```

앱 프로젝트(`SampleOrderSystem-ownovadoz-0715.vcxproj`)의 두 `ItemGroup`도 아래로 교체:

```xml
  <ItemGroup>
    <ClCompile Include="main.cpp" />
    <ClCompile Include="Common\Clock.cpp" />
    <ClCompile Include="SampleClerk\SampleModel.cpp" />
    <ClCompile Include="SampleClerk\SampleController.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Common\Time.h" />
    <ClInclude Include="Common\OrderStatus.h" />
    <ClInclude Include="Common\Sample.h" />
    <ClInclude Include="Common\Order.h" />
    <ClInclude Include="Common\Clock.h" />
    <ClInclude Include="Common\StringUtil.h" />
    <ClInclude Include="Common\Result.h" />
    <ClInclude Include="SampleClerk\SampleModel.h" />
    <ClInclude Include="SampleClerk\SampleController.h" />
  </ItemGroup>
```

`SampleOrderSystem-ownovadoz-0715.vcxproj.filters`의 두 `ItemGroup`에도 아래 항목을 추가한다 (기존 항목은
그대로 유지):

```xml
    <ClCompile Include="SampleClerk\SampleController.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
```

```xml
    <ClInclude Include="Common\Result.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
    <ClInclude Include="SampleClerk\SampleController.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
```

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `SampleController.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 4: `SampleController.h` 구현**

```cpp
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "SampleModel.h"
#include "../Common/Result.h"

namespace sampleclerk {

class SampleController {
public:
    explicit SampleController(SampleModel& model);

    common::Result registerSample(const std::string& id, const std::string& name,
                                   common::Duration avgProductionTime, double yield);
    std::vector<common::Sample> listSamples() const;
    std::vector<common::Sample> searchSamples(const std::string& keyword) const;
    std::optional<common::Sample> getSample(const std::string& id) const;
    int getStock(const std::string& id) const;
    common::Result increaseStock(const std::string& id, int qty);
    common::Result decreaseStock(const std::string& id, int qty);

private:
    SampleModel& model_;
};

} // namespace sampleclerk
```

- [ ] **Step 5: `SampleController.cpp` 구현**

```cpp
#include "SampleController.h"
#include <algorithm>
#include <cctype>

namespace sampleclerk {

namespace {
std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}
} // namespace

SampleController::SampleController(SampleModel& model) : model_(model) {}

common::Result SampleController::registerSample(const std::string& id, const std::string& name,
                                                  common::Duration avgProductionTime, double yield) {
    if (model_.exists(id)) {
        return common::Result::failure("이미 존재하는 시료 ID입니다: " + id);
    }
    if (yield <= 0.0 || yield > 1.0) {
        return common::Result::failure("수율은 0보다 크고 1 이하이어야 합니다");
    }
    if (avgProductionTime.count() <= 0) {
        return common::Result::failure("평균 생산시간은 0보다 커야 합니다");
    }
    common::Sample sample{id, name, avgProductionTime, yield, 0};
    model_.insert(sample);
    return common::Result::success();
}

std::vector<common::Sample> SampleController::listSamples() const {
    return model_.findAll();
}

std::vector<common::Sample> SampleController::searchSamples(const std::string& keyword) const {
    std::vector<common::Sample> result;
    std::string lowerKeyword = toLower(keyword);
    for (const auto& sample : model_.findAll()) {
        if (toLower(sample.name).find(lowerKeyword) != std::string::npos) {
            result.push_back(sample);
        }
    }
    return result;
}

std::optional<common::Sample> SampleController::getSample(const std::string& id) const {
    return model_.find(id);
}

int SampleController::getStock(const std::string& id) const {
    auto sample = model_.find(id);
    return sample.has_value() ? sample->stock : 0;
}

common::Result SampleController::increaseStock(const std::string& id, int qty) {
    auto sample = model_.find(id);
    if (!sample.has_value()) {
        return common::Result::failure("존재하지 않는 시료 ID입니다: " + id);
    }
    model_.updateStock(id, sample->stock + qty);
    return common::Result::success();
}

common::Result SampleController::decreaseStock(const std::string& id, int qty) {
    auto sample = model_.find(id);
    if (!sample.has_value()) {
        return common::Result::failure("존재하지 않는 시료 ID입니다: " + id);
    }
    if (sample->stock < qty) {
        return common::Result::failure("재고보다 많은 수량을 차감할 수 없습니다");
    }
    model_.updateStock(id, sample->stock - qty);
    return common::Result::success();
}

} // namespace sampleclerk
```

- [ ] **Step 6: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: `13/13 tests passed` (Clock 3 + SampleModel 4 + SampleController 6개 신규:
`SampleController_RegisterSample_Succeeds`, `SampleController_RegisterDuplicateId_Fails`,
`SampleController_RegisterInvalidYield_Fails`, `SampleController_SearchSamples_CaseInsensitiveSubstring`,
`SampleController_IncreaseAndDecreaseStock`, `SampleController_DecreaseStockBelowZero_Fails`)

- [ ] **Step 7: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/Common/Result.h \
        SampleOrderSystem-ownovadoz-0715/SampleClerk \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "시료 담당자: SampleController (등록 검증/재고 관리) 추가"
```

---

### Task 3: SampleView — 콘솔 화면 + 메인 메뉴 연결 (TDD)

**Files:**
- Create: `SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleView.h`
- Create: `SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleView.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/main.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` (+filters)
- Create: `SampleOrderSystem-ownovadoz-0715.Tests/SampleClerk/SampleViewTests.cpp`
- Modify: `SampleOrderSystem-ownovadoz-0715.Tests/SampleOrderSystem-ownovadoz-0715.Tests.vcxproj` (+filters)

**Interfaces:**
- Consumes: `sampleclerk::SampleController` (Task 2)
- Produces: `sampleclerk::SampleView` — `showRegisterScreen(istream&, ostream&)`,
  `showListScreen(ostream&)`, `showSearchScreen(istream&, ostream&)`. main.cpp가 메인 메뉴에서 호출한다.

- [ ] **Step 1: 실패하는 테스트 작성 — `SampleViewTests.cpp`**

```cpp
#include "../../SampleOrderSystem-ownovadoz-0715/SampleClerk/SampleView.h"
#include "../Testing/MicroTest.h"
#include <sstream>

using namespace sampleclerk;
using namespace common;

TEST_CASE(SampleView_ListScreen_ShowsEmptyMessage) {
    SampleModel model("view_test_empty.dat");
    SampleController controller(model);
    SampleView view(controller);
    std::ostringstream out;
    view.showListScreen(out);
    REQUIRE(out.str().find("등록된 시료가 없습니다") != std::string::npos);
}

TEST_CASE(SampleView_RegisterScreen_SuccessShowsSample) {
    SampleModel model("view_test_register.dat");
    SampleController controller(model);
    SampleView view(controller);
    std::istringstream in("S-001\n실리콘 웨이퍼\n30\n0.92\n");
    std::ostringstream out;
    view.showRegisterScreen(in, out);
    REQUIRE(out.str().find("등록 완료") != std::string::npos);
    REQUIRE(out.str().find("실리콘 웨이퍼") != std::string::npos);
}

TEST_CASE(SampleView_SearchScreen_NoResults) {
    SampleModel model("view_test_search.dat");
    SampleController controller(model);
    SampleView view(controller);
    std::istringstream in("존재하지않는이름\n");
    std::ostringstream out;
    view.showSearchScreen(in, out);
    REQUIRE(out.str().find("검색 결과 없음") != std::string::npos);
}
```

- [ ] **Step 2: vcxproj/필터에 등록 후 컴파일 실패 확인 (Red)**

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj`의 두 `ItemGroup`을 아래로 교체:

```xml
  <ItemGroup>
    <ClCompile Include="TestMain.cpp" />
    <ClCompile Include="ClockTests.cpp" />
    <ClCompile Include="SampleClerk\SampleModelTests.cpp" />
    <ClCompile Include="SampleClerk\SampleControllerTests.cpp" />
    <ClCompile Include="SampleClerk\SampleViewTests.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleController.cpp" />
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleView.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Testing\MicroTest.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Clock.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Time.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Sample.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\StringUtil.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\Common\Result.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleModel.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleController.h" />
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleView.h" />
  </ItemGroup>
```

`SampleOrderSystem-ownovadoz-0715.Tests.vcxproj.filters`의 두 `ItemGroup`에 각각 아래 항목을 추가한다
(기존 항목은 그대로 유지):

```xml
    <ClCompile Include="SampleClerk\SampleViewTests.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
    <ClCompile Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleView.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
```

```xml
    <ClInclude Include="..\SampleOrderSystem-ownovadoz-0715\SampleClerk\SampleView.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
```

앱 프로젝트(`SampleOrderSystem-ownovadoz-0715.vcxproj`)의 두 `ItemGroup`도 아래로 교체:

```xml
  <ItemGroup>
    <ClCompile Include="main.cpp" />
    <ClCompile Include="Common\Clock.cpp" />
    <ClCompile Include="SampleClerk\SampleModel.cpp" />
    <ClCompile Include="SampleClerk\SampleController.cpp" />
    <ClCompile Include="SampleClerk\SampleView.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="Common\Time.h" />
    <ClInclude Include="Common\OrderStatus.h" />
    <ClInclude Include="Common\Sample.h" />
    <ClInclude Include="Common\Order.h" />
    <ClInclude Include="Common\Clock.h" />
    <ClInclude Include="Common\StringUtil.h" />
    <ClInclude Include="Common\Result.h" />
    <ClInclude Include="SampleClerk\SampleModel.h" />
    <ClInclude Include="SampleClerk\SampleController.h" />
    <ClInclude Include="SampleClerk\SampleView.h" />
  </ItemGroup>
```

`SampleOrderSystem-ownovadoz-0715.vcxproj.filters`의 두 `ItemGroup`에도 아래 항목을 추가한다 (기존 항목은
그대로 유지):

```xml
    <ClCompile Include="SampleClerk\SampleView.cpp">
      <Filter>소스 파일</Filter>
    </ClCompile>
```

```xml
    <ClInclude Include="SampleClerk\SampleView.h">
      <Filter>헤더 파일</Filter>
    </ClInclude>
```

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `SampleView.h: No such file or directory` 류의 컴파일 오류로 실패

- [ ] **Step 3: `SampleView.h` 구현**

```cpp
#pragma once
#include <iostream>
#include <vector>
#include "SampleController.h"

namespace sampleclerk {

class SampleView {
public:
    explicit SampleView(SampleController& controller);

    void showRegisterScreen(std::istream& in, std::ostream& out);
    void showListScreen(std::ostream& out);
    void showSearchScreen(std::istream& in, std::ostream& out);

private:
    void printTable(const std::vector<common::Sample>& samples, std::ostream& out);
    SampleController& controller_;
};

} // namespace sampleclerk
```

- [ ] **Step 4: `SampleView.cpp` 구현**

```cpp
#include "SampleView.h"

namespace sampleclerk {

SampleView::SampleView(SampleController& controller) : controller_(controller) {}

void SampleView::printTable(const std::vector<common::Sample>& samples, std::ostream& out) {
    out << "ID\t이름\t평균생산시간(분)\t수율\t재고\n";
    for (const auto& sample : samples) {
        out << sample.id << '\t' << sample.name << '\t'
            << (sample.avgProductionTime.count() / 60) << '\t'
            << sample.yield << '\t' << sample.stock << '\n';
    }
}

void SampleView::showRegisterScreen(std::istream& in, std::ostream& out) {
    out << "시료 ID > ";
    std::string id;
    std::getline(in, id);

    out << "이름 > ";
    std::string name;
    std::getline(in, name);

    out << "평균 생산시간(분) > ";
    int minutes = 0;
    in >> minutes;

    out << "수율 > ";
    double yield = 0.0;
    in >> yield;
    in.ignore();

    auto result = controller_.registerSample(id, name, common::Duration(minutes * 60), yield);
    if (result.ok) {
        out << "등록 완료\n";
        printTable({*controller_.getSample(id)}, out);
    } else {
        out << "등록 실패: " << result.error << "\n";
    }
}

void SampleView::showListScreen(std::ostream& out) {
    auto samples = controller_.listSamples();
    if (samples.empty()) {
        out << "등록된 시료가 없습니다\n";
        return;
    }
    printTable(samples, out);
}

void SampleView::showSearchScreen(std::istream& in, std::ostream& out) {
    out << "검색어 > ";
    std::string keyword;
    std::getline(in, keyword);

    auto results = controller_.searchSamples(keyword);
    if (results.empty()) {
        out << "검색 결과 없음\n";
        return;
    }
    printTable(results, out);
}

} // namespace sampleclerk
```

- [ ] **Step 5: 테스트 통과 확인 (Green)**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Expected: `Build succeeded.`

Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.Tests.exe`
Expected: `16/16 tests passed` (Clock 3 + SampleModel 4 + SampleController 6 + SampleView 3개 신규:
`SampleView_ListScreen_ShowsEmptyMessage`, `SampleView_RegisterScreen_SuccessShowsSample`,
`SampleView_SearchScreen_NoResults`)

- [ ] **Step 6: main.cpp에 메인 메뉴 연결**

```cpp
#include <iostream>
#include "SampleClerk/SampleModel.h"
#include "SampleClerk/SampleController.h"
#include "SampleClerk/SampleView.h"

int main() {
    sampleclerk::SampleModel sampleModel("samples.dat");
    sampleModel.load();
    sampleclerk::SampleController sampleController(sampleModel);
    sampleclerk::SampleView sampleView(sampleController);

    while (true) {
        std::cout << "\n반도체 시료 생산주문관리 시스템\n";
        std::cout << "[1] 시료 등록  [2] 시료 조회  [3] 시료 검색  [0] 종료\n";
        std::cout << "선택 > ";
        int choice = 0;
        if (!(std::cin >> choice)) break;
        std::cin.ignore();

        if (choice == 1) {
            sampleView.showRegisterScreen(std::cin, std::cout);
        } else if (choice == 2) {
            sampleView.showListScreen(std::cout);
        } else if (choice == 3) {
            sampleView.showSearchScreen(std::cin, std::cout);
        } else if (choice == 0) {
            break;
        }
    }

    sampleModel.save();
    return 0;
}
```

(다른 모듈 계획들이 이 `while` 루프의 메뉴 항목 4~5번을 이어서 추가한다. 지금은 시료 관리 3개 메뉴만
동작한다.)

- [ ] **Step 7: 실행 파일로 수동 확인**

Run: `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
Run: `x64\Debug\SampleOrderSystem-ownovadoz-0715.exe`
- `1` 선택 → `S-001` / `실리콘 웨이퍼` / `30` / `0.92` 입력 → "등록 완료"와 방금 등록한 시료 정보가 표에
  출력되는지 확인
- `2` 선택 → 방금 등록한 시료가 재고 0으로 표시되는지 확인
- `3` 선택 → `실리콘` 검색 시 해당 시료가 나오는지 확인
- `0` 선택 → 종료. 저장소 루트에 `samples.dat` 파일이 생성됐는지, 다시 실행해서 `2` 선택 시 데이터가
  복원되는지 확인

- [ ] **Step 8: 커밋**

```bash
git add SampleOrderSystem-ownovadoz-0715/SampleClerk SampleOrderSystem-ownovadoz-0715/main.cpp \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj \
        SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj.filters \
        SampleOrderSystem-ownovadoz-0715.Tests
git commit -m "시료 담당자: SampleView 및 메인 메뉴 연결"
```

---

## 완료 후 상태

- 시료 등록/조회/검색이 실제로 동작하고, 실행 시 로드/종료 시 저장이 이루어짐
- `sampleclerk::SampleController`의 공개 API(`getSample`, `getStock`, `increaseStock`, `decreaseStock`,
  `listSamples`, `searchSamples`)가 다음 모듈(주문 담당자) plan에서 그대로 재사용됨
- 다음 단계: 주문 담당자 모듈(레이어 1) plan — 스펙 4.2절/5장/10.4절 참고, `SampleController`를 참조로
  주입받아 시료 존재 확인에 사용
