# Product X — Virtual Plant Simulator / Digital Test Bench

## Purpose

Product X does not currently have access to the full real industrial plant hardware stack (real motor, VFD, PLC, energy meter, pressure/temperature/vibration sensors, etc.).

Therefore the prototype MUST include a **first-class Virtual Plant Simulator**.

The simulator is not a UI-only fake-data generator.

It must behave like a small virtual industrial plant whose generated telemetry and events travel through the same Product X application path used for future real devices.

Core principle:

> **Simulator → industrial-style data source → Product X Edge/Domain pipeline → live state → HMI/Web UI → analytics/events/energy/lifecycle**

The simulator must allow Product X to be tested end-to-end before real hardware is connected.

---

# 1. Architectural relationship

The simulator sits beside the eventual real-device adapters.

```text
                    PRODUCT X

       ┌────────────────────────────────────┐
       │       Real Industrial Devices      │
       │ PLC / PAC / VFD / Meter / Sensors  │
       └─────────────────┬──────────────────┘
                         │
                  Protocol Adapters
                         │
                         ▼
              ┌──────────────────────┐
              │     Edge Core        │
              │ Canonical Model      │
              │ Event Engine         │
              │ Live State           │
              └──────────┬───────────┘
                         │
                 WebSocket / API
                         │
                         ▼
              ┌──────────────────────┐
              │       HMI / Web      │
              └──────────────────────┘


          VIRTUAL / SIMULATED SIDE

       ┌────────────────────────────────────┐
       │       Virtual Plant Simulator      │
       │                                    │
       │ Motors / VFDs / Pumps / Sensors    │
       │ Energy Meter / Compressor / PLC    │
       │ Process model / Fault injection    │
       └─────────────────┬──────────────────┘
                         │
                  Simulator Adapter
                         │
                         ▼
                 SAME EDGE CORE PATH
```

The simulator must NOT bypass the canonical domain model.

---

# 2. Main objective

The simulator must prove:

1. Asset onboarding works.
2. Asset configuration can create a simulated device.
3. Telemetry enters Product X through a realistic adapter boundary.
4. Live state changes.
5. Derived metrics change.
6. Alarms/events are generated.
7. Event correlation produces incidents.
8. Energy analytics respond to process changes.
9. Asset health changes with condition degradation.
10. Lifecycle/configuration states are visible.
11. WebSocket updates reach the UI.
12. Reconnection works.
13. Topology deployment can activate a new simulated asset.
14. Fault injection produces reproducible scenarios.

---

# 3. Simulator is a virtual plant, not random numbers

DO NOT implement:

```text
Math.random() every second
```

as the primary simulator.

Instead use:

```text
state
  +
process relationships
  +
time evolution
  +
scenario rules
  =
telemetry
```

Values should be correlated.

Example:

```text
Motor speed ↑
    ↓
load ↑
    ↓
current ↑
    ↓
power ↑
    ↓
temperature rises over time
```

Another example:

```text
pump restriction ↑
    ↓
flow ↓
    ↓
motor load ↑
    ↓
current ↑
    ↓
power ↑
```

This makes anomaly detection and incident correlation meaningful.

---

# 4. Virtual plant assets

Initial plant:

```text
Plant Demo-01
│
├── Production Line A
│   ├── Conveyor M-17
│   │   └── VFD-17
│   │
│   └── Pump P-104
│       ├── VFD-104
│       ├── Pressure Sensor PS-104
│       ├── Flow Sensor FS-104
│       ├── Temperature Sensor TS-104
│       └── Vibration Sensor VS-104
│
├── Compressor Station
│   └── Compressor C-201
│       ├── Pressure Sensor
│       ├── Temperature Sensor
│       └── Current Sensor
│
└── Electrical Panel EP-01
    ├── Energy Meter EM-01
    └── Breaker BR-01
```

Do not build dozens of assets initially.

