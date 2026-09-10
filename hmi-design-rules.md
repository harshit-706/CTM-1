# HMI Design Rules for Product X (CTM-1)

### A senior-designer rulebook for the agent building the frontend

This document is written to be read by a coding agent before it writes or edits any screen under `apps/web`. It is not a moodboard. Every rule exists because a real industrial HMI failure mode (missed alarm, false confidence, operator fatigue, chartjunk) taught someone this the hard way, mostly ISA-101.01, ISA-18.2 (alarm management) and the High Performance HMI body of practice. Treat this as binding, not inspirational.

Read this alongside `context.md` and `rules.md` before generating any screen. Where they conflict, `context.md`'s product boundaries win; this document governs how it looks and behaves, not what it is allowed to claim.

## 0. The one sentence that governs everything

**An industrial screen's job is to make the true state of the plant impossible to misread, at a glance, under stress, by someone who has been staring at it for eight hours.**

Every rule below specializes that sentence. If a design choice makes the screen prettier but makes the true state one millisecond harder to extract, the design choice is wrong.

## 1. Color is a signal, not a decoration

**1.1 Base palette is low-saturation gray/blue-gray.** Backgrounds, panels, borders, static labels, idle equipment and disconnected states live in a narrow band of desaturated grays. A screen should not look colorful before anything is wrong.

- Use a dark neutral theme for the control-room product.
- Panels are one step lighter than the background, flat, with no decorative floating-card treatment.
- Structural text is muted gray, never pure white. Reserve near-white for live process values and active alarm counts.

**1.2 Color is reserved exclusively for state, and each color means exactly one thing across the app.** Define these as shared CSS variables or theme tokens:

| Token | Meaning | Typical treatment |
| --- | --- | --- |
| `--state-normal` | Running, good, in-spec | Muted green, used sparingly |
| `--state-warning` | Approaching a limit, degraded, advisory | Amber/yellow |
| `--state-alarm` | Out of limit, active alarm | Red |
| `--state-critical` | Safety-relevant, highest-priority active alarm | Red plus motion; priority comes from weight and position |
| `--state-fault` | Comms lost, device fault, stale data | Magenta or violet, distinct from alarm red |
| `--state-inactive` | Stopped or off by design | Neutral gray |
| `--state-simulated` | Simulated data path | Distinct dashed/blue treatment |

If a new concept needs an eighth color, model it with severity and a text label instead.

**Hard rule:** every color-coded state also gets a shape or icon and a text label. A red dot alone is not a valid alarm indicator.

**1.3 No color-only encoding.** Pair state color with an icon and words such as `ALARM` or `FAULT` so the interface remains colorblind-safe.

**1.4 Never use color to represent a continuous quantity as a rainbow gradient.** Do not use decorative heatmaps or traffic-light fills for normal values that have not breached a threshold.

## 2. Typography is a data hierarchy, not a font choice

**2.1 Exactly three text roles apply consistently everywhere:**

1. **Live value:** the changing number, largest, tabular-nums, high contrast.
2. **Label/unit/tag:** static context, small and muted.
3. **Meta:** timestamp, quality flag, source device and comms status, smallest and most muted.

**2.2** Use `font-variant-numeric: tabular-nums` or a monospace numeric font on every live-updating number.

**2.3** Never center-align a live numeric value in a table column. Right-align numbers.

**2.4** Use one typeface family with regular and semibold weights, plus at most one monospace face for numerics.

## 3. Layout follows ISA-101's three-level hierarchy

Do not build one dashboard with everything. Enforce these depths in the route structure:

- **Level 1, Overview:** `/` Plant Command Center. Show roughly 5 to 9 top-level areas or lines, each collapsed to one health tile with status, one KPI and an active-alarm badge. No trend charts or Level-3 density.
- **Level 2, Area/Asset:** `/assets`, `/assets/[id]`. Show live process variables, 2 to 6 bounded trends, the asset alarm list and contributing health factors.
- **Level 3, Diagnostic/Detail:** deep drill-down for raw tags, full trend history, incident timelines, configuration, lifecycle and audit details.

Active alarms are always visible from Level 1 through a persistent alarm banner.

**3.2 Grid, not flexbox soup.** Use a consistent 4/8/12/16/24/32px spacing scale and CSS grid for aligned panels and KPI tiles.

**3.3 Persistent chrome:** every screen retains plant/area breadcrumb, connection quality, active alarm count and current user/mode in stable positions.

## 4. Real-time rendering rules

**4.1** The UI must not re-render per raw sample. Consume the latest, display-appropriate state and cap client-side display updates at roughly 2 to 4 per second per widget.

**4.2** Numeric changes may use a subtle transition of 150ms or less, never layout reflow. Reserve digit width.

**4.3** Trends use bounded fixed windows. Re-render on a throttled interval and prefer incremental chart updates.

**4.4** Connection loss and bad quality are first-class visual states. Desaturate stale values, show `last updated`, and surface the fault state. A frozen-looking live number is unsafe.

## 5. Component-level rules

**5.1 Status chip:** build one reusable component with a fixed set of states: Normal, Warning, Alarm, Fault, Stopped and Simulated. Use an icon or dot plus a text label.

