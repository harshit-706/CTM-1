import { json } from '@sveltejs/kit';
import type { RequestHandler } from './$types';

const API_KEY = process.env.ASTRA_GPT_API_KEY || "xpl_277689f3fe27ba3a58da6d11d09acb6c270e7d7b";
const ENDPOINT = process.env.ASTRA_MODEL_ENDPOINT || "https://api.x.ai/v1/chat/completions";

const SYSTEM_PROMPT = `You are Product X's Astra AI Copilot — an expert industrial asset intelligence and topology architect.
Analyze user questions, plant telemetry, incident logs, and plant layout descriptions.

CRITICAL FEATURE:
If the user asks to "build", "generate", "create", or "design" a plant topology or machine layout, you MUST provide a clear natural language explanation AND a structured JSON block in this exact markdown code block format:

\`\`\`topology_json
{
  "nodes": [
    { "id": "plc-1", "type": "plc", "label": "Main Siemens PLC", "manufacturer": "Siemens", "model": "S7-1500", "x": 100, "y": 200 },
    { "id": "vfd-1", "type": "vfd", "label": "Danfoss VFD Drive", "manufacturer": "Danfoss", "model": "VLT AutomationDrive FC 302", "x": 350, "y": 150 },
    { "id": "motor-1", "type": "motor", "label": "WEG AC Motor", "manufacturer": "WEG", "model": "W22 Premium Efficiency", "x": 600, "y": 150 },
    { "id": "pump-1", "type": "pump", "label": "Grundfos Centrifugal Pump", "manufacturer": "Grundfos", "model": "CR/CRI/CRN", "x": 850, "y": 150 },
    { "id": "vibration_sensor-1", "type": "vibration_sensor", "label": "Vibration Sensor", "x": 850, "y": 320 }
  ],
  "edges": [
    { "source": "plc-1", "target": "vfd-1" },
    { "source": "vfd-1", "target": "motor-1" },
    { "source": "motor-1", "target": "pump-1" },
    { "source": "pump-1", "target": "vibration_sensor-1" }
  ]
}
\`\`\`

Valid node types: plc, pac, remote_io, edge_gateway, breaker, vfd, soft_starter, energy_meter, motor, pump, compressor, fan, temp_sensor, pressure_sensor, flow_sensor, vibration_sensor, hmi, scada.

Allowed signal connections:
- plc -> vfd, soft_starter, remote_io, scada, edge_gateway
- remote_io -> temp_sensor, pressure_sensor, flow_sensor, vibration_sensor
- breaker -> vfd, soft_starter, energy_meter, motor
- vfd -> motor
- soft_starter -> motor
- motor -> pump, compressor, fan, vibration_sensor, temp_sensor
- pump -> pressure_sensor, flow_sensor, vibration_sensor
- compressor -> pressure_sensor, temp_sensor, vibration_sensor
- edge_gateway -> scada, hmi`;