Start with 3–4 rich representative assets.

---

# 5. Virtual asset state

Every simulated asset needs internal state.

Example motor:

```text
running
speed_hz
load_percent
current_a
voltage_v
power_kw
energy_kwh
temperature_c
vibration_mm_s
fault_state
health_score
```

Example pump:

```text
running
speed_hz
pressure_bar
flow_m3_h
current_a
power_kw
temperature_c
vibration_mm_s
health_score
```

Example compressor:

```text
running
speed_hz
discharge_pressure_bar
temperature_c
current_a
power_kw
energy_kwh
health_score
```

Example energy meter:

```text
voltage_v
current_a
power_kw
energy_kwh
power_factor
demand_kw
frequency_hz
harmonics
```

---

# 6. Important formula relationships

These are simulation relationships, NOT claims of exact physical plant behavior.

Make them configurable.

### Power

For prototype approximation:

```text
power_kw ≈ voltage_v × current_a × power_factor / 1000
```

For a three-phase simplified model, use an explicit three-phase approximation if desired.

### Energy

```text
energy_kwh += power_kw × dt_hours
```

### Basic motor behavior

```text
speed_hz ↑
    => load tends to ↑
    => current tends to ↑
    => power tends to ↑
```

### Temperature

Temperature should have inertia.

Do not instantly jump temperature with load.

Use a simple first-order model:

```text
temperature_next =
    temperature_current
    + alpha * (temperature_target - temperature_current)
```

### Vibration

Vibration should depend on:
- base vibration
- load
- degradation factor
- transient/fault scenario

### Flow

Pump flow should depend on:
- speed
- restriction/process demand
- optional efficiency factor

### Pressure

Pressure should respond to:
- pump speed
- system resistance
- process demand

These models are for deterministic demonstration, not engineering certification.

---

# 7. Simulator command surface

The simulator must expose commands separately from telemetry.

Example:

```text
POST /simulator/assets/{id}/commands/start
POST /simulator/assets/{id}/commands/stop
POST /simulator/assets/{id}/commands/set-speed
POST /simulator/assets/{id}/commands/reset-fault
POST /simulator/scenarios/{name}/start
POST /simulator/scenarios/reset
```

Commands should produce:
- command event
- state change
- audit record

For the prototype these are simulator commands.

Never pretend they are real PLC commands.

---

# 8. Main simulator controls in UI

Create a dedicated:

# Simulator / Control Room

Screen should show:

```text
SIMULATED PLANT

Asset: Pump P-104

[ RUN ] [ STOP ]

Speed
20 Hz ───────────── 50 Hz
              ●

Process Load
30% ─────────────── 100%
                  ●

Scenario
[ Normal ▼ ]

[ Inject Scenario ]

Telemetry
────────────────────────────
Speed       42 Hz
Current     12.6 A
Power       6.4 kW
Pressure    4.7 bar
Flow        82 m³/h
Temp        46 °C
Vibration   2.1 mm/s
Health      94
```

The values shown here are simulated.

---

# 9. Scenario engine

Implement explicit named scenarios.

Minimum set:

## NORMAL

Stable operation.

## HIGH_LOAD

Increase process load.

Expected effects:

```text
load ↑
current ↑
power ↑
temperature ↑ slowly
```

## VIBRATION_DRIFT

Slow degradation:

```text
vibration ↑ gradually
temperature ↑ gradually
current ↑ slightly
health ↓ gradually
```

## PRESSURE_DROP

```text
pressure ↓
flow ↓
process deviation ↑
```

## ENERGY_WASTE

Production remains approximately stable while:

```text
power ↑
energy/unit ↑
efficiency ↓
```

## OVERCURRENT

```text
current ↑ sharply
VFD event
fault/trip
motor stops
```

## VFD_TRIP

Generate:

```text
VFD fault event
PLC-like stop event
asset state = STOPPED/FAULT
incident
```

