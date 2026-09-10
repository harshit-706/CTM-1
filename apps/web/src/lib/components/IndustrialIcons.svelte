<script lang="ts">
    let { type, className = "w-6 h-6" } = $props<{ type: string; className?: string }>();
</script>

{#if type === 'plc' || type === 'pac' || type === 'remote_io'}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <rect x="2" y="4" width="20" height="16" rx="2" />
        <line x1="6" y1="8" x2="6" y2="8.01" stroke-width="3" />
        <line x1="10" y1="8" x2="10" y2="8.01" stroke-width="3" />
        <line x1="14" y1="8" x2="14" y2="8.01" stroke-width="3" />
        <line x1="18" y1="8" x2="18" y2="8.01" stroke-width="3" />
        <line x1="6" y1="16" x2="6" y2="16.01" stroke-width="3" />
        <line x1="10" y1="16" x2="10" y2="16.01" stroke-width="3" />
        <line x1="14" y1="16" x2="14" y2="16.01" stroke-width="3" />
        <line x1="18" y1="16" x2="18" y2="16.01" stroke-width="3" />
        <rect x="6" y="11" width="12" height="2" fill="currentColor" stroke="none" />
    </svg>
{:else if type === 'motor'}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <circle cx="12" cy="12" r="9" />
        <path d="M9 14L9 10 12 14 15 10 15 14" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
        <path d="M12 3 L12 0" />
        <path d="M12 24 L12 21" />
    </svg>
{:else if type === 'pump'}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <circle cx="12" cy="12" r="9" />
        <path d="M12 3 L12 21" />
        <path d="M3 12 L21 12" />
        <path d="M12 12 L19 5" />
    </svg>
{:else if type === 'compressor' || type === 'fan'}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <path d="M12 2 A10 10 0 0 1 22 12 A10 10 0 0 1 12 22 A10 10 0 0 1 2 12 A10 10 0 0 1 12 2" />
        <path d="M12 12 L12 2 M12 12 L22 12 M12 12 L7.5 20 M12 12 L2 7.5 M12 12 L20 18" />
        <circle cx="12" cy="12" r="2" fill="currentColor" />
    </svg>
{:else if type === 'vfd' || type === 'soft_starter'}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <rect x="4" y="3" width="16" height="18" rx="1" />
        <rect x="7" y="6" width="10" height="4" fill="currentColor" opacity="0.3" />
        <line x1="8" y1="14" x2="10" y2="14" stroke-width="2" />
        <line x1="14" y1="14" x2="16" y2="14" stroke-width="2" />
        <path d="M8 18 L16 18 M12 14 L12 18" />
        <polyline points="8,10 12,8 16,10" stroke-width="1" />
    </svg>
{:else if type.includes('sensor')}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <circle cx="12" cy="8" r="5" />
        <line x1="12" y1="13" x2="12" y2="22" />
        <line x1="9" y1="22" x2="15" y2="22" />
        <text x="12" y="9.5" text-anchor="middle" font-size="6" font-family="sans-serif" stroke="none" fill="currentColor">
            {type === 'temp_sensor' ? 'T' : type === 'pressure_sensor' ? 'P' : type === 'flow_sensor' ? 'F' : type === 'vibration_sensor' ? 'V' : 'S'}
        </text>
    </svg>
{:else if type === 'energy_meter' || type === 'breaker'}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <rect x="5" y="4" width="14" height="16" />
        <polyline points="12,7 9,12 12,12 12,17 15,12 12,12 12,7" stroke-width="1.5" fill="currentColor" />
    </svg>
{:else if type === 'hmi' || type === 'scada' || type === 'edge_gateway'}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <rect x="2" y="4" width="20" height="14" rx="2" />
        <path d="M8 22 L16 22 M12 18 L12 22" stroke-width="2" />
        <rect x="4" y="6" width="16" height="10" fill="currentColor" opacity="0.2" stroke="none" />
    </svg>
{:else}
    <svg class={className} viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <rect x="3" y="3" width="18" height="18" rx="2" />
        <circle cx="12" cy="12" r="3" />
    </svg>
{/if}
