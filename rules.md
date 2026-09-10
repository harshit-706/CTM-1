# Product X — Non-Negotiable Engineering Rules

## A. Product boundary
1. Product X is an industrial asset intelligence/operations layer.
2. Do not market it as merely an HMI dashboard.
3. Do not replace PLC/PAC logic in the prototype.
4. Do not replace safety controllers.
5. Do not make cloud availability a prerequisite for local control.
6. Do not claim universal vendor compatibility.
7. Define and display a supported protocol/device matrix.
8. Treat vendor-specific firmware mechanisms as integrations, not as reinvented universal firmware flashing.
9. Treat predictive maintenance output as a hypothesis/recommendation unless validated.
10. Never invent industrial readings in a “connected” state.

## B. Core domain
11. Everything important is modeled as an Asset, Device, Sensor, Tag, Alarm, Event, Incident, Recommendation, FirmwarePackage, ConfigurationVersion or MaintenanceRecord.
12. Asset IDs are unique.
13. Tag IDs are unique within the required namespace.
14. Device identity is separate from asset identity.
15. A physical asset can have multiple devices.
16. A device can expose multiple tags.
17. Tag metadata must include unit and quality semantics.
18. Timestamps must be explicit.
19. Quality state must be preserved.
20. Source/provenance must be preserved where possible.
21. Configuration versions are immutable once published.
22. Changes create audit records.
23. Do not store raw telemetry as an untyped bag of JSON in the hot path.
24. Use typed domain objects.
25. Keep internal IDs stable across UI reorderings.

## C. Control / safety
26. PLC/PAC remains authoritative for deterministic control.
27. Safety interlocks remain authoritative in the safety/control layer.
28. Cloud must never be required for E-stop or basic safe shutdown.
29. Product X must tolerate WAN loss.
30. Edge runtime must continue local observation during cloud outage.
31. Remote commands require explicit authorization.
32. Remote commands are audited.
33. Remote commands include source, target, command, timestamp and result.
34. Simulation commands must be clearly marked as simulation.
35. Never hide the difference between simulation and live plant.

## D. Edge runtime
36. Prefer C++20 for the edge core prototype.
37. Keep protocol adapters behind interfaces.
38. Keep the canonical model independent of protocol libraries.
39. Keep storage APIs behind repository interfaces.
40. Keep UI APIs behind transport interfaces.
41. Use event-driven ingestion.
42. Avoid unbounded queues.
43. Prefer bounded queues for telemetry hot paths.
44. Measure queue depth.
45. Measure ingestion latency.
46. Measure processing latency.
47. Measure WebSocket delivery latency.
48. Minimize allocation in hot paths.
49. Avoid unnecessary copies.
50. Do not claim “zero-copy” unless measured and demonstrated.
51. Batch where it reduces overhead without increasing operational latency unacceptably.
52. Apply backpressure.
53. Define drop/aggregation policy for overloaded telemetry.
54. Never silently discard safety-critical events.
55. Hot path and historical path must be separate.
56. Do not make UI rendering responsible for telemetry storage.

## E. Live state
57. Maintain a latest-value/live-state representation.
58. Live HMI reads live state or subscribes to a live channel.
59. Do not poll historical storage for every live widget update.
60. WebSocket is the preferred prototype live-data channel.
61. UI update cadence is independent from sensor sampling cadence.
62. A 100 Hz sensor does not force 100 UI renders per second.
63. Coalesce redundant UI updates where appropriate.
64. Preserve event ordering where required.
65. Preserve timestamps from source when available.

## F. Protocols
66. Modbus TCP/RTU is an adapter, not the canonical model.
67. OPC UA is an adapter, not the canonical model.
68. MQTT is for decoupled telemetry/event messaging, not deterministic PLC control.
69. Sparkplug may be used above MQTT for stateful industrial semantics.
70. Never send browser UI commands directly to a PLC without an authenticated/validated server-side boundary.
71. Keep protocol-specific register/node/address details out of core UI components.
72. Document byte order, scaling and units for Modbus mappings.
73. Document OPC UA node identity and namespace.
74. Document MQTT topic/schema.
75. Keep protocol connection status visible.

## G. Asset configuration
76. “Add Asset” is the primary onboarding action.
77. Do not hard-code a single machine type.
78. Support asset templates.
79. Templates contain required and optional tags.
80. Templates define validation constraints.
81. Templates define protocol capabilities.
82. Invalid connections are visually blocked.
83. Invalid connections explain why they are invalid.
84. Duplicate IDs are blocked.
85. Missing required properties are blocked.
86. Unsupported protocol/device combinations are blocked.
87. Deployment requires a valid configuration graph.
88. Configuration validation must be deterministic.
89. Dry-run validation must exist.
90. Published configuration must be versioned.
91. Rollback must be possible at configuration level.
92. Never auto-deploy a dangerous configuration.