## COMMUNICATION_LOSS

Stop telemetry updates from a simulated source for a configurable interval.

Expected UI:

```text
Connection = DEGRADED / OFFLINE
Last update = <timestamp>
```

Product X should not treat stale data as current data.

## SENSOR_FAILURE

Example:
- vibration sensor quality becomes BAD
- or a sensor stops changing
- or sends invalid/out-of-range values

The anomaly engine should distinguish sensor-quality failure from machine degradation where possible.

---

# 10. Fault injection

Fault injection is a first-class feature.

UI:

```text
FAULT INJECTION

Asset:
[ Pump P-104 ]

Fault:
[ VFD Overcurrent ▼ ]

Severity:
[ Medium ▼ ]

Duration:
[ 30 sec ]

[ INJECT ]
```

Fault injection creates deterministic outputs.

It should be possible to reproduce the same scenario repeatedly.

Every injection creates an audit event.

---

# 11. Incident correlation test

Create a deterministic example:

```text
Incident: INC-1024
Asset: Pump P-104
```

Timeline:

```text
10:42:00
Process load ↑

10:42:04
Current deviation ↑

10:42:06
Vibration deviation ↑

10:42:08
Flow ↓

10:42:10
VFD overcurrent

10:42:10
Pump stopped

10:42:12
Production impact detected
```

The correlation engine should build the timeline automatically from generated events.

Do not manually hard-code the UI incident timeline separately from events.

---

# 12. Energy intelligence test

Simulator must support production context.

Example:

```text
Production:
100 units/hour

Power:
8 kW

Energy intensity:
0.42 kWh/unit
```

Then ENERGY_WASTE:

```text
Production:
approximately 100 units/hour

Power:
11 kW

Energy intensity:
0.57 kWh/unit
```

UI should show:

```text
Energy intensity
+35.7% vs baseline
```

The baseline must be represented as data/configuration.

---

# 13. Asset health

For prototype:

```text
health_score: 0..100
```

Can be derived from weighted deviations.

Example conceptual model:

```text
health =
100
- vibration_penalty
- temperature_penalty
- current_penalty
- process_penalty
```

Make weights configurable.

Display contributing evidence.

Do not create a mysterious AI score with no explanation.

---

# 14. Anomaly engine integration

Simulator should NOT directly tell the anomaly engine:

```text
ANOMALY = TRUE
```

Instead it emits telemetry.

The anomaly engine must independently determine:

```text
normal
or
deviation
```

This is critical because it tests the real Product X pipeline.

Scenario metadata may exist for debugging, but production-style analytics should consume telemetry/events rather than scenario labels.

---

# 15. WebSocket behavior

Simulator -> Edge -> WebSocket -> UI.

The UI should receive messages such as:

```json
{
  "type": "telemetry.update",
  "timestamp": 1710000000000,
  "payload": {
    "assetId": "P-104",
    "tag": "motor.current",
    "value": 12.6,
    "unit": "A",
    "quality": "GOOD",
    "source": "SIMULATED"
  }
}
```

All simulated telemetry should identify:

```text
source = SIMULATED
```

The UI must make that visible.

---

# 16. Dynamic asset creation

This is an important Product X differentiator.

User:

```text
Topology Designer
    ↓
+ Add Asset
    ↓
Pump
    ↓
Vendor/Model
    ↓
Protocol
    ↓
Tags/Sensors
    ↓
Validate
    ↓
Publish
    ↓
Deploy to Simulator
```

After deployment:

```text
Pump P-105
SOURCE = SIMULATED
STATUS = ONLINE
TELEMETRY = ACTIVE
```

The simulator should create its internal model dynamically from the published asset configuration.

Do not hard-code P-105 into simulator code.

---

# 17. Simulator state machine

Every simulated device should have explicit states:

```text
OFFLINE
STARTING
RUNNING
DEGRADED
FAULT
STOPPING
STOPPED
```

