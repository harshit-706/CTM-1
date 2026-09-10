# CTM-1 — Simulator & Runtime Integration Rework

## Why this file exists

The current CTM-1 prototype has UI pages that can render, but rendering is not proof that the industrial runtime works.

The screenshots show:
- Topology publish failure
- simulator controls that are not yet proven to affect plant state
- runtime claims that exceed what has actually been verified

Do not add more static UI until the runtime contracts are verified.

---

## 1. Correct architecture

CTM-1 has two separate web experiences.

### Operator HMI
Used for:
- plant monitoring
- asset investigation
- alarms/events
- incidents
- energy
- lifecycle
- approved operational commands

### Simulator / Engineering Control Room
Used for:
- discovering virtual assets
- inspecting their simulated parameters
- changing operating inputs
- running scenarios
- injecting faults
- testing communication/sensor failures
- controlling simulation time
- resetting the virtual plant

They must be separate logically and visually.

---

## 2. Runtime flow

### Telemetry path

```text
Virtual Plant Simulator
        ↓
Simulator Data Source / Adapter
        ↓
C++20 Product X Edge Core
        ↓
Canonical Telemetry / Events
        ↓
Live State + Alarm + Correlation + Energy + Lifecycle
        ↓
WebSocket
        ↓
Operator HMI
```

### Simulator control path

```text
Simulator Control UI
        ↓
Simulator API
        ↓
Virtual Device / Process Model
        ↓
Derived Telemetry
        ↓
C++ Edge
```

### Topology/configuration path

```text
Topology Designer
        ↓
C++ Edge
        ↓
validation + registry
        ↓
simulator management API/adapter
        ↓
dynamic virtual asset
        ↓
telemetry
        ↓
HMI
```

The Edge may participate in configuration synchronization with the simulator, but it must not become a fake proxy for every simulator interaction.

---

## 3. Simulator is a virtual plant

The simulator is NOT:
- random numbers
- hard-coded frontend values
- a second fake edge server

It must model correlated state.

Example:

```text
Set motor speed = 50 Hz
        ↓
motor speed changes
        ↓
load changes
        ↓
current changes
        ↓
power changes
        ↓
temperature evolves
        ↓
energy accumulates
        ↓
telemetry reaches Edge
        ↓
HMI changes
```

For a pump:

```text
speed
  ↓
pressure / flow
  ↓
load
  ↓
current
  ↓
power
```

These are configurable prototype relationships, not physical certification models.

---

## 4. Simulator Control Room requirements

The simulator page must be a complete engineering/test page.

### Asset inventory

Show virtual assets dynamically:

```text
P-104   Pump        RUNNING
M-017   Motor       STOPPED
C-201   Compressor  DEGRADED
EP-01   Meter       ONLINE
```

Support:
- search
- filter by type
- filter by status
- select asset

Do NOT hard-code the inventory in the frontend.

---

## 5. Selected asset detail

Show only parameters relevant to the selected asset.

### Identity
- ID
- name
- type
- vendor/model
- source = SIMULATED
- runtime state
- health
- last update

### Electrical
Where supported:
- voltage
- current
- frequency
- power factor
- active power
- accumulated energy

### Mechanical/process
Where supported:
- RPM/frequency
- load
- pressure
- flow
- temperature
- vibration

Every controlled input needs:
- current value
- unit
- min
- max
- step
- validation

Example:

```text
Voltage
380 V ─────────────── 440 V

Frequency
0 Hz ──────────────── 50 Hz
```

These limits are simulator model limits, not claims about real-device safety.

---

## 6. Real controls

For a motor/VFD model:
- START
- STOP
- speed/frequency
- load

For a pump:
- speed
- process demand
- optional restriction/valve condition

For an electrical model:
- voltage where supported
- load
- power factor
- breaker state

Controls must change simulator runtime state.

They must NOT directly change HMI values.

---

## 7. Live simulator telemetry

The simulator control room itself must display live runtime values.

Example:

```text
Pump P-104

RUNNING
42 Hz
12.7 A
6.3 kW
4.8 bar
81 m3/h
46.2 C
2.2 mm/s

Last update: 17:51:42
Source: SIMULATED
Edge: CONNECTED
```

All values must originate from simulator runtime state.

---

## 8. Scenarios

Implement:

- NORMAL
- HIGH_LOAD
- VIBRATION_DRIFT
- PRESSURE_DROP
- ENERGY_WASTE
- OVERCURRENT
- VFD_TRIP
- COMMUNICATION_LOSS
- SENSOR_FAILURE

Each scenario needs:
- start
- stop
- selected target asset(s)
- elapsed time
- reset

A scenario must change underlying telemetry/state.

Do NOT simply set `alarm=true`.

---

## 9. Fault injection

Example:

```text
Asset: Pump P-104
Fault: VFD Overcurrent
Duration: 30 s
Severity: Medium

[ INJECT ]
```

The fault must produce actual consequences:

