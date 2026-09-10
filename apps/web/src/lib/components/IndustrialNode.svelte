<script lang="ts">
    import { Handle, Position } from '@xyflow/svelte';
    import IndustrialIcons from './IndustrialIcons.svelte';
    let { data } = $props<{ data: { label: string; type: string; manufacturer?: string; model?: string; isSource?: boolean; isTarget?: boolean } }>();

    // ISA-inspired color system mapped to neon-green industrial palette
    const typeConfig: Record<string, { border: string; bg: string; text: string; dot: string }> = {
        'plc':             { border: '#3b82f6', bg: 'rgba(59,130,246,0.08)', text: '#93c5fd', dot: '#3b82f6' },
        'pac':             { border: '#6366f1', bg: 'rgba(99,102,241,0.08)', text: '#a5b4fc', dot: '#6366f1' },
        'remote_io':       { border: '#06b6d4', bg: 'rgba(6,182,212,0.08)',  text: '#67e8f9', dot: '#06b6d4' },
        'edge_gateway':    { border: '#8b5cf6', bg: 'rgba(139,92,246,0.08)', text: '#c4b5fd', dot: '#8b5cf6' },
        'breaker':         { border: '#f43f5e', bg: 'rgba(244,63,94,0.08)',  text: '#fda4af', dot: '#f43f5e' },
        'vfd':             { border: '#f59e0b', bg: 'rgba(245,158,11,0.08)', text: '#fcd34d', dot: '#f59e0b' },
        'soft_starter':    { border: '#f97316', bg: 'rgba(249,115,22,0.08)', text: '#fdba74', dot: '#f97316' },
        'energy_meter':    { border: '#00e676', bg: 'rgba(0,230,118,0.06)',  text: '#86efac', dot: '#00e676' },
        'motor':           { border: 'rgba(255,255,255,0.3)', bg: 'rgba(255,255,255,0.04)', text: 'rgba(255,255,255,0.85)', dot: '#e2e8f0' },
        'pump':            { border: '#10b981', bg: 'rgba(16,185,129,0.08)', text: '#6ee7b7', dot: '#10b981' },
        'compressor':      { border: '#14b8a6', bg: 'rgba(20,184,166,0.08)', text: '#5eead4', dot: '#14b8a6' },
        'fan':             { border: '#0ea5e9', bg: 'rgba(14,165,233,0.08)', text: '#7dd3fc', dot: '#0ea5e9' },
        'temp_sensor':     { border: '#ef4444', bg: 'rgba(239,68,68,0.07)',  text: '#fca5a5', dot: '#ef4444' },
        'pressure_sensor': { border: '#a855f7', bg: 'rgba(168,85,247,0.07)', text: '#d8b4fe', dot: '#a855f7' },
        'flow_sensor':     { border: '#06b6d4', bg: 'rgba(6,182,212,0.07)',  text: '#67e8f9', dot: '#06b6d4' },
        'vibration_sensor':{ border: '#eab308', bg: 'rgba(234,179,8,0.07)',  text: '#fde047', dot: '#eab308' },
        'hmi':             { border: 'rgba(255,255,255,0.2)', bg: 'rgba(255,255,255,0.03)', text: 'rgba(255,255,255,0.6)', dot: '#9ca3af' },
        'scada':           { border: 'rgba(255,255,255,0.2)', bg: 'rgba(255,255,255,0.03)', text: 'rgba(255,255,255,0.6)', dot: '#9ca3af' },
    };

    let cfg = $derived(typeConfig[data.type] || { border: 'rgba(255,255,255,0.15)', bg: 'rgba(255,255,255,0.04)', text: 'rgba(255,255,255,0.7)', dot: '#9ca3af' });
</script>

<div style="
    padding: 10px 14px;
    border-radius: 8px;
    border: 1px solid {cfg.border};
    background: {cfg.bg};
    min-width: 160px;
    backdrop-filter: blur(8px);
    box-shadow: 0 0 20px {cfg.dot}18, 0 2px 8px rgba(0,0,0,0.4);
    transition: box-shadow 0.2s;
    position: relative;
    font-family: var(--font-sans, 'Space Grotesk', sans-serif);
">
    {#if data.isTarget !== false}
        <Handle type="target" position={Position.Top}
            style="width:8px;height:8px;background:{cfg.dot};border:2px solid #07070c;top:-4px;box-shadow:0 0 6px {cfg.dot};" />
    {/if}

    <div style="display:flex;align-items:flex-start;gap:10px;">
        <div style="width:24px;height:24px;border-radius:6px;background:{cfg.dot}22;display:flex;align-items:center;justify-content:center;color:{cfg.dot};flex-shrink:0;">
            <IndustrialIcons type={data.type} className="w-4 h-4" />
        </div>

        <div style="display:flex;flex-direction:column;min-width:0;flex:1;">
            <div style="display:flex;align-items:center;justify-content:space-between;gap:6px;">
                <span style="font-size:9px;font-weight:700;text-transform:uppercase;letter-spacing:0.08em;color:{cfg.dot};opacity:0.7;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">
                    {data.type.replace(/_/g, ' ')}
                </span>
                <div style="width:5px;height:5px;border-radius:50%;background:{cfg.dot};flex-shrink:0;box-shadow:0 0 4px {cfg.dot};"></div>
            </div>
            <span style="font-size:12px;font-weight:600;color:{cfg.text};white-space:nowrap;overflow:hidden;text-overflow:ellipsis;margin-top:2px;">
                {data.label}
            </span>
            
            {#if data.manufacturer || data.model}
                <div style="margin-top:6px;padding-top:6px;border-top:1px dashed {cfg.dot}44;display:flex;flex-direction:column;gap:2px;">
                    {#if data.manufacturer}
                        <span style="font-size:9px;color:rgba(255,255,255,0.4);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">Mfr: <span style="color:rgba(255,255,255,0.7);">{data.manufacturer}</span></span>
                    {/if}
                    {#if data.model}
                        <span style="font-size:9px;color:rgba(255,255,255,0.4);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">Mod: <span style="font-family:var(--font-mono);color:rgba(255,255,255,0.6);">{data.model}</span></span>
                    {/if}
                </div>
            {/if}
        </div>
    </div>

    {#if data.isSource !== false}
        <Handle type="source" position={Position.Bottom}
            style="width:8px;height:8px;background:{cfg.dot};border:2px solid #07070c;bottom:-4px;box-shadow:0 0 6px {cfg.dot};" />
    {/if}
</div>