Transitions must be explicit.

Example:

```text
STOPPED
  -> STARTING
  -> RUNNING
  -> DEGRADED
  -> FAULT
  -> STOPPING
  -> STOPPED
```

Invalid transitions should be rejected.

---

# 18. Time model

Use a deterministic simulation clock abstraction.

Do not sprinkle raw wall-clock calls throughout simulation logic.

Prefer:

```text
ISimulationClock
```

with:
- current time
- delta time
- pause
- resume
- speed multiplier

Useful controls:

```text
Simulation speed:
0.5x
1x
2x
10x
```

This lets testers accelerate slow degradation scenarios.

---

# 19. Sampling

Simulator supports configurable rates:

```text
1 Hz
5 Hz
10 Hz
50 Hz
100 Hz
```

Start prototype at 1–10 Hz.

Higher rates are for performance testing only.

Remember:
high telemetry rate != high UI render rate.

---

# 20. Performance test mode

Add a test mode:

```text
Assets: 100
Tags/asset: 20
Rate: 10 Hz
```

Then optionally:

```text
Assets: 1000
Tags/asset: 20
Rate: 10 Hz
```

Measure:

```text
ingestion p50/p95/p99
processing p50/p95/p99
WebSocket delivery p50/p95/p99
queue depth
dropped/coalesced updates
CPU
memory
```

Do not make product capacity claims from these tests.

Record:
- machine CPU/RAM
- build type
- test duration
- asset count
- tag count
- source rate

---

# 21. Backpressure behavior

Simulator must be able to intentionally generate more data than the UI should render.

Expected architecture:

```text
high-rate telemetry
       ↓
bounded queue
       ↓
aggregation/coalescing
       ↓
live-state store
       ↓
controlled UI update rate
```

Do NOT:
- grow an unbounded queue
- render every sample
- freeze browser UI

When overloaded, apply explicit policies such as:
- latest-value replacement for gauge-like tags
- window aggregation for charts
- preserve discrete alarms/events
- expose overload metrics

---

# 22. Simulator UI vs HMI

Simulator controls should be clearly separated from operator HMI.

Recommended:

```text
Operator UI
  -> Plant monitoring
  -> incidents
  -> energy
  -> assets

Engineer / Demo UI
  -> Simulator
  -> fault injection
  -> scenario control
  -> time acceleration
```

Never put a giant “fake plant” control panel on the main operator screen.

---

# 23. Real vs simulated source abstraction

Create:

```cpp
class IDataSource {
public:
    virtual ~IDataSource() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::string sourceName() const = 0;
};
```

Possible implementations:

```text
SimulatorDataSource
ModbusDataSource
OpcUaDataSource
MqttDataSource
```

All feed the same canonical telemetry layer.

This is the most important architectural requirement.

---

# 24. Simulator config file

Support a declarative configuration file.

Example:

```yaml
plant:
  id: demo-01
  name: Demo Plant

assets:
  - id: P-104
    type: pump
    name: Pump P-104
    model: virtual-pump
    sampling_hz: 10
    tags:
      - motor.speed
      - motor.current
      - motor.power
      - process.pressure
      - process.flow
      - temperature
      - vibration
```

The schema can evolve.

Do not make scenario logic depend on UI component code.

---

# 25. Reset

Provide:

```text
RESET SIMULATION
```

This returns:
- state
- faults
- incidents
- energy counters if configured
- scenario
- clock

to a known baseline.

For audit/history, decide explicitly whether reset:
- creates a new demo session, or
- clears only simulator state.

Do not silently erase audit history in a production-oriented mode.

---

# 26. Seed/demo presets

Provide at least:

### Preset A — Normal Plant

All assets healthy.

### Preset B — Failing Pump

P-104 degradation.

### Preset C — Energy Waste

Production stable; energy intensity rises.

### Preset D — VFD Trip

Overcurrent -> trip -> production impact.