**5.2 Gauges:** prefer thin linear bars with a marked setpoint or limit tick. Avoid 3D, bevels and needles. Radial gauges, if used, are limited to one hero metric per screen.

**5.3 Trends:** use flat 1 to 2px lines, show axis min/max or setpoint and shade only relevant threshold regions.

**5.4 Tables:** use dense monospace numerics, right-aligned numbers, left-aligned text and row-level status chips. Sort by clicking headers. Zebra striping, if used, is neutral and subtle.

**5.5 Topology canvas:** nodes are canonical model objects (Plant, Area, Line, Machine, Asset, Device, Sensor, Drive and Meter). Edges represent actual typed relationships and distinguish power/control, sensor/telemetry and network/protocol links through a persistent legend. Validation errors use red outlines and inline messages on the offending node or edge.

**5.6 Alarm/event list:** order priority first, then asset, description, timestamp and acknowledge action. Default sorting is priority then recency, never alphabetical.

## 6. Alarm philosophy

**6.1** A persistent alarm strip is visible from every screen. It shows count by priority and links directly to the alarm list.

**6.2** Alarms are never silently cleared by changing data. Require explicit acknowledgement and distinguish Unacknowledged-Active, Acknowledged-Active and Cleared.

**6.3** Red and amber are the only alarm-adjacent hues. Do not create an orange tier.

**6.4** Motion is reserved for the single highest-priority unacknowledged condition. Use a slow, low-amplitude pulse around a one-second cycle; everything else is static.

**6.5** Correlated incident copy is hedged: use `likely cause`, `leading hypothesis` and `evidence`, never `root cause confirmed`. Inferred information gets lighter or reduced-emphasis styling than measured values.

## 7. Labeling reality

Every panel, value or asset that is simulated, mocked or connected carries a small, consistent, always-visible badge:

- `SIM`: dashed blue outline
- `MOCK`: dashed gray outline
- No badge: genuinely connected and live only

Silence is honest only when the data is actually live.

## 8. Interaction and command safety

**8.1** Every control action requires confirmation that restates the exact action and target, such as `Stop Pump P-104?`.

**8.2** After a command is sent, show an explicit pending state until observed live state confirms it or a timeout/failure is reported.

**8.3** Every control action produces an inline audit indication referencing the audit trail.

**8.4** Destructive or irreversible actions such as firmware deployment or configuration push require a heavier two-step or type-to-confirm pattern.

## 9. Energy and lifecycle display rules

**9.1** Never show a bare energy number. Pair `kW` with a normalized contextual figure such as `kWh/unit` or percent versus baseline in the same visual unit.

**9.2** Lifecycle status uses the same status-chip vocabulary as live telemetry. Do not invent a second color system.

## 10. Anti-pattern checklist

Reject any screen containing:

- Background gradients, glassmorphism blur or floating-card aesthetics.
- More than three typefaces or active weights.
- A color with different meanings in different components.
- A live number without tabular or monospace digits.
- A pie or donut chart for time-based or KPI data.
- 3D or skeuomorphic gauges.
- More than one blinking element at once.
- An alarm dismissible without explicit acknowledgement.
- A control action without confirmation.
- Simulated data without a `SIM` or `MOCK` badge.
- Decorative rainbow or heatmap gradients.
- Level-3 density on the Level-1 Command Center.
- State encoded only by color.

## 11. Screen-by-screen application

| Screen | Level | Primary emphasis |
| --- | --- | --- |
| Plant Command Center | 1 | Hierarchy discipline, alarm banner, minimal KPI tiles |
| Asset list/detail | 2 | Shared status chips, throttled live values |
| Live telemetry | 2/3 | Tabular numerics, bounded trends, stale state |
| Incident timeline | 3 | Hedged hypotheses, evidence chips, chronological rail |
| Energy analytics | 2/3 | Contextualized metrics, no decorative gradients |
| Topology/configuration | 3 | Typed edges, persistent legend, inline validation |
| Firmware/config lifecycle | 3 | Shared status vocabulary, heavy deploy confirmation |
| Alarm/event center | 2/3 | Priority-first sorting, acknowledge states |
| Audit log | 3 | Dense neutral table, action-type icons |
| Simulator/control room | - | `SIM` badges everywhere and persistent simulator chrome |

## 12. Implementation notes for SvelteKit, Tailwind, shadcn-svelte and Lucide

- Define `--state-*` tokens once in `app.css` or the Tailwind theme. Do not hardcode state colors in components.
- Build shared primitives in `src/lib/components`: `StatusChip`, `LiveValue`, `Sparkline`, `ThresholdBar`, `AlarmBanner` and `SimBadge`.
- Use derived, throttled stores for display rather than subscribing components directly to raw WebSocket messages.
- Use Lucide state icons consistently: warning triangle, critical octagon, comms-loss icon, inactive pause icon and simulated flask icon.
- Respect the Level 1/2/3 route hierarchy. Keep diagnostic density in dedicated detail routes.

## 13. How the agent should use this document

Before generating or editing a component or route under `apps/web`:

1. Identify the screen level and required screen.
2. Pull the relevant rules instead of re-deriving state, typography or color logic.
3. Check the screen against the anti-pattern checklist.
4. If a new visual pattern is needed, extend this document with a justified rule rather than inventing a one-off component pattern.
