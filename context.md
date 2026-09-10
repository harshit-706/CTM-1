# Product X — Engineering Context

## 0. Product identity

Product X is a **vendor-neutral industrial asset intelligence and operations layer** for plants that already have PLC/PACs, HMIs, SCADA, VFDs/drives, soft starters, sensors, remote I/O, energy meters and mixed-vendor equipment.

Product X is **not primarily an HMI replacement** and must not claim to replace a PLC, safety system, VFD, SCADA or a vendor firmware-management utility.

Core product thesis:

> **Connect to existing industrial automation infrastructure, normalize heterogeneous asset data, correlate operations + energy + asset lifecycle signals, and turn fragmented telemetry/events into actionable operator/maintenance intelligence.**

The product should support a customer adding a new asset through a validated configuration workflow instead of hand-editing code for every new machine.

---

## 1. Core problem

Industrial plants often have heterogeneous assets and fragmented operational data.

Typical fleet:
- PLC / PAC
- HMI
- SCADA
- VFD / drive
- soft starter
- motors
- compressors
- fans
- heaters
- breakers
- sensors
- remote I/O
- energy meters
- legacy devices
- newer Ethernet/OPC UA-capable devices

Typical pain points:
1. Asset onboarding and device/tag configuration are manual.
2. Data is split across PLCs, VFDs, meters, sensors and SCADA.
3. A fault may create several alarms/events, but operators still have to reconstruct the sequence.
4. Energy numbers are visible but not contextualized against production/process state.
5. Condition degradation can be distributed across several weak signals instead of one alarm.
6. Firmware/configuration/maintenance status can be scattered and difficult to audit.
7. Plants want remote visibility without moving deterministic control into the cloud.

---

## 2. Required product outcomes

### A. Asset onboarding
User can create an asset:
- asset type
- vendor
- model
- protocol
- endpoint/IP
- polling/subscription configuration
- tag mapping
- sensor attachment
- equipment hierarchy/location
- operating constraints
- metadata

The UI must validate whether the chosen connection and mapping are plausible before allowing deployment.

For the prototype, support a defined protocol/device matrix. Do NOT claim universal compatibility.

### B. Common asset model
All devices map into a canonical Product X model.

Canonical concepts:
- Plant
- Area
- Line
- Machine
- Asset
- Device
- Sensor
- Drive
- Meter
- Tag
- Alarm
- Event
- Incident
- Maintenance record
- Firmware package
- Configuration version
- Work order
- Recommendation

Every telemetry sample should carry enough metadata to identify:
- timestamp
- asset_id
- tag_id
- value
- unit
- quality
- source/device
- optional sequence/event correlation id

### C. Live monitoring
Display:
- asset status
- current process variables
- VFD state/speed/load/fault
- motor current/power
- temperature/pressure/flow/vibration where present
- energy and demand
- active alarms
- communications quality
- asset health indicator

Live UI should consume a **live-state channel**, not repeatedly query the historical database.

### D. Operations intelligence
Compute/show:
- OEE
- availability
- performance
- quality
- downtime
- MTBF
- MTTR
- condition monitoring
- asset health
- anomaly detection
- root-cause hypotheses
- alarm management
- event correlation
- incident timelines

Use careful language:
- “likely cause”, “leading hypothesis”, “evidence”
- not unjustified “root cause confirmed”

### E. Energy intelligence
Support:
- kW
- kWh
- power factor
- demand
- energy intensity
- baseline
- load profile
- power quality
- harmonics

Key UX:
- “8.5 kW” by itself is not enough.
- Contextualize with production/process state.
- Example metric: kWh per unit, or equivalent production-normalized energy metric.

### F. Lifecycle intelligence
Track:
- firmware version
- recommended firmware
- configuration version
- configuration checksum/version
- last successful maintenance
- communication status
- diagnostic status
- maintenance due/recommended
- audit trail

Firmware update is **not the novelty claim**.
The novelty is orchestration:
- inventory
- compatibility checks
- dependency awareness
- maintenance-window planning
- pre-check
- staged deployment metadata
- verification
- rollback/abort state
- complete audit trail

