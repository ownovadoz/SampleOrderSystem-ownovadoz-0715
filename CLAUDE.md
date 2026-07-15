# CLAUDE.md

이 문서는 Claude Code(claude.ai/code)가 이 저장소에서 작업할 때 참고하는 안내 문서입니다.

## 문서 구조 안내

> **핵심 요약**: 공통 규칙은 `PRD.md`, 모듈별 상세 규칙은 `docs/requirements/*.md`. 모듈을 개발할 때는
> `PRD.md` + 해당 모듈 문서 두 개만 읽으면 된다.

각 모듈(시료 관리 / 주문 접수·승인·거절 / 생산 라인 / 모니터링 / 출고 처리)은 서로 다른 에이전트가 나누어
개발하기 때문에, 요구사항 문서를 아래와 같이 분리해 두었습니다.

- **`PRD.md`**: 모든 모듈이 공통으로 알아야 하는 내용만 (공유 도메인 모델, 주문 상태 전이, 재고 선점 규칙,
  시간/영속성 정책, 아키텍처·개발 공통 규칙).
- **`docs/requirements/*.md`** (모듈별 1개 파일): 해당 모듈에만 해당하는 상세 요구사항. 다른 모듈과 겹치는
  내용이라도 각자의 문서에 중복해서 적어 두었으므로, 자신이 맡은 모듈 문서만 봐도 완결적으로 이해할 수
  있습니다.

## 프로젝트 현황

> **핵심 요약**: 아직 소스 코드가 없는 빈 Visual Studio C++20 콘솔 프로젝트 뼈대만 존재한다.

이 저장소는 현재 비어 있는 Visual Studio C++ 콘솔 애플리케이션 뼈대
(`SampleOrderSystem-ownovadoz-0715.vcxproj` / `.slnx`)만 있고, 실제 소스 파일은 아직 없습니다. 요구사항은
`docs/requirements.pdf`(공식 기능 명세, 한글)와 `docs/memo.txt`(명세의 모호한 부분에 대해 개발자 본인이 미리
정리해 둔 결정 사항)에 있습니다. 구현 전 두 문서를 모두 읽어야 하며, 두 문서가 상충할 경우 `memo.txt`의
결정을 우선합니다.

## 참고 PoC 저장소

> **핵심 요약**: 4개의 PoC 저장소가 모두 준비 완료 상태이며, 이 프로젝트는 그 구조를 참고해 다시 구현한다
> (라이브러리로 import하지 않는다).

이 저장소("SampleOrderSystem-ownovadoz-0715")는 최종 프로젝트이며, 아래 4개의 별도 PoC 저장소와는 구분되는
프로젝트입니다.

| PoC 항목 | 저장소 | 상태 |
|---|---|---|
| MVC 스켈레톤 코드 | `ConsoleMVC-ownovadoz-0715` | 준비 완료 |
| 데이터 영속성 처리 | `DataPersistence-ownovadoz-0715` | 준비 완료 |
| 데이터 모니터링 Tool | `DataMonitor-ownovadoz-0715` | 준비 완료 |
| Dummy 데이터 생성 Tool | `DummyDataGenerator-ownovadoz-0715` | 준비 완료 |

이들은 모두 별도의 GitHub 저장소이며, 이 프로젝트가 라이브러리처럼 import하는 대상이 아닙니다. 각 PoC가
검증한 구조/패턴(MVC 계층 분리, 영속성 저장 방식, 모니터링 도구 구조, 더미 데이터 생성 방식)을 참고해서
이 프로젝트 안에 동등한 방식으로 다시 구현합니다.

> 출처: `docs/requirements.pdf` p.25 (미션1 PoC 개발 표), p.27 (제출 방법 — 저장소 이름 규칙) /
> `docs/memo.txt` 25번째 줄("PoC를 그대로 쓰는 건 아니지만 그 기반을 사용해야 함"), 29번째 줄("각 기능을 만들
> 때 PoC로 만든 것들을 참고해서 만들도록 해야 함")

## 빌드

> **핵심 요약**: Visual Studio에서 `.slnx`를 열거나 `msbuild`로 빌드. 테스트 프레임워크는 아직 없음.

Visual Studio 2022+ (PlatformToolset v145), C++20, 콘솔 애플리케이션(`_CONSOLE`), Win32/x64 및 Debug/Release
구성을 모두 지원합니다.

- `SampleOrderSystem-ownovadoz-0715.slnx`를 Visual Studio로 열어 빌드하거나, CLI에서:
  `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
- 아직 테스트 프레임워크나 린트 설정이 없습니다. 요구사항에서 요구하는 Harness(검증 도구) 구축의 일환으로
  (예: 앱 옆에 `Tests` 프로젝트 추가 등) 직접 구성해야 합니다.

> 출처: `SampleOrderSystem-ownovadoz-0715/SampleOrderSystem-ownovadoz-0715.vcxproj` 설정값 (직접 확인) /
> `docs/requirements.pdf` p.26 (미션2 프로젝트 개발 — Test, Harness 주안점)

## 도메인 개요

> **핵심 요약**: 도메인 모델·상태 전이·재고 선점 규칙 등 상세 내용은 `PRD.md` 2장에 정리되어 있다. 여기서는
> 전체 그림만 요약한다.

콘솔 기반으로 동작하는 시료 주문/생산/재고 관리 시스템입니다. **고객**이 시료를 요청하면 **주문 담당자**가
주문을 접수하고, **생산 담당자**가 승인/거절 및 생산 라인을 운용합니다. 주문 담당자와 생산 라인 사이에는
직접적인 연결이 없으며, 재고 수량만이 둘을 잇는 유일한 접점입니다. 상세 도메인 모델(시료/주문 필드), 주문
상태 전이(RESERVED → CONFIRMED/PRODUCING → RELEASE, REJECTED), 재고 선점 규칙, 시간/영속성 공통 정책은
`PRD.md` 2장을 참고하세요.

> 출처: `docs/requirements.pdf` p.6 (역할별 흐름도), p.7 (시스템 개요) / `docs/memo.txt` 8번째 줄
> ("주문담당자 - 생산담당자 - 생산라인에서 주문담당자와 생산라인의 직접적인 연결은 없다는 뜻")