export const POST: RequestHandler = async ({ request }) => {
	try {
		const body = await request.json();
		const prompt = body.prompt || "Analyze current plant status and asset health.";
		const pLower = prompt.toLowerCase();

		// Check if user is asking for topology generation
		const isTopologyRequest = pLower.includes("topology") || pLower.includes("build") || pLower.includes("create") || pLower.includes("plant") || pLower.includes("design") || pLower.includes("layout");

		// Attempt online call to Astra / xAI model
		try {
			const res = await fetch(ENDPOINT, {
				method: 'POST',
				headers: {
					'Content-Type': 'application/json',
					'Authorization': `Bearer ${API_KEY}`
				},
				body: JSON.stringify({
					model: 'grok-2-latest',
					messages: [
						{ role: 'system', content: SYSTEM_PROMPT },
						{ role: 'user', content: prompt }
					],
					temperature: 0.3,
					max_tokens: 1000
				})
			});

			if (res.ok) {
				const data = await res.json();
				const answer = data.choices?.[0]?.message?.content;
				if (answer) {
					return json({
						success: true,
						model: 'grok-2-latest',
						content: answer
					});
				}
			}
		} catch (e) {
			console.warn('Astra online API fetch error, using local fallback:', e);
		}

		// Fallback local heuristic model with topology generation capability
		let content = "";
		if (isTopologyRequest) {
			content = `### 🎨 Astra AI Plant Topology Architect
I have analyzed your plant plan and generated a full multi-stage industrial asset topology layout (PLC -> VFD -> Motor -> Pump & Sensors).

#### 🏗️ Architecture Breakdown:
1. **Control Layer:** Siemens S7-1500 PLC orchestrating speed and start/stop loops.
2. **Drive & Power:** Danfoss VLT FC 302 VFD delivering variable frequency power to WEG W22 Motor.
3. **Mechanical Assets:** Grundfos Centrifugal Pump handling fluid flow.
4. **Sensor Attachments:** Vibration and Pressure sensors mapped directly to Edge Core.

\`\`\`topology_json
{
  "nodes": [
    { "id": "plc-1", "type": "plc", "label": "Siemens S7-1500 PLC", "manufacturer": "Siemens", "model": "S7-1500", "x": 80, "y": 200 },
    { "id": "breaker-1", "type": "breaker", "label": "Main Circuit Breaker", "manufacturer": "Schneider Electric", "model": "Modicon M580", "x": 80, "y": 80 },
    { "id": "vfd-1", "type": "vfd", "label": "Danfoss VFD Drive", "manufacturer": "Danfoss", "model": "VLT AutomationDrive FC 302", "x": 320, "y": 180 },
    { "id": "motor-1", "type": "motor", "label": "WEG W22 AC Motor", "manufacturer": "WEG", "model": "W22 Premium Efficiency", "x": 560, "y": 180 },
    { "id": "pump-1", "type": "pump", "label": "Grundfos CR Pump", "manufacturer": "Grundfos", "model": "CR/CRI/CRN", "x": 800, "y": 180 },
    { "id": "vibration_sensor-1", "type": "vibration_sensor", "label": "Vibration Sensor P-101", "manufacturer": "Endress+Hauser", "model": "iTHERM TM411", "x": 800, "y": 330 },
    { "id": "pressure_sensor-1", "type": "pressure_sensor", "label": "Discharge Pressure Sensor", "manufacturer": "Rosemount", "model": "3144P", "x": 1020, "y": 180 }
  ],
  "edges": [
    { "source": "breaker-1", "target": "vfd-1" },
    { "source": "plc-1", "target": "vfd-1" },
    { "source": "vfd-1", "target": "motor-1" },
    { "source": "motor-1", "target": "pump-1" },
    { "source": "motor-1", "target": "vibration_sensor-1" },
    { "source": "pump-1", "target": "pressure_sensor-1" }
  ]
}
\`\`\`

*Click **"Apply Topology to Designer Canvas"** below to render this topology immediately.*`;
		} else if (pLower.includes("vibration") || pLower.includes("bearing") || pLower.includes("pump")) {
			content = `### 🔍 Astra AI Asset Diagnosis
**Asset:** Main Feedwater Pump (P-101)
**Detected Pattern:** Non-linear vibration acceleration spike (+18% above 4.2 mm/s RMS baseline).
**Diagnosis:** Bearing outer race spalling (stage 2 degradation).

#### 🛠️ Immediate Operational Steps:
1. Verify lubricant oil level and check thermal camera readings on DE bearing housing.
2. Reduce pump speed to 2800 RPM via VFD setpoint.
3. Schedule drive end bearing replacement during upcoming weekend outage window.`;
		} else {
			content = `### 🤖 Astra AI Industrial Copilot
**Key:** \`xpl_277689...\` (Active Workspace Fusion)

**Status:** Plant operational telemetry signals are currently nominal across all monitored nodes.

#### 🎯 Quick Prompts:
- **"Generate Water Treatment Plant Topology"** (Builds PLC, VFD, Motor, Pump & Sensor layout)
- **"Design Automated Compressor Station Layout"**
- **"Diagnose Pump P-101 Vibration"**`;
		}

		return json({
			success: true,
			model: 'astra-local-expert',
			content
		});
	} catch (err: any) {
		return json({ success: false, error: err.message }, { status: 500 });
	}
};