## H. Topology canvas
93. Use Svelte Flow/xyflow for the prototype graph editor unless a better benchmarked option is identified.
94. Nodes represent domain entities, not arbitrary UI boxes.
95. Edges represent typed relationships.
96. Connection rules are declared in domain metadata.
97. Visual edges have protocol/status metadata where applicable.
98. The canvas supports pan/zoom.
99. The canvas supports selection.
100. The canvas supports inspector panels.
101. The canvas supports validation indicators.
102. The canvas supports save/load of topology JSON.
103. The canvas supports undo/redo where practical.
104. Do not let the graph become an unvalidated free-for-all.
105. Keep graph state serializable.
106. UI graph IDs must map to stable domain IDs.

## I. Operations intelligence
107. OEE must use explicit availability/performance/quality definitions.
108. Downtime reasons must be modeled.
109. MTBF must define failure-event semantics.
110. MTTR must define repair-start/repair-complete semantics.
111. Condition monitoring uses multiple signals when available.
112. Anomaly detection produces evidence.
113. The prototype should show feature deviations that led to an anomaly.
114. Root cause is ranked as a hypothesis unless validated.
115. Alarm management must distinguish active, acknowledged, cleared.
116. Alarm priority/severity must be explicit.
117. Event correlation must include a time window.
118. Correlation should show contributing events.
119. Incidents should have a lifecycle.
120. Recommendations should record reason/evidence.
121. Human approval is required for operationally consequential recommendations.

## J. Energy
122. kW is instantaneous power.
123. kWh is accumulated energy.
124. Demand is explicitly modeled as a measurement/window concept.
125. Energy intensity is production/process-normalized.
126. Baselines must include time/context assumptions.
127. Load profiles are time series, not single values.
128. Power factor is distinct from active power.
129. Power quality is broader than energy consumption.
130. Harmonics must be represented as distortion/measurement information, not generic “bad power”.
131. Show energy with production/process context.
132. Avoid “AI says save energy” without evidence.

## K. Lifecycle
133. Track firmware installed version.
134. Track recommended version where available.
135. Track configuration version.
136. Track last successful maintenance.
137. Track update eligibility.
138. Track compatibility/pre-check result.
139. Track maintenance window.
140. Track update status.
141. Track verification result.
142. Track rollback/abort result when supported.
143. Firmware actions are audited.
144. Do not pretend Product X can update every vendor device.
145. Vendor tools may be invoked/integrated rather than reimplemented.
146. Security-sensitive updates require appropriate authentication and authorization.

## L. Auditability
147. Every config mutation produces an audit record.
148. Every remote command produces an audit record.
149. Every firmware lifecycle action produces an audit record.
150. Audit record includes actor/source, time, action, target, before/after where appropriate.
151. Audit log is append-oriented.
152. Audit views support filtering.
153. Do not expose secrets in audit logs.
154. Do not log passwords/tokens.

## M. UI architecture
155. SvelteKit is the preferred web application framework.
156. Use TypeScript.
157. Use accessible reusable components.
158. Prefer shadcn-svelte for base components.
159. Use Lucide for icons.
160. Use Svelte Flow for topology.
161. Use Apache ECharts or Lightweight Charts where suited to telemetry visualization.
162. Avoid 20 tiny chart libraries.
163. Avoid generic admin-dashboard styling.
164. Industrial UI must emphasize state, causality and action.
165. Use clear severity and quality states.
166. Use dark theme carefully; optimize legibility.
167. Do not use motion as decoration.
168. Animation should explain state changes.
169. Avoid excessive gradients/glows/neon “AI” styling.
170. Keep critical values visually stable.
171. Use skeleton/loading states only where useful.
172. Make live vs stale data obvious.
173. Show “last updated” timestamps.
174. Show connection quality/status.

## N. Performance UI
175. Do not rerender the whole page on every telemetry event.
176. Use local state stores/selectors.
177. Coalesce updates where possible.
178. Virtualize long asset lists.
179. Downsample long time-series for display.
180. Keep charts interactive without loading millions of points at once.
181. Avoid sending historical time-series over the live channel.
182. Use binary/typed payloads for high-rate internal transport only when measured to help.
183. JSON is acceptable for configuration and low-rate APIs.
184. Do not add a binary protocol just because it sounds fast.

## O. Backend API
185. Separate command APIs from telemetry subscription APIs.
186. Use explicit schemas.
187. Validate all inputs.
188. Return structured errors.
189. Include correlation IDs for command/incident workflows.
190. Do not expose internal implementation details to the browser.
191. API versioning should be considered from the start.
192. WebSocket messages have a stable envelope/schema.
193. Connection lifecycle is explicit.
194. Heartbeats/timeouts are explicit.
195. Stale connections are cleaned up.

## P. Storage
196. Time-series storage is not the source of truth for live control.
197. Metadata/configuration may use relational storage.
198. Telemetry schema should be optimized for time.
199. Repository interfaces isolate vendor choice.
200. Prototype may use SQLite/local files if this accelerates delivery.
201. Benchmark TimescaleDB/InfluxDB/QuestDB before scale claims.
202. Never claim a specific million-tags capacity without a benchmark.
203. Keep historical retention configurable.
204. Preserve raw/aggregated distinction where needed.

