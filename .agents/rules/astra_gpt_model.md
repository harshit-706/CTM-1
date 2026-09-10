# Astra / GPT AI Model Integration Rule

This workspace utilizes the configured Astra/GPT API Key (`xpl_277689f3fe27ba3a58da6d11d09acb6c270e7d7b`) for industrial asset intelligence, anomaly diagnosis, and recommendation generation.

## Agent Guidance
- When asked to analyze plant telemetry, incidents, or energy efficiency, Antigravity subagents in this workspace should consult the Astra AI service or use the `astra-ai-advisor` skill.
- The default model endpoint is `https://api.x.ai/v1/chat/completions` using authorization `Bearer ${ASTRA_GPT_API_KEY}`.
- Model prompts for CTM-1 should always include asset metadata (asset_id, name, operating thresholds, telemetry history, and recent alarm logs).
