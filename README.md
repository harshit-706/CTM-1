# CTM-1 Prototype

A vendor-neutral industrial asset intelligence and operations layer.

Frontend screens under `apps/web` must follow the [HMI design rules](hmi-design-rules.md) alongside the product context and engineering rules.

## Architecture

This prototype consists of three main components running locally:

1. **Web App (`apps/web`)**: A SvelteKit frontend built with TailwindCSS, shadcn-svelte components, and Lucide icons. It serves the Plant Command Center, Topology Designer, Incident Correlation, Energy Analytics, and Lifecycle UI.
2. **Edge Core (`services/edge-core`)**: A C++20 backend. It leverages the Drogon framework to expose a REST API (for telemetry ingestion) and a WebSocket endpoint (for live UI updates). It maintains the canonical live-state of the plant and includes a deterministic rule engine for alarms.
3. **Simulator (`simulator`)**: A Python-based script (`main.py`) that acts as the virtual plant. It simulates process parameters (speed, current, pressure, flow, vibration) for multiple assets and publishes this telemetry to the Edge Core at a configurable tick rate.

### Key Contributions

### Frontend / HMI Development

- Contributed to project ideation and MVP planning, focusing on the frontend and HMI experience.
- Contributed to frontend implementation, including HMI panel features and supporting core tools.
- Contributed to documentation related to the frontend and HMI components.
- Helped shape the frontend portion of the CTM-1 MVP.

## What is real vs simulated

**Real Prototype Functionality:**
- The SvelteKit Web App is fully functional as a frontend shell.
- The C++ Edge Core processes incoming events, updates its in-memory live state, evaluates alarm rules, and broadcasts WebSocket messages.
- The architecture correctly separates the hot-path live telemetry from historical storage and the UI rendering loop.

**Simulated/Mocked Functionality:**
- Hardware Devices: The Pumps and Compressors are simulated by the Python script. They generate deterministic telemetry based on mathematical models, rather than reading physical Modbus/OPC UA registers.
- Advanced ML/Anomaly Models: The anomaly detection and incident correlation engines use threshold-based logic for the prototype instead of ungrounded ML diagnosis.
- Firmware Flashing: The Lifecycle module simulates the state transitions (Precheck, Schedule, Verify) of an update, but does not actually write binaries to physical devices.
- Cloud Integration: Azure IoT or similar cloud connections are bypassed to allow the prototype to run completely locally without subscriptions.

**Future Integrations:**
- Modbus TCP/RTU and OPC UA protocol adapters to replace the Simulator ingestion boundary.
- TimescaleDB or InfluxDB for time-series persistence (currently bypassed to keep the prototype fast and simple).
- Actual remote execution of firmware binaries via vendor-specific APIs.

## How to Run

1. **Start the Web App:**
   ```bash
   cd apps/web
   npm install
   npm run dev
   ```
2. **Start the Edge Core (C++):**
   ```bash
   # Ensure you have a C++20 compiler and CMake installed.
   # Requires Drogon framework to be built/available.
   cd services/edge-core
   mkdir build && cd build
   cmake ..
   cmake --build .
   ./edge-core
   ```
3. **Start the Simulator:**
   ```bash
   cd simulator
   python main.py
   ```
