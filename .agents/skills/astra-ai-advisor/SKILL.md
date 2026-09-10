---
name: astra-ai-advisor
description: Queries the Astra/GPT AI Model API using the workspace API key (xpl_...) to perform automated root-cause diagnosis, anomaly scoring, and actionable maintenance advice for CTM-1 assets.
---

# Astra AI Industrial Advisor Skill

This skill allows Antigravity subagents to query the Astra/GPT model directly for real-time asset telemetry intelligence.

## Usage Procedure

1. Read the API key from `.env` (`ASTRA_GPT_API_KEY=xpl_277689f3fe27ba3a58da6d11d09acb6c270e7d7b`).
2. Run `python scripts/ai_insights.py --prompt "<your question or incident JSON>"` or issue an HTTP POST to `https://api.x.ai/v1/chat/completions`.
3. Format response into standard CTM-1 recommendation structure:
   - **Root Cause**
   - **Severity Assessment**
   - **Immediate Action Steps**
   - **Preventative Maintenance Advice**
