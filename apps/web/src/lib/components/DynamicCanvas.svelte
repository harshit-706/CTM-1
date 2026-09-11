<script lang="ts">
  import { liveTelemetry } from '$lib/store';

  let { data }: { data: any } = $props();

  // Helpers to safely pull telemetry data
  function getTelemetryValue(binding: string) {
    // binding e.g., "asset-id.tag"
    if (!binding) return "N/A";
    const parts = binding.split('.');
    if (parts.length < 2) return "N/A";
    const assetId = parts[0];
    const tag = parts.slice(1).join('.');
    
    if ($liveTelemetry && $liveTelemetry[assetId] && $liveTelemetry[assetId][tag] !== undefined) {
      return $liveTelemetry[assetId][tag].toFixed(2);
    }
    return "--";
  }
</script>

{#if data && data.type === 'canvas_view'}
  <div style="margin-top: 12px; padding: 12px; background: rgba(15, 23, 42, 0.95); border: 1px solid rgba(16, 185, 129, 0.3); border-radius: 12px; box-shadow: inset 0 0 20px rgba(0,0,0,0.5);">
    <div style="display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid rgba(255,255,255,0.1); padding-bottom: 8px; margin-bottom: 12px;">
      <div style="color: #6ee7b7; font-size: 13px; font-weight: 700; text-transform: uppercase; letter-spacing: 0.05em;">
        {data.target || 'Plant Overview'}
      </div>
      <div style="background: rgba(16, 185, 129, 0.1); color: #34d399; font-size: 10px; padding: 2px 6px; border-radius: 4px; border: 1px solid rgba(16, 185, 129, 0.2); font-family: monospace;">
        Intent: {data.intent || 'Unknown'}
      </div>
    </div>

    <div style="display: flex; flex-direction: column; gap: 10px;">
      {#if data.components && data.components.length > 0}
        {#each data.components as comp}
          {#if comp.type === 'metric_card'}
            <div style="display: flex; justify-content: space-between; align-items: center; background: rgba(30, 41, 59, 0.8); padding: 10px 14px; border-radius: 8px; border-left: 3px solid #0ea5e9;">
              <span style="color: #94a3b8; font-size: 12px; font-weight: 500; text-transform: capitalize;">{comp.label}</span>
              <span style="color: #e2e8f0; font-size: 16px; font-weight: 700; font-family: monospace;">{getTelemetryValue(comp.binding)}</span>
            </div>
          {:else if comp.type === 'alarm_banner'}
            <div style="background: {comp.priority === 'CRITICAL' ? 'rgba(220, 38, 38, 0.15)' : 'rgba(245, 158, 11, 0.15)'}; border-left: 3px solid {comp.priority === 'CRITICAL' ? '#ef4444' : '#f59e0b'}; padding: 10px 14px; border-radius: 8px;">
              <div style="color: {comp.priority === 'CRITICAL' ? '#f87171' : '#fbbf24'}; font-size: 11px; font-weight: 700; text-transform: uppercase; margin-bottom: 4px;">
                {comp.priority} ALARM
              </div>
              <div style="color: #f1f5f9; font-size: 13px;">{comp.message}</div>
            </div>
          {/if}
        {/each}
      {:else}
        <div style="color: #64748b; font-size: 12px; text-align: center; padding: 20px 0;">
          No operational data available for this target.
        </div>
      {/if}
    </div>
  </div>
{/if}