```text
overcurrent
  ↓
current rises
  ↓
VFD fault
  ↓
machine stops
  ↓
telemetry changes
  ↓
Edge alarm/correlation
  ↓
HMI incident
```

---

## 10. Communication and sensor failures

### Communication loss

When communication loss is injected:
- telemetry must stop or become unavailable
- simulator shows degraded/offline state
- Edge eventually marks data stale/offline
- HMI shows STALE/OFFLINE
- recovery restores telemetry

### Sensor failure

Example:
- vibration quality becomes BAD
- machine may continue running
- analytics should distinguish sensor failure from machine degradation where possible

Never silently swallow the failure.

---

## 11. Simulator API

Implement a real API:

```text
GET  /health
GET  /api/v1/assets
GET  /api/v1/assets/{id}
GET  /api/v1/state
GET  /api/v1/scenarios
POST /api/v1/control
POST /api/v1/scenarios/{id}/start
POST /api/v1/scenarios/{id}/stop
POST /api/v1/faults
POST /api/v1/reset
```

Browser GET requests must return useful JSON.

Do NOT return:

```text
501 Unsupported method ('GET')
```

for normal health/discovery operations.

---

## 12. Simulator-to-Edge contract

Canonical telemetry should contain conceptually:

```text
timestamp
asset_id
tag_id
value
unit
quality
source=SIMULATED
sequence
```

The Edge must:
- validate
- normalize
- update live state
- run rules/engines
- persist historical data where configured
- broadcast WebSocket state/events

The simulator must not create a special UI-only schema.

---

## 13. Connection status

Simulator UI must show:

```text
EDGE CONNECTED
EDGE DISCONNECTED
TELEMETRY DEGRADED
```

Also show:
- last successful transmission
- current error
- reconnect status

There must be no blanket:

```python
except Exception:
    pass
```

on important network paths.

---

## 14. Topology Publish failure must be debugged, not hidden

Current UI shows:

```text
Failed to publish. Is Edge Core running?
```

Do not declare topology complete.

Trace the actual request:

1. URL
2. method
3. port
4. request body
5. response status
6. response body
7. CORS if relevant
8. C++ server log
9. SQLite write
10. WebSocket registry event
11. simulator dynamic asset registration

Verify with both:
- browser Network panel
- direct API call

Do not fix the error by bypassing the Edge with frontend-only state.

---

## 15. Dynamic topology -> simulator

When the user creates:

```text
PLC
 ↓
VFD
 ↓
Motor
 ↓
Pump
 ↓
Pressure Sensor
```

and publishes:

```text
canvas
 ↓
validation
 ↓
configuration version
 ↓
C++ Edge registry
 ↓
simulator management API/adapter
 ↓
virtual assets instantiated
 ↓
telemetry starts
 ↓
HMI updates
```

Do not hard-code the asset in simulator source code.

The simulator must instantiate it from configuration.

---

## 16. Dynamic asset model

Each virtual component type should define:

```text
type
required properties
telemetry template
simulator model
operating inputs
input limits
ports/capabilities
```

Example:

```text
type = motor

inputs:
  running
  frequency_hz
  load_percent

outputs:
  current_a
  power_kw
  temperature_c
  vibration_mm_s
```

The topology engine should not know the internal physics.

---

## 17. Simulator UI layout

Recommended:

```text
┌──────────────┬───────────────────────┬────────────────────┐
│ ASSET        │ LIVE ASSET            │ CONTROL            │
│ INVENTORY    │                       │                    │
│              │     [ASSET VIEW]      │ Speed              │
│ P-104  🟢    │                       │ Load               │
│ M-017  🟢    │ State RUNNING         │ Voltage            │
│ C-201  🟠    │ Power 6.3 kW          │                    │
│ EP-01  🟢    │ Temp 46 C             │ [START] [STOP]     │
│              │ Vibration 2.2         │                    │
│              │                       │ Scenario           │
├──────────────┴───────────────────────┴────────────────────┤
│ EVENTS / TELEMETRY                                       │
│ 17:51:40 P-104 speed 30 -> 42 Hz                       │
│ 17:51:41 current 9.2 -> 12.7 A                         │
└───────────────────────────────────────────────────────────┘
```

The simulator UI should feel like an engineering control room, not a generic form.

---

## 18. Industrial component canvas

Topology is not a generic node diagram.

Use `@xyflow/svelte`.

Palette should include:
- PLC
- PAC
- Motor
- Pump
- Compressor
- Fan
- VFD
- Soft Starter
- Temperature Sensor
- Pressure Sensor
- Flow Sensor
- Vibration Sensor
- Energy Meter
- Breaker
- Remote I/O
- HMI
- SCADA
- Edge Gateway

Where licensing permits, reuse suitable open-source industrial SVG/symbol assets.
Investigate FUXA and other permissively licensed industrial symbol sets.

Do not copy proprietary vendor artwork.

