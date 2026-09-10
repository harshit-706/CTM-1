# Product X — Asset Finder / OSS Research & Reuse Guide

## Purpose

This file tells Codex where to reuse proven open-source building blocks instead of reinventing common UI, graph, chart, industrial-protocol and edge-runtime components.

### Important distinction

The following two repositories are **agent/design skills**, not UI runtime libraries:
- https://github.com/emilkowalski/skills
- https://github.com/ibelick/ui-skills

Use them to improve the coding agent’s design decisions. Do not add them as browser runtime dependencies.

---

## Tier 1 — Use directly in the prototype

### 1. SvelteKit
Repository:
https://github.com/sveltejs/kit

Use for:
- web app
- routing
- server endpoints where needed
- deployment shell

Why:
- mature Svelte web framework
- TypeScript-friendly
- MIT licensed

Suggested setup:
```bash
npx sv create product-x-web
cd product-x-web
npm install
```

Reference:
SvelteKit GitHub indicates MIT licensing and active 2.x releases.

### 2. shadcn-svelte
Repository:
https://github.com/huntabyte/shadcn-svelte

Use for:
- buttons
- dialogs
- dropdowns
- tabs
- forms
- command menus
- sheets/drawers
- tables
- badges
- tooltips

Why:
- components are added/copied into the project and customized
- useful for a serious design system
- MIT licensed

Suggested:
```bash
npx shadcn-svelte@latest init
```

Do not install every component. Add only what the screens need.

### 3. Svelte Flow / xyflow
Repository:
https://github.com/xyflow/xyflow

Use for:
- asset topology canvas
- drag/drop nodes
- typed edges
- zoom/pan
- selection
- connection handles
- node inspectors

Why:
- official repo contains React Flow and Svelte Flow
- Svelte package is `@xyflow/svelte`
- MIT licensed

Suggested:
```bash
npm install @xyflow/svelte
```

### 4. Lucide
Repository:
https://github.com/lucide-icons/lucide

Use for:
- alarm
- motor
- pump
- settings
- energy
- firmware
- sensor
- topology
- command/status icons

Svelte package:
```bash
npm install lucide-svelte
```

License:
ISC.

### 5. Apache ECharts
Repository:
https://github.com/apache/echarts

Use for:
- energy charts
- time-series
- power/energy correlation
- incident trend views
- multi-axis analytics
- heatmaps where justified

Install:
```bash
npm install echarts
```

License:
Apache 2.0.

### 6. TradingView Lightweight Charts
Repository:
https://github.com/tradingview/lightweight-charts

Use selectively for:
- very responsive time-series
- dense telemetry-like chart views
- custom high-performance charting

License:
Apache 2.0.

Prototype choice:
- start with ECharts for broad industrial analytics
- use Lightweight Charts only where its interaction/performance is materially better

---

## Tier 2 — Agent quality

### 7. emilkowalski/skills
https://github.com/emilkowalski/skills

Use:
```bash
npx skills@latest add emilkowalski/skills
```

Purpose:
- animation judgment
- UI craft
- design review
- choosing when NOT to animate

Important:
Do not use animation heavily in industrial control UI. Use the skill as design review guidance.

### 8. ibelick/ui-skills
https://github.com/ibelick/ui-skills

Use:
```bash
npx ui-skills start
```

Purpose:
- selects narrow UI skills for the task
- helps coding agents avoid poor UI patterns

Do not treat it as a runtime dependency.

---

## Tier 3 — Industrial reference / adapter building blocks

### 9. open62541
https://github.com/open62541/open62541

Use for:
- OPC UA client/server integration
- nodes/tags
- industrial data exchange

License:
MPL-2.0, with some CC0 components/examples.

Recommended architecture:
```text
Product X OPC UA Adapter
        |
        +-- open62541
        |
        +-- Canonical Tag Mapper
```

Do not let open62541 types leak throughout the whole Product X domain model.

### 10. Modbus C++ candidate
https://github.com/openenergysolutions/modbus-cpp

Use as a candidate for:
- Modbus TCP master
- prototype register polling

Before production use:
- inspect maintenance activity
- run tests
- verify RTU support requirements
- verify license and dependency posture
- benchmark under your target polling/load pattern

Do not assume any GitHub library is production-ready simply because it is popular.

### 11. Eclipse Mosquitto
https://github.com/eclipse-mosquitto/mosquitto

Use for:
- local MQTT broker
- edge-to-cloud simulation
- event/telemetry fan-out

License:
Eclipse-licensed open source project; inspect project distribution/license files for exact obligations in your release process.

Example:
```bash
docker run --rm -p 1883:1883 eclipse-mosquitto
```

### 12. Eclipse Sparkplug
https://github.com/eclipse-sparkplug/sparkplug

Use for:
- industrial MQTT topic/payload/state conventions
- birth/death state semantics
- standardized industrial MQTT integration

Important:
Sparkplug is a specification/trademarked project, not just “another MQTT broker”.

Use MQTT + Sparkplug as:
```text
Edge telemetry
    -> MQTT
    -> Sparkplug payload/topic/state conventions
    -> cloud/SCADA consumers
```

Do not use MQTT as a substitute for deterministic PLC control.

---

## Tier 4 — Full industrial reference applications

These are useful to study for product ideas, UX and feature decomposition; do not copy branding/UI blindly.