### Preset E — Communication Loss

One virtual device disappears temporarily.

---

# 27. Developer API

Expose read-only simulator state:

```text
GET /simulator/state
GET /simulator/assets
GET /simulator/scenarios
GET /simulator/events
```

Expose commands:

```text
POST /simulator/reset
POST /simulator/scenarios/{id}/start
POST /simulator/scenarios/{id}/stop
POST /simulator/assets/{id}/start
POST /simulator/assets/{id}/stop
POST /simulator/assets/{id}/fault
```

All command endpoints return structured results.

---

# 28. Testing requirements

Unit tests:
- motor model
- pump model
- compressor model
- energy calculation
- temperature inertia
- anomaly feature calculation
- scenario transitions
- asset state transitions
- fault injection
- time advancement
- dynamic asset creation

Integration tests:
- simulator -> edge
- edge -> live state
- edge -> WebSocket
- topology publish -> simulator
- simulator event -> incident correlation
- simulator telemetry -> energy analytics
- WebSocket reconnect -> state recovery

---

# 29. What the simulator must prove in the final demo

The final demo should show this exact causal chain:

```text
User changes simulated process condition
             ↓
Virtual asset state changes
             ↓
Telemetry changes
             ↓
Edge receives telemetry
             ↓
Live state updates
             ↓
HMI updates
             ↓
Derived metrics change
             ↓
Anomaly detected
             ↓
Alarm/event generated
             ↓
Incident correlated
             ↓
Energy/context impact shown
             ↓
Operator sees action/recommendation
```

This is the core reason the simulator exists.

---

# 30. Critical implementation rules

1. Simulator is first-class architecture.
2. Simulator must share the same canonical telemetry/event model as real adapters.
3. Simulator must not bypass edge processing.
4. Simulator values must be correlated, not random.
5. Scenarios must be deterministic/reproducible.
6. Fault injection must be explicit.
7. Every simulated value is visibly marked SIMULATED.
8. Simulator must support dynamic asset creation.
9. Simulator must support time acceleration.
10. Simulator must support communication-loss tests.
11. Simulator must support sensor-quality failure.
12. Simulator must generate real Product X events.
13. Incident correlation must consume those generated events.
14. Energy analytics must consume simulator telemetry.
15. Lifecycle views may consume simulated firmware/configuration metadata.
16. Do not implement real firmware flashing.
17. Do not claim simulator equals physical validation.
18. Use simulator for software integration, workflow, analytics and UI testing.
19. Real hardware validation remains a later stage.
20. Keep simulator code replaceable by real protocol adapters.

---

# 31. Definition of done

The simulator is complete when:

- [ ] 4 representative virtual assets run
- [ ] correlated telemetry is generated
- [ ] start/stop/speed commands work
- [ ] scenarios work
- [ ] faults work
- [ ] communication loss works
- [ ] sensor failure works
- [ ] time acceleration works
- [ ] dynamic asset creation works
- [ ] telemetry enters the same canonical edge pipeline
- [ ] live UI updates
- [ ] anomaly engine reacts
- [ ] event correlation reacts
- [ ] incident timeline appears
- [ ] energy analytics react
- [ ] audit events exist
- [ ] simulator source is visible in UI
- [ ] integration tests pass
- [ ] performance test can run
- [ ] reset returns to known state

---

# 32. Final product principle

The simulator is not a replacement for the plant.

It is the **software verification environment for Product X**.

The architectural goal is:

```text
                SAME PRODUCT LOGIC

Real Devices ───────┐
                    ├──> Canonical Edge/Data Model
Virtual Devices ────┘
                              ↓
                         Product X
                              ↓
                 HMI / Operations / Energy
                 Incidents / Lifecycle / Audit
```

That means when real motors, PLCs, VFDs and meters become available later, the Product X application should require **a new data-source adapter/configuration, not a rewrite of the intelligence/UI layer.**