Each node must have:
- visual asset
- stable type
- metadata
- ports
- allowed relationships
- telemetry template
- simulator mapping when appropriate

---

## 19. Feature completion rule

A feature is not complete because its UI exists.

Required chain:

```text
UI action
 ↓
API/command
 ↓
backend/simulator handler
 ↓
domain state change
 ↓
telemetry/event
 ↓
Edge processing
 ↓
WebSocket
 ↓
UI state update
 ↓
visible result
 ↓
test
```

If any important step is missing:

```text
NOT COMPLETE
```

A screenshot is not verification.
A running process is not verification.

---

## 20. Static-data ban

Do not use hard-coded runtime values for functional screens:

```text
42.5 kW
92 health
+38%
1 pending firmware update
static incident timeline
static connection state
```

Unless clearly labelled:

```text
DEMO / STATIC / FIXTURE
```

For functional runtime screens, values must come from:
- simulator
- Edge
- SQLite/runtime state
- engine outputs
- WebSocket

---

## 21. C++ Edge Core

The C++ Edge is authoritative.

Stack:
- C++20
- Drogon
- CMake
- vcpkg
- SQLite
- WebSocket
- REST
- typed canonical model
- bounded queues where appropriate

Do not restore a Python mock Edge.

C++ must genuinely:
- configure
- compile
- start
- expose health/API
- accept telemetry
- maintain live state
- broadcast WebSocket events

---

## 22. Build environment

Known working Windows environment:

```text
MSVC / cl.exe
CMake
Git
vcpkg
Drogon x64-windows
JsonCpp x64-windows
SQLite3 x64-windows
```

vcpkg:

```text
C:\devcpkg
```

toolchain:

```text
C:\devcpkg\scriptsuildsystemscpkg.cmake
```

Do not ask for reinstallation unless verification proves it is broken.

---

## 23. Incident correlation

Do not hard-code the incident page.

The system should generate incidents from actual events.

Example:

```text
load ↑
 ↓
current ↑
 ↓
vibration ↑
 ↓
flow ↓
 ↓
VFD overcurrent
 ↓
machine stopped
 ↓
production impact
```

Show evidence and leading hypotheses.

Do not claim confirmed physical root cause from simple correlation.

---

## 24. Energy intelligence

The simulator and Edge should drive:
- kW
- kWh
- power factor
- demand where modeled
- production rate
- energy intensity
- baseline
- deviation

Example:

```text
Production = 100 units/hour
Baseline = 0.42 kWh/unit
Current = 0.58 kWh/unit
Deviation = +38%
```

Calculate this from runtime data in the functional prototype.

---

## 25. Lifecycle

Firmware is simulated lifecycle orchestration, not real universal flashing.

State machine:

```text
ELIGIBLE
 ↓
PRECHECK
 ↓
SCHEDULED
 ↓
EXECUTING
 ↓
VERIFYING
 ↓
COMPLETED
```

Failure:

```text
EXECUTING
 ↓
FAILED
 ↓
ROLLED_BACK
```

Every transition:
- changes state
- emits event
- creates audit record
- updates UI

---

## 26. Verification

Before claiming end-to-end completion, verify:

```text
C++ configure        PASS/FAIL
C++ build            PASS/FAIL
C++ runtime          PASS/FAIL
Simulator /health    PASS/FAIL
Simulator control    PASS/FAIL
WebSocket            PASS/FAIL
Telemetry pipeline   PASS/FAIL
Alarm                PASS/FAIL
Incident             PASS/FAIL
Energy               PASS/FAIL
Lifecycle            PASS/FAIL
Topology publish     PASS/FAIL
Dynamic asset        PASS/FAIL
Audit                PASS/FAIL
Reconnect            PASS/FAIL
```

Every PASS must include the actual command/action and observed result.

---

## 27. Reporting rule

Never say:
- fully resolved
- end-to-end working
- verified
- done

unless it was actually tested.

Use:

```text
VERIFIED
PARTIALLY VERIFIED
FAILED
NOT IMPLEMENTED
```

with exact evidence.

---

## 28. Priority

Fix in this order:

1. Topology publish/runtime connection
2. C++ Edge as the only authoritative backend
3. Real Virtual Plant Simulator
4. Complete Simulator Control Room
5. Simulator commands -> state changes
6. Simulator -> Edge -> HMI telemetry path
7. Dynamic topology -> simulator registration
8. Remove/labelling of static runtime data
9. Alarm/correlation/energy/lifecycle runtime flows
10. Visual polish

Do not spend time making fake screens prettier while the runtime chain is broken.

---

## 29. Final principle

The goal is not:

```text
make the dashboard look finished
```

The goal is:

```text
make a virtual industrial plant controllable,
observable, testable and connected to the same
Product X runtime architecture that real PLC/VFD/
sensor adapters will use later.
```

When real hardware arrives, the simulator should be replaceable by
real device adapters without rewriting the Product X domain, intelligence
or presentation layers.
