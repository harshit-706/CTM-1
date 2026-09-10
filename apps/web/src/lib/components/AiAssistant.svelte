<script lang="ts">
  import { goto } from '$app/navigation';
  import { aiGeneratedTopology, liveTelemetry } from '$lib/store';
  import { get } from 'svelte/store';

  // Svelte 5 runes – MUST use $state for reactivity
  let isOpen = $state(false);
  let query = $state('');
  let isLoading = $state(false);

  interface Message {
    id: string;
    sender: 'user' | 'ai';
    text: string;
    topologyData?: { nodes: any[], edges: any[] } | null;
    time: string;
  }

  let messages = $state<Message[]>([
    {
      id: 'welcome-1',
      sender: 'ai',
      text: "**CTM1 online.** Live asset telemetry fused.\n\nAsk a diagnostic question, request an energy report, or say **\"Build water treatment topology\"** to generate a plant layout.",
      time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    }
  ]);

  function toggleOpen() {
    isOpen = !isOpen;
  }

  function extractTopology(text: string): { nodes: any[], edges: any[] } | null {
    const match = text.match(/```topology_json\s*([\s\S]*?)\s*```/);
    if (match && match[1]) {
      try {
        return JSON.parse(match[1]);
      } catch (err) {
        console.error("Failed to parse topology_json:", err);
      }
    }
    return null;
  }

  async function sendPrompt(customPrompt?: string) {
    const textToSend = customPrompt || query.trim();
    if (!textToSend || isLoading) return;

    const userTime = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    messages = [...messages, { id: 'msg-' + Date.now(), sender: 'user', text: textToSend, time: userTime }];
    if (!customPrompt) query = '';

    // Attach live telemetry snapshot for context-aware AI responses
    const telemSnapshot = get(liveTelemetry);
    const telemetryPayload = Object.keys(telemSnapshot).length > 0 ? telemSnapshot : null;

    isLoading = true;
    try {
      const res = await fetch('/api/ai-insights', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ prompt: textToSend, telemetry: telemetryPayload })
      });
      const data = await res.json();
      const aiTime = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
      if (data.success) {
        const topo = extractTopology(data.content);
        messages = [...messages, {
          id: 'msg-' + Date.now(),
          sender: 'ai',
          text: data.content,
          topologyData: topo,
          time: aiTime
        }];
        if (topo) applyTopology(topo);
      } else {
        messages = [...messages, { id: 'msg-' + Date.now(), sender: 'ai', text: "❌ " + (data.error || "Error"), time: aiTime }];
      }
    } catch (err: any) {
      messages = [...messages, { id: 'msg-' + Date.now(), sender: 'ai', text: "⚠️ " + err.message, time: new Date().toLocaleTimeString() }];
    } finally {
      isLoading = false;
    }
  }

  function applyTopology(topo: { nodes: any[], edges: any[] }) {
    aiGeneratedTopology.set(topo);
    isOpen = false;
    goto('/topology');
  }

  function handleKeydown(e: KeyboardEvent) {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      sendPrompt();
    }
  }

  function formatMarkdown(text: string): string {
    const clean = text.replace(/```topology_json[\s\S]*?```/g, '');
    return clean
      .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
      .replace(/### (.*?)\n/g, '<h3 style="color:#34d399;font-weight:700;margin:8px 0 4px;font-size:13px;">$1</h3>')
      .replace(/#### (.*?)\n/g, '<h4 style="color:#67e8f9;font-weight:600;margin:6px 0 3px;font-size:12px;">$1</h4>')
      .replace(/\*\*(.*?)\*\*/g, '<strong style="color:#f1f5f9;">$1</strong>')
      .replace(/\*(.*?)\*/g, '<em style="color:#94a3b8;">$1</em>')
      .replace(/`([^`]+)`/g, '<code style="background:#1e293b;color:#fbbf24;padding:1px 5px;border-radius:4px;font-size:10px;font-family:monospace;">$1</code>')
      .replace(/\n\n/g, '<br/><br/>').replace(/\n/g, '<br/>');
  }
</script>

<!-- ✅ Portal wrapper: sits at body level via fixed positioning with very high z-index -->
<div style="position: fixed; bottom: 0; right: 0; z-index: 9999; pointer-events: none;">

  <!-- FAB Button -->
  {#if !isOpen}
    <div style="position: absolute; bottom: 24px; right: 24px; pointer-events: all;">
      <button
        onclick={toggleOpen}
        style="
          display: flex; align-items: center; gap: 8px;
          background: linear-gradient(135deg, #059669, #0d9488);
          color: white; font-weight: 700; font-size: 13px;
          padding: 12px 18px; border-radius: 999px; border: none;
          box-shadow: 0 0 24px rgba(16,185,129,0.5), 0 4px 16px rgba(0,0,0,0.4);
          cursor: pointer; transition: all 0.2s;
          font-family: 'Space Grotesk', sans-serif;
          user-select: none;
        "
        onmouseenter={(e) => { (e.currentTarget as HTMLElement).style.transform = 'scale(1.05)'; (e.currentTarget as HTMLElement).style.boxShadow = '0 0 30px rgba(16,185,129,0.7), 0 6px 20px rgba(0,0,0,0.5)'; }}
        onmouseleave={(e) => { (e.currentTarget as HTMLElement).style.transform = 'scale(1)'; (e.currentTarget as HTMLElement).style.boxShadow = '0 0 24px rgba(16,185,129,0.5), 0 4px 16px rgba(0,0,0,0.4)'; }}
      >
        <span style="position:relative;display:flex;width:10px;height:10px;">
          <span style="position:absolute;inset:0;border-radius:50%;background:#6ee7b7;opacity:0.75;animation:ping 1.5s ease-in-out infinite;"></span>
          <span style="position:relative;width:10px;height:10px;border-radius:50%;background:#34d399;display:block;"></span>
        </span>
        CTM1 AI
        <span style="background:rgba(4,47,46,0.8);color:#6ee7b7;font-size:9px;padding:2px 7px;border-radius:999px;letter-spacing:0.08em;font-family:monospace;text-transform:uppercase;">LIVE</span>
      </button>
    </div>
  {/if}

  <!-- Chat Drawer -->
  {#if isOpen}
    <div style="
      position: fixed; top: 0; right: 0; bottom: 0; width: 460px;
      background: rgba(3,7,18,0.97); backdrop-filter: blur(20px);
      border-left: 1px solid rgba(255,255,255,0.08);
      box-shadow: -8px 0 40px rgba(0,0,0,0.6);
      display: flex; flex-direction: column;
      z-index: 9999; pointer-events: all;
      font-family: 'Space Grotesk', sans-serif;
    ">

      <!-- Header -->
      <div style="padding: 16px 20px; border-bottom: 1px solid rgba(255,255,255,0.07); display:flex; align-items:center; justify-content:space-between; background:rgba(6,12,28,0.7); flex-shrink:0;">
        <div style="display:flex; align-items:center; gap:12px;">
          <div style="width:38px;height:38px;border-radius:12px;background:linear-gradient(135deg,#10b981,#06b6d4);display:flex;align-items:center;justify-content:center;font-size:18px;font-weight:900;box-shadow:0 0 16px rgba(16,185,129,0.5);">
            ⚡
          </div>
          <div>
            <div style="color:white;font-weight:700;font-size:15px;display:flex;align-items:center;gap:8px;">
              CTM1 AI
              <span style="background:rgba(16,185,129,0.2);color:#34d399;border:1px solid rgba(16,185,129,0.3);font-size:9px;padding:2px 6px;border-radius:4px;font-family:monospace;">Grok-2</span>
            </div>
            <div style="color:#64748b;font-size:11px;font-family:monospace;margin-top:2px;">Live telemetry fused · ctm1-edge</div>
          </div>
        </div>
        <button
          onclick={toggleOpen}
          style="color:#64748b;background:transparent;border:none;font-size:18px;cursor:pointer;padding:6px 10px;border-radius:8px;transition:all 0.15s;"
          onmouseenter={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.06)'; (e.currentTarget as HTMLElement).style.color = '#f1f5f9'; }}
          onmouseleave={(e) => { (e.currentTarget as HTMLElement).style.background = 'transparent'; (e.currentTarget as HTMLElement).style.color = '#64748b'; }}
        >✕</button>
      </div>

      <!-- Action Chips -->
      <div style="padding: 10px 14px; border-bottom: 1px solid rgba(255,255,255,0.05); display:flex; gap:8px; overflow-x:auto; flex-shrink:0; scrollbar-width:none;">
        <button
          onclick={() => sendPrompt("Build a water treatment plant topology with Siemens S7-1500 PLC, Danfoss VFD, WEG Motor, Grundfos Pump, and vibration sensors")}
          style="background:rgba(5,46,22,0.8);color:#34d399;border:1px solid rgba(16,185,129,0.5);padding:6px 12px;border-radius:999px;white-space:nowrap;font-size:11px;font-weight:600;cursor:pointer;transition:all 0.15s;font-family:'Space Grotesk',sans-serif;"
          onmouseenter={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(16,185,129,0.25)'; }}
          onmouseleave={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(5,46,22,0.8)'; }}
        >🎨 Build Topology</button>
        <button
          onclick={() => sendPrompt("Diagnose P-104 vibration and bearing health")}
          style="background:rgba(255,255,255,0.04);color:#94a3b8;border:1px solid rgba(255,255,255,0.1);padding:6px 12px;border-radius:999px;white-space:nowrap;font-size:11px;cursor:pointer;transition:all 0.15s;font-family:'Space Grotesk',sans-serif;"
          onmouseenter={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.08)'; }}
          onmouseleave={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.04)'; }}
        >🩺 Diagnose P-104</button>
        <button
          onclick={() => sendPrompt("Energy efficiency report for active plant assets")}
          style="background:rgba(255,255,255,0.04);color:#94a3b8;border:1px solid rgba(255,255,255,0.1);padding:6px 12px;border-radius:999px;white-space:nowrap;font-size:11px;cursor:pointer;transition:all 0.15s;font-family:'Space Grotesk',sans-serif;"
          onmouseenter={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.08)'; }}
          onmouseleave={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.04)'; }}
        >⚡ Energy Report</button>
        <button
          onclick={() => sendPrompt("Plant status summary with all active asset telemetry")}
          style="background:rgba(255,255,255,0.04);color:#94a3b8;border:1px solid rgba(255,255,255,0.1);padding:6px 12px;border-radius:999px;white-space:nowrap;font-size:11px;cursor:pointer;transition:all 0.15s;font-family:'Space Grotesk',sans-serif;"
          onmouseenter={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.08)'; }}
          onmouseleave={(e) => { (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.04)'; }}
        >📊 Plant Status</button>
      </div>

      <!-- Messages -->
      <div style="flex:1;padding:16px;overflow-y:auto;display:flex;flex-direction:column;gap:14px;">
        {#each messages as msg (msg.id)}
          <div style="display:flex;flex-direction:column;align-items:{msg.sender === 'user' ? 'flex-end' : 'flex-start'};">
            <div style="
              max-width: 92%; border-radius: 16px; padding: 12px 14px; font-size: 12px; line-height: 1.6;
              {msg.sender === 'user'
                ? 'background:linear-gradient(135deg,#059669,#0d9488);color:white;border-bottom-right-radius:4px;'
                : 'background:rgba(15,23,42,0.9);color:#cbd5e1;border:1px solid rgba(255,255,255,0.06);border-bottom-left-radius:4px;'}
            ">
              {#if msg.sender === 'ai'}
                {@html formatMarkdown(msg.text)}
                {#if msg.topologyData}
                  <div style="margin-top:12px;padding:12px;background:rgba(2,6,23,0.8);border:1px solid rgba(16,185,129,0.4);border-radius:12px;">
                    <div style="color:#34d399;font-weight:700;font-size:11px;margin-bottom:8px;display:flex;justify-content:space-between;align-items:center;">
                      <span>🎨 Generated Topology</span>
                      <span style="background:rgba(16,185,129,0.2);color:#6ee7b7;padding:2px 7px;border-radius:4px;font-family:monospace;font-size:9px;">{msg.topologyData.nodes?.length || 0} Nodes</span>
                    </div>
                    <button
                      onclick={() => applyTopology(msg.topologyData!)}
                      style="width:100%;background:linear-gradient(135deg,#10b981,#06b6d4);color:#022c22;font-weight:700;padding:8px;border-radius:8px;border:none;cursor:pointer;font-size:11px;font-family:'Space Grotesk',sans-serif;box-shadow:0 0 12px rgba(16,185,129,0.3);transition:all 0.15s;"
                      onmouseenter={(e) => { (e.currentTarget as HTMLElement).style.opacity = '0.85'; }}
                      onmouseleave={(e) => { (e.currentTarget as HTMLElement).style.opacity = '1'; }}
                    >Apply Topology to Canvas ➔</button>
                  </div>
                {/if}
              {:else}
                {msg.text}
              {/if}
            </div>
            <span style="font-size:10px;color:#334155;margin-top:3px;padding:0 4px;">{msg.time}</span>
          </div>
        {/each}

        {#if isLoading}
          <div style="display:flex;align-items:center;gap:8px;background:rgba(15,23,42,0.9);border:1px solid rgba(255,255,255,0.06);border-radius:16px;border-bottom-left-radius:4px;padding:12px 14px;width:fit-content;">
            <span style="width:8px;height:8px;border-radius:50%;background:#34d399;animation:ping 1s ease-in-out infinite;display:block;"></span>
            <span style="font-size:12px;color:#34d399;">CTM1 analyzing...</span>
          </div>
        {/if}
      </div>

      <!-- Input -->
      <div style="padding:12px;border-top:1px solid rgba(255,255,255,0.06);background:rgba(6,12,28,0.8);flex-shrink:0;">
        <div style="position:relative;display:flex;align-items:center;">
          <textarea
            bind:value={query}
            onkeydown={handleKeydown}
            placeholder="Describe your plant layout or ask a diagnostic question..."
            rows={2}
            style="width:100%;background:rgba(2,6,23,0.9);color:#e2e8f0;font-size:12px;border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:10px 48px 10px 12px;resize:none;outline:none;font-family:'Space Grotesk',sans-serif;box-sizing:border-box;transition:border-color 0.15s;"
            onfocus={(e) => { (e.currentTarget as HTMLElement).style.borderColor = 'rgba(16,185,129,0.6)'; }}
            onblur={(e) => { (e.currentTarget as HTMLElement).style.borderColor = 'rgba(255,255,255,0.1)'; }}
          ></textarea>
          <button
            onclick={() => sendPrompt()}
            disabled={!query.trim() || isLoading}
            style="position:absolute;right:8px;background:{(query.trim() && !isLoading) ? 'linear-gradient(135deg,#10b981,#06b6d4)' : 'rgba(255,255,255,0.08)'};border:none;border-radius:8px;padding:7px 9px;cursor:{(query.trim() && !isLoading) ? 'pointer' : 'not-allowed'};color:{query.trim() ? '#022c22' : '#475569'};font-weight:700;transition:all 0.15s;font-size:14px;"
          >➔</button>
        </div>
        <div style="margin-top:6px;font-size:10px;color:#334155;display:flex;justify-content:space-between;padding:0 2px;">
          <span>CTM1 · Grok-2 · Live Telemetry</span>
          <span style="color:rgba(52,211,153,0.6);font-family:monospace;">Edge Fused</span>
        </div>
      </div>
    </div>
  {/if}
</div>

<style>
  @keyframes ping {
    0%, 100% { transform: scale(1); opacity: 0.75; }
    50% { transform: scale(1.5); opacity: 0; }
  }
</style>
