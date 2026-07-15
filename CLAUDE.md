# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project status

This repository currently contains only an empty Visual Studio C++ console application skeleton
(`SampleOrderSystem-ownovadoz-0715.vcxproj` / `.slnx`) — no source files exist yet. Requirements live in
`docs/requirements.pdf` (official spec, Korean) and `docs/memo.txt` (developer's own clarifying notes/decisions
on ambiguous points in the spec). Read both before implementing; `memo.txt` resolves several ambiguities left
open by the spec and should take precedence where the two differ.

This repo ("SampleOrderSystem-*") is the final project build, distinct from four separate PoC repos
(`ConsoleMVC-*`, `DataPersistence-*`, `DataMonitor-*`, `DummyDataGenerator-*`) that validate MVC structure,
persistence, a data monitoring console tool, and dummy data generation respectively. This project is not meant
to import those as libraries, but its architecture (MVC skeleton, persistence approach, monitoring tool) should
be built following the same patterns/foundations established in those PoCs when they are available alongside
this repo.

## Build

Visual Studio 2022+ (PlatformToolset v145), C++20, console application (`_CONSOLE`), targets Win32 and x64,
Debug and Release configurations.

- Open `SampleOrderSystem-ownovadoz-0715.slnx` in Visual Studio and build, or via CLI:
  `msbuild SampleOrderSystem-ownovadoz-0715.slnx /p:Configuration=Debug /p:Platform=x64`
- No test framework or lint config is present yet — set one up (e.g. a `Tests` project alongside the app) as
  part of implementing the harness called for in the requirements.

## Domain overview

The system is a console-driven order/production/inventory manager for a fictional semiconductor sample
("시료") vendor. A **customer** requests samples by email; an **order clerk** (주문 담당자) enters the order
into the system; a **production clerk** (생산 담당자) approves/rejects it and owns the production line. There is
no direct link between the order clerk and the production line — orders only interact with inventory and the
production queue through the approval step.

### Core entities

- **Sample (시료)**: `sample ID`, `name`, `average production time`, `yield` (수율 = good units / total units
  produced, e.g. 0.9). Only registered samples can be ordered.
- **Order (주문)**: `order ID`, `sample ID`, `customer name`, `quantity`, `status`.
- **Inventory**: per-sample stock count.
- **Production line**: a single line, produces one unit at a time, FIFO queue of production jobs.

### Order state machine

```
RESERVED --(reject)--> REJECTED                          [terminal, excluded from all monitoring]
RESERVED --(approve, stock sufficient)--> CONFIRMED
RESERVED --(approve, stock insufficient)--> PRODUCING --(production job completes)--> CONFIRMED
CONFIRMED --(manual release action)--> RELEASE
```

- `RESERVED`: order accepted, awaiting approval decision.
- `REJECTED`: rejected by production clerk; not a normal-flow state, always excluded from monitoring.
- `PRODUCING`: approved, but available stock was insufficient — a production job was queued to cover the
  shortfall.
- `CONFIRMED`: approved and stock is sufficient to fulfill the order (either immediately, or once the queued
  production job finishes) — awaiting manual shipment.
- `RELEASE`: shipped. **Shipment is always a manual, human-triggered action** — production completing must
  never itself flip an order to `RELEASE`, even though inventory accounting treats the order's stock as already
  reserved at that point.

### Inventory reservation semantics

- Placing/approving a new order only ever checks **unreserved** stock (current stock minus what other pending
  orders have already claimed) — the production line and order clerk are never directly coupled, so this
  unreserved figure is the sole handoff between them.
- The **inventory/stock view** (monitoring, sample list) always shows raw on-hand stock, including quantity
  already reserved by other orders — reservation only affects what's available to *new* order approvals, not
  what's displayed as "in stock."

### Production line rules

- Single line, one unit at a time; queued jobs run strictly FIFO — whatever enters the queue is produced
  unconditionally, in order.
- On approval with insufficient stock, the shortfall (order quantity − available stock) is what gets queued for
  production, not the full order quantity.
- Actual production quantity accounts for yield on the way in, but inventory is credited at 100% on completion
  (this is why stock accumulates over time): `actual_qty = ceil(shortfall / yield)`,
  `total_production_time = average_production_time * actual_qty`.
- When a job completes, inventory increases by `actual_qty` and the associated order transitions
  `PRODUCING -> CONFIRMED`.
- No multithreading: production isn't advanced by a background timer/thread. Instead, elapsed real time is
  checked/caught up lazily whenever relevant state is touched (e.g. on order approval, stock inquiry, or
  production line inquiry) — see memo.txt point 20 on when to refresh.

### Persistence & time

- State must be fully saved on exit and restored on startup, including any production job **in progress**
  (single production line's current job and its progress must itself be persisted/restored, not just completed
  jobs).
- On restart, if real time has advanced past a queued job's (or several queued jobs') completion time while the
  app was closed, that catch-up must be applied immediately on load — production is computed from wall-clock
  time, not from an in-memory tick loop.
- Time must be mockable/overridable for tests (don't hard-code `std::chrono::system_clock::now()` calls
  throughout — go through a single time source that tests can override).

### Menu structure (console UI, exact layout is flexible)

- **시료 관리 (Sample management)**: register sample (ID, name, avg. production time, yield), list all
  samples with current stock, search by name/attribute.
- **시료 주문 (Order placement)**: create a RESERVED order (sample ID, customer name, quantity).
- **주문 승인/거절 (Approve/reject)**: list RESERVED orders; approve (auto-routes to CONFIRMED or PRODUCING per
  stock check above) or reject (-> REJECTED) a specific order.
- **모니터링 (Monitoring)**: order counts by status (RESERVED/CONFIRMED/PRODUCING/RELEASE, excluding REJECTED);
  per-sample stock levels with a derived status label — 여유 (ample) / 부족 (low) / 고갈 (zero) relative to
  demand.
- **출고 처리 (Shipment)**: list CONFIRMED orders; ship a specific one, moving it to RELEASE.
- **생산 라인 (Production line)**: currently-producing job's progress; FIFO queue of jobs awaiting production.