The system may integrate with or hand off to vendor-specific firmware tooling rather than reimplementing every vendor update mechanism.

### G. Customer-configurable topology
Provide a visual canvas where the customer can:
- add assets
- connect devices logically
- map protocol endpoints
- map tags
- attach sensors
- define relationships
- view the resulting plant topology

Example:
PLC -> VFD -> Motor -> Pump -> Flow Sensor
Energy Meter -> Electrical Panel
Temperature Sensor -> Bearing
Compressor -> Pressure Sensors

The graph editor must validate:
- duplicate IDs
- invalid source/target types
- duplicate connections where prohibited
- protocol compatibility
- type mismatches
- missing mandatory properties
- cycles where the domain disallows them
- deployment-time conflicts

---

## 3. Architecture

### A. Control plane — deterministic industrial control
Authoritative systems:
- PLC/PAC
- safety PLC/SIS where present
- fieldbus / industrial Ethernet
- VFD/drive control
- remote I/O
- hardwired safety/interlocks

Rules:
- Product X must not become the safety controller.
- Cloud must never be required for basic machine safety or deterministic control.
- Loss of cloud or WAN must not stop local control.
- Product X may observe and, where explicitly authorized, request non-safety control actions through approved interfaces.

### B. Data plane — edge runtime
Preferred prototype:
- C++20
- Drogon for local HTTP/WebSocket API
- protocol-adapter abstraction
- canonical tag model
- event-driven ingestion
- bounded queues/ring buffers where appropriate
- live-state store
- rule engine
- local anomaly/aggregation logic
- local persistence/buffering
- audit/event stream

Conceptual flow:

Field/PLC/VFD/Meter
  -> Protocol Adapter
  -> Canonical Telemetry
  -> Event Engine
  -> Live State Store
  -> WebSocket
  -> HMI / Remote UI

In parallel:

Canonical Telemetry
  -> time-series writer
  -> historian/analytics

And:

Selected telemetry/events
  -> MQTT/Sparkplug
  -> cloud/IoT
  -> analytics/ML

### C. Presentation plane
Production HMI direction:
- Qt Quick / QML candidate for an embedded industrial panel runtime.

Web/customer portal direction:
- SvelteKit
- TypeScript
- accessible component system
- visual topology designer
- remote monitoring
- asset configuration
- audit/history views

For the prototype requested by this prompt, implement the web UI first because it is faster to iterate and easier to demo. Keep APIs and domain models portable so a Qt Quick HMI can later reuse the same edge APIs.

### D. Intelligence plane
Cloud/analytics:
- Azure IoT / messaging as appropriate
- time-series storage
- anomaly detection
- Azure ML for predictive/diagnostic models
- recommendation engine
- fleet analytics

The prototype should run without a real cloud subscription by using a local simulator and mocked cloud/model responses.

---

## 4. Performance architecture

Do not imitate high-frequency trading or spacecraft architecture blindly. Borrow the principles that transfer well:

- event-driven execution
- bounded memory where practical
- low allocation in hot paths
- explicit ownership
- immutable/typed messages where practical
- batching
- backpressure
- separation of hot-path state from historical storage
- metrics around queue depth and processing latency
- deterministic-ish scheduling where useful
- avoid unnecessary serialization
- avoid UI updates for every raw telemetry sample

Important:
A telemetry source may emit at 10 Hz, 100 Hz or more, but the UI does NOT need to render every sample individually.

Use:
- edge ingestion at source rate
- aggregation/windowing as needed
- latest-value state
- UI rendering at a human/display-appropriate frame/update cadence

---

## 5. Protocol strategy

Prototype protocol support:
1. Modbus TCP/RTU
2. OPC UA
3. MQTT
4. Industrial Ethernet conceptually represented; actual protocol implementation may be adapter-specific.

Strong production candidates:
- open62541 for OPC UA
- a well-maintained Modbus implementation for Modbus
- Eclipse Mosquitto for MQTT broker
- Eclipse Sparkplug for stateful MQTT information model/topic/payload conventions