### 13. FUXA
https://github.com/frangoteam/fuxa

Use for:
- SCADA/HMI UX references
- web-based industrial editor patterns
- industrial protocol connection patterns
- historian concepts

It advertises support for Modbus RTU/TCP, OPC-UA, MQTT, Siemens S7 and other integrations.

Important:
FUXA is a reference implementation / source of ideas. Product X should still keep its own canonical model and edge architecture.

### 14. ThingsBoard
https://github.com/thingsboard/thingsboard

Use for:
- asset/device modeling ideas
- dashboard composition
- rule-chain ideas
- alarm concepts
- customer/tenant concepts
- SCADA widget ideas

License:
Apache 2.0 according to the repository.

Important:
Do not clone the entire ThingsBoard architecture into Product X. It is a full platform. Reuse concepts, not unnecessary complexity.

---

## Tier 5 — Performance / data options

### 15. Drogon
https://github.com/drogonframework/drogon

Use for:
- C++ HTTP API
- local edge API
- WebSocket endpoint
- high-performance server-side services

License:
MIT.

Preferred role:
```text
C++ Edge Core
  └── Drogon API/WebSocket layer
```

Do not use Drogon as the HMI rendering engine.

### 16. QuestDB
https://github.com/questdb/questdb

Use for:
- high-ingest telemetry prototype
- time-series queries
- ASOF/window-style telemetry analysis

License:
Apache 2.0.

### 17. InfluxDB
https://github.com/influxdata/influxdb

Use for:
- alternative time-series backend
- telemetry/historian experiments

The current repository describes InfluxDB 3 Core as a time-series/event store built with Apache Arrow, DataFusion and Parquet.

### 18. Polars
https://github.com/pola-rs/polars

Use for:
- offline analytics
- feature engineering
- data transformation
- fast batch/streaming analysis

Do not put Polars inside the millisecond-sensitive live control path.

### 19. Zenoh
https://github.com/eclipse-zenoh/zenoh

Use only as an experiment/benchmark candidate for:
- edge data fabric
- pub/sub
- query/store
- robotics/edge-oriented data distribution

It should not replace industrial field protocols in the first prototype.

### 20. NATS
https://github.com/nats-io/nats-server

Use as an optional internal event bus candidate when:
- you truly need application-level pub/sub/request-reply
- you want a lightweight cloud/edge messaging system

Do not introduce it alongside MQTT + Sparkplug + Zenoh + DDS unless a benchmark/use-case justifies it.

---

## Tier 6 — Future embedded HMI

### 21. Qt Declarative / Qt Quick
https://github.com/qt/qtdeclarative

Use for:
- future embedded/panel HMI
- GPU-backed scene-graph UI
- industrial appliance runtime

Recommended future split:
```text
C++ Edge Core
      |
      +-- Qt Quick HMI
      |
      +-- WebSocket/API
             |
             +-- SvelteKit Designer/Remote Portal
```

Prototype rule:
Build the SvelteKit version first; keep the domain/API portable so Qt Quick can be added later.

---

## What NOT to add immediately

Avoid these in the first prototype unless a benchmark demands them:
- Kafka
- Redpanda
- DDS
- Zenoh
- NATS
- Kubernetes
- multi-service event mesh
- multiple time-series databases
- custom GPU renderer
- custom chart engine
- custom drag/drop graph engine

The stack should feel intentional, not like a technology collection.

---

## Recommended first prototype stack

### Frontend
- SvelteKit
- TypeScript
- shadcn-svelte
- Tailwind
- Lucide
- Svelte Flow
- ECharts

### Edge
- C++20
- Drogon
- in-memory live state
- bounded queues/ring buffers
- simulator
- protocol adapter interfaces

### Industrial adapters
- Modbus candidate
- open62541
- MQTT
- Sparkplug modeling

### Local storage
Choose one for the prototype:
- SQLite for simplest setup, or
- QuestDB / InfluxDB for telemetry-focused demonstration

### Cloud/ML
Mocked interfaces:
- Azure IoT / MQTT gateway
- Azure ML recommendation/anomaly endpoint

---

## Reuse policy

1. Prefer package installation over copying source files manually.
2. Preserve third-party licenses and notices.
3. Record direct dependencies in `THIRD_PARTY_NOTICES.md`.
4. Do not copy proprietary vendor assets.
5. Do not scrape screenshots or proprietary UI.
6. Prefer open-source icons/charts/components.
7. Keep vendor trademarks out of the core UI unless the device is actually selected in a configuration.
8. Do not claim compatibility with a project merely because you imported one library.

---

## Git clone commands

When internet is available:

```bash
git clone --depth 1 https://github.com/emilkowalski/skills.git
git clone --depth 1 https://github.com/ibelick/ui-skills.git
git clone --depth 1 https://github.com/huntabyte/shadcn-svelte.git
git clone --depth 1 https://github.com/xyflow/xyflow.git
git clone --depth 1 https://github.com/open62541/open62541.git
git clone --depth 1 https://github.com/openenergysolutions/modbus-cpp.git
git clone --depth 1 https://github.com/frangoteam/fuxa.git
git clone --depth 1 https://github.com/thingsboard/thingsboard.git
```

For the prototype, Codex should **not** vendor these entire repositories into the app unless there is a concrete need. Prefer package dependencies or small reference reads.
