import { json } from '@sveltejs/kit';
import type { RequestHandler } from './$types';

const API_KEY = process.env.GROQ_API_KEY;
const ENDPOINT = process.env.GROQ_ENDPOINT || "https://api.groq.com/openai/v1/chat/completions";

const SYSTEM_PROMPT = `You are CTM1 — an embedded industrial AI for the CTM-1 edge monitoring platform.
You receive real-time asset telemetry and operator questions. Respond ONLY with a valid JSON object.
Do not output raw text outside the JSON object.

JSON SCHEMA:
{
  "message": "Conversational reply. Provide context, explain alarms, say hello, or output the topology_json markdown block here.",
  "action": {
    "intent": "show | show_overview | none",
    "target": "asset_id or asset_name (e.g., 'breaker-1', 'pump-1', or empty)",
    "properties": ["temp", "vibration", "current", "speed", "state", "status", "health"]
  }
}

RULES:
- If the user asks a conversational question, answer it in the "message" field.
- If the user asks for asset data, set "action.intent" to "show" and "action.target" to the asset ID. If asking for a general status, set "action.intent" to "show_overview".
- If NO widget needs to be rendered, set "action.intent" to "none".
- Map requested properties to "action.properties". Always include ["state", "status", "health"] for general status.

TOPOLOGY FORMAT (use ONLY when user asks to build/create/design a plant. Put this INSIDE the "message" field):
\`\`\`topology_json
{
  "nodes": [
    { "id": "plc-1", "type": "plc", "label": "Siemens S7-1500", "manufacturer": "Siemens", "model": "S7-1500", "x": 80, "y": 200 }
  ],
  "edges": [
    { "source": "plc-1", "target": "vfd-1" }
  ]
}
\`\`\`
Valid node types: plc, pac, remote_io, edge_gateway, breaker, vfd, soft_starter, energy_meter, motor, pump, compressor, fan, temp_sensor, pressure_sensor, flow_sensor, vibration_sensor, hmi, scada.
Valid edges: plc->vfd, breaker->motor, etc.`;

export const POST: RequestHandler = async ({ request }) => {
	try {
		const body = await request.json();
		const userPrompt = body.prompt || "Give plant status summary.";

		// 1. Fetch current machine context from C++ Edge Core
		let contextData: any = null;
		try {
			const ctxRes = await fetch('http://127.0.0.1:8080/api/v1/context');
			if (ctxRes.ok) {
				contextData = await ctxRes.json();
			}
		} catch (e) {
			console.warn('[CTM1] Failed to fetch context from Edge Core:', e);
			return json({ success: false, error: "Edge Core unavailable for context retrieval." }, { status: 502 });
		}

		// 2. Prepare context for Groq
		const contextStr = contextData ? JSON.stringify(contextData, null, 2) : "No context available.";
		const fullPrompt = `OPERATOR INTENT: ${userPrompt}\n\nCURRENT CONTEXT:\n${contextStr}`;

		// 3. Call Groq LLM to interpret intent into a BOUNDED JSON TASK
		let intentTask: any = null;
		let aiMessage: string = "Processing completed.";

		try {
			const res = await fetch(ENDPOINT, {
				method: 'POST',
				headers: {
					'Content-Type': 'application/json',
					'Authorization': `Bearer ${API_KEY}`
				},
				body: JSON.stringify({
					model: 'qwen/qwen3.8-27b',
					messages: [
						{ role: 'system', content: SYSTEM_PROMPT },
						{ role: 'user', content: fullPrompt }
					],
					temperature: 0.1,
					max_tokens: 1500,
					response_format: { type: "json_object" }
				})
			});

			if (res.ok) {
				const data = await res.json();
				const answer = data.choices?.[0]?.message?.content;
				if (answer && (answer.startsWith('{') || answer.startsWith('['))) {
					const parsed = JSON.parse(answer);
					aiMessage = parsed.message || "Screen generated.";
					intentTask = parsed.action;
				}
			} else {
			    const errText = await res.text();
			    console.error("Groq error:", errText);
			    return json({ success: false, error: "AI interpretation failed (Groq Error)" }, { status: 502 });
			}
		} catch (e) {
			console.warn('[CTM1] LLM unavailable:', e);
			return json({ success: false, error: "AI interpretation failed (Network Error)" }, { status: 502 });
		}

        if (!intentTask) {
            return json({ success: false, error: "AI failed to produce a valid action task." }, { status: 500 });
        }

		// 4. Send Bounded JSON Task to C++ Edge Core for Deterministic Validation (if applicable)
		let validatedHmi: any = null;
		if (intentTask.intent !== "none") {
			try {
				const validateRes = await fetch('http://127.0.0.1:8080/api/v1/ai/resolve', {
				    method: 'POST',
				    headers: { 'Content-Type': 'application/json' },
				    body: JSON.stringify(intentTask)
				});
				const validateData = await validateRes.json();
				
				if (validateData.success) {
				    validatedHmi = validateData.hmi_json;
				} else {
				    return json({ success: false, error: validateData.error || "C++ Validation failed." });
				}
			} catch (e) {
				console.warn('[CTM1] C++ Validation unavailable:', e);
				return json({ success: false, error: "Validation layer unavailable." }, { status: 502 });
			}
		}

        // Return the dynamically generated and C++ validated HMI JSON
		return json({ 
		    success: true, 
		    model: 'qwen/qwen3.8-27b', 
		    content: aiMessage, 
		    hmi_json: validatedHmi,
		    context_version: contextData?.contextVersion,
		    bounded_task: intentTask
		});
	} catch (err: any) {
		return json({ success: false, error: err.message }, { status: 500 });
	}
};