Advanced/future:
- OPC UA PubSub
- OPC UA FX
- TSN
- DDS/RTPS
- Zenoh
- NATS

These are not all required in the first prototype. Benchmark before adding infrastructure.

---

## 6. Storage strategy

Prototype baseline:
- time-series database or local historian with a thin repository interface
- relational database for configuration metadata if needed

Candidates to benchmark:
- TimescaleDB
- InfluxDB 3 Core
- QuestDB

Do not couple the domain layer to one database vendor.
Use repository interfaces so storage can change.

Historical storage must be separate from the live-state path.

---

## 7. UI / UX strategy

Primary visual concept:
**Plant Command Center + Asset Explorer + Incident Timeline + Topology Designer + Energy Lens + Lifecycle Center**

Required screens:
1. Plant Command Center
2. Asset list / asset detail
3. Live telemetry panel
4. Incident timeline
5. Energy analytics
6. Topology/configuration canvas
7. Firmware/configuration lifecycle
8. Alarm/event center
9. Audit log
10. Simulator/control room

A strong demo flow:
- open plant
- select Pump P-104
- show normal live telemetry
- trigger anomaly
- show multi-signal deviation
- show correlated event timeline
- show energy-per-unit degradation
- show lifecycle/config warning
- add a new asset through topology canvas
- deploy validated configuration
- watch the asset appear live

---

## 8. Innovation boundaries

Do NOT pitch these alone as unique:
- “HMI dashboard”
- “IoT dashboard”
- “predictive maintenance”
- “firmware update”
- “energy monitoring”
- “SCADA”
- “digital twin”

Differentiating product idea:
**A unified, vendor-neutral, self-configuring industrial asset intelligence layer that sits above existing automation systems and correlates process, energy, alarms, events and lifecycle state with a validated customer-configurable asset graph.**

---

## 9. Prototype philosophy

The prototype should be:
- runnable locally
- visually impressive but technically credible
- deterministic and testable
- simulator-first
- modular
- explainable
- honest about mocked functionality
- easy to connect to real PLC/VFD/meter adapters later

The demo must never pretend a mock telemetry stream is a real PLC connection.

Clearly label:
- SIMULATED
- MOCKED
- CONNECTED
where applicable.

---

## 10. Example representative assets

Use a small representative virtual plant:
- Pump P-104
- Compressor C-201
- Conveyor M-17
- Electrical Panel EP-01

Each can expose some combination of:
- running/stopped/fault
- speed/frequency
- current
- voltage
- power
- energy
- temperature
- pressure
- flow
- vibration
- health
- alarms
- events
- firmware/configuration

---

## 11. Incident example

Incident:
Pump P-104 trips.

Timeline:
1. process load increased
2. current rose
3. vibration rose
4. flow fell
5. VFD reported overcurrent
6. PLC recorded stop
7. production state changed
8. energy efficiency degraded

Product X should correlate the sequence and show:
- event timeline
- evidence
- related assets
- possible hypotheses
- operator actions
- audit history

---

## 12. Remote-control boundary

Allowed prototype actions:
- start/stop in simulator
- set VFD frequency in simulator
- acknowledge alarm
- create maintenance record
- schedule a simulated firmware/configuration action

Real plant control should require:
- explicit authorization
- command validation
- interlock checks
- audit entry
- timeout/acknowledgement
- clear source-of-command
- never bypass safety logic

---

## 13. Source-derived research notes used for this context

Research checked current GitHub sources for:
- emilkowalski/skills: design-engineering agent skills
- ibelick/ui-skills: routing/selection for UI agent skills
- SvelteKit
- shadcn-svelte
- Svelte Flow / xyflow
- Lucide
- Apache ECharts
- Drogon
- open62541
- Sparkplug
- Mosquitto
- FUXA
- ThingsBoard
- QuestDB
- InfluxDB
- Polars
- Zenoh
- NATS

Important distinction:
The two “skills” repositories are **agent guidance**, not industrial UI component packs. Use them to improve Codex/agent behavior, not as runtime dependencies.