## Q. Cloud
205. Cloud is for fleet analytics, remote monitoring, centralized policy and ML.
206. Edge should keep running without cloud.
207. Cloud commands must be treated as untrusted input until validated locally.
208. Cloud telemetry may be sampled/aggregated.
209. Do not uplink every raw high-rate sample by default.
210. Use MQTT/Sparkplug where it fits.
211. Do not add Kafka/Redpanda unless there is a demonstrated scale need.
212. Azure ML integration can be mocked behind an interface for the prototype.

## R. Testing
213. Unit-test canonical model.
214. Unit-test graph validation.
215. Unit-test protocol parsing with fixtures.
216. Unit-test anomaly feature calculations.
217. Unit-test energy calculations.
218. Unit-test incident correlation.
219. Unit-test audit events.
220. Add simulator tests.
221. Add WebSocket contract tests.
222. Add load tests for telemetry ingestion.
223. Measure p50/p95/p99 for key paths.
224. Document hardware used for benchmarks.
225. Do not confuse benchmark environment with plant performance.

## S. Simulator
226. Simulator must be first-class.
227. Simulator produces deterministic scenarios.
228. Scenario controls include normal, overload, vibration drift, overcurrent, pressure excursion, energy waste, communication loss.
229. Simulator supports fault injection.
230. Simulator can emit correlated event sequences.
231. Simulator can change asset health over time.
232. Simulator can toggle firmware/config status.
233. Simulator supports configurable sample rates.
234. Simulator labels all data as SIMULATED.
235. Simulator can reset state.

## T. Demo
236. Demo must show the product story, not every technology.
237. Recommended sequence:
    - plant overview
    - asset drill-down
    - anomaly
    - incident correlation
    - energy context
    - lifecycle/config status
    - topology onboarding
    - validation
    - simulated deployment
238. Every major screen should answer “what happened?”, “why does it matter?”, “what can I do?”
239. Avoid a wall of charts.
240. Avoid showing raw registers unless in a diagnostic view.

## U. Technology selection
241. emilkowalski/skills is agent guidance, not runtime UI.
242. ibelick/ui-skills is agent guidance, not runtime UI.
243. Shadcn-svelte is a reusable component source.
244. Svelte Flow is a graph editor source.
245. Lucide is an icon source.
246. ECharts/Lightweight Charts are charting candidates.
247. FUXA is a useful industrial UX/reference project.
248. ThingsBoard is a useful asset/dashboard/rule-engine reference project.
249. open62541 is a serious OPC UA implementation candidate.
250. Mosquitto is a serious MQTT broker candidate.
251. Sparkplug is a serious industrial MQTT information-model/specification candidate.
252. Drogon is a C++ HTTP/WebSocket edge API candidate.
253. Qt Quick is a strong future embedded HMI candidate.
254. Zenoh/DDS/NATS are optional experimental data-bus candidates.
255. Skia/Dear ImGui are not the default customer HMI choices.
256. Benchmark before introducing additional buses/storage systems.

## V. Security
257. Never embed secrets in the frontend.
258. Never hard-code production passwords.
259. Separate authentication and authorization.
260. Principle of least privilege.
261. Commands need authorization.
262. Configuration deployment needs authorization.
263. Firmware actions need stronger authorization than read-only telemetry.
264. TLS should be supported for networked production interfaces.
265. Certificates/secrets belong in secure configuration/secret stores.
266. Do not expose PLC networks directly to the public internet.
267. Edge should sit behind appropriate network segmentation.
268. Log security events.

## W. Code quality
269. Prefer small modules.
270. Keep domain logic framework-agnostic.
271. Avoid giant components.
272. Avoid giant global stores.
273. Avoid hidden side effects.
274. Use strict TypeScript.
275. Use clang-format for C++.
276. Use clang-tidy where practical.
277. Fail fast on invalid configuration.
278. Use structured logging.
279. Use health endpoints.
280. Add configuration examples.
281. Document local setup.
282. Keep secrets out of git.
283. Keep dependency versions pinned/lockfile-controlled.
284. Record license/attribution for third-party assets.

## X. “Do not overengineer the prototype”
285. One edge service is enough.
286. One web app is enough.
287. One simulator is enough.
288. One local datastore is enough.
289. One message broker is enough if needed.
290. Do not deploy Kubernetes just to demo four virtual machines.
291. Do not deploy Kafka/Redpanda just to say “streaming”.
292. Do not introduce microservices before a concrete boundary is needed.
293. Do not introduce DDS + Zenoh + NATS + MQTT together in the first demo.
294. Keep interfaces extensible, implementations small.
295. Prefer working end-to-end over broad but fake feature coverage.

## Y. Completion criteria
296. `npm run check` or equivalent passes.
297. Frontend builds.
298. Backend builds.
299. Simulator runs.
300. UI connects to simulator.
301. Live telemetry updates.
302. Alarm/event flow works.
303. Incident timeline works.
304. Energy KPI calculations work on simulator data.
305. Asset onboarding works.
306. Topology validation works.
307. Audit log works.
308. Lifecycle screen works with simulated firmware/config state.
309. README explains exactly what is real vs simulated.
310. No unsafe claims are presented as real-world control guarantees.
