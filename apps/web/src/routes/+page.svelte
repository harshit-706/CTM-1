<script lang="ts">
    import { liveTelemetry, activeAlarms, incidents, energyStats } from '$lib/store';
    import { onMount } from 'svelte';

    let assets = $state<any[]>([]);
    let loading = $state(true);
    let totalAssets = $derived(assets.length);
    let alarmList = $derived(Object.values($activeAlarms).slice(0, 6) as any[]);
    let criticalCount = $derived(alarmList.filter((alarm) => alarm.priority === 'critical').length);
    let warningCount = $derived(alarmList.length - criticalCount);
    let healthyAssets = $derived(assets.filter((asset) => !$activeAlarms[asset.id]).length);
    let incidentCount = $derived(Object.keys($incidents).length);
    let healthPct = $derived(totalAssets === 0 ? 100 : Math.round((healthyAssets / totalAssets) * 100));
    let avgIntensity = $derived(() => {
        const stats = Object.values($energyStats) as any[];
        return stats.length ? stats.reduce((sum, stat) => sum + (stat.intensity || 0), 0) / stats.length : 0;
    });

    onMount(async () => {
        try {
            const response = await fetch('http://localhost:8080/api/v1/assets');
            if (response.ok) assets = (await response.json()).assets || [];
        } catch (error) {
            console.error('Failed to load assets', error);
        } finally {
            loading = false;
        }
    });
</script>

<svelte:head><title>Plant Command Center | CTM-1</title></svelte:head>

<div class="command-page">
    <header class="command-header">
        <div><div class="eyebrow"><span class="status-mark state-normal"></span> PLANT 01 / OVERVIEW</div><h1>Plant Command Center</h1><p>Current operating state across monitored areas.</p></div>
        <div class="header-actions"><span class="sim-badge">SIMULATION DATA</span><a class="command-button" href="/topology">+ Add asset</a></div>
    </header>

    <a class="alarm-strip {alarmList.length ? 'alarm-active' : ''}" href="/incidents" aria-label="Open active alarms">
        <span class="alarm-icon">!</span><strong>{alarmList.length ? 'ACTIVE ALARMS' : 'NO ACTIVE ALARMS'}</strong>
        {#if alarmList.length}<span class="alarm-count critical-count">{criticalCount} critical</span><span class="alarm-count warning-count">{warningCount} warning</span><span class="alarm-link">Review alarm list →</span>{:else}<span class="alarm-link">Alarm state is clear</span>{/if}
    </a>

    <section class="kpi-grid responsive-kpis" aria-label="Plant summary">
        <div class="hmi-panel kpi-panel"><span class="section-label">Monitored assets</span><strong class="live-number kpi-value">{loading ? '—' : totalAssets}</strong><span class="kpi-context state-normal">{healthyAssets} normal</span></div>
        <div class="hmi-panel kpi-panel"><span class="section-label">Process health</span><strong class="live-number kpi-value">{healthPct}<small>%</small></strong><span class="kpi-context">Based on current asset state</span></div>
        <div class="hmi-panel kpi-panel"><span class="section-label">Active incidents</span><strong class="live-number kpi-value {incidentCount ? 'value-alarm' : ''}">{incidentCount}</strong><span class="kpi-context {incidentCount ? 'state-alarm' : ''}">{incidentCount ? 'Action required' : 'No action required'}</span></div>
        <div class="hmi-panel kpi-panel"><span class="section-label">Energy intensity</span><strong class="live-number kpi-value">{avgIntensity().toFixed(2)}<small> kWh/unit</small></strong><span class="kpi-context">Production-normalized average</span></div>
    </section>

    <div class="overview-grid responsive-grid">
        <section class="hmi-panel asset-panel">
            <div class="panel-heading"><div><span class="section-label">Level 1 overview</span><h2>Asset health</h2></div><a href="/assets">View all →</a></div>
            {#if loading}<div class="empty-state">Loading current asset state...</div>{:else if !assets.length}<div class="empty-state">No assets found. <a href="/topology">Open topology</a> to configure one.</div>{:else}
                <div class="asset-list">{#each assets.slice(0, 8) as asset}
                    {@const hasAlarm = !!$activeAlarms[asset.id]}
                    {@const hasData = !!$liveTelemetry[asset.id]}
                    <a class="asset-row" href={`/assets/${asset.id}`}><span class="asset-state-icon {hasAlarm ? 'state-alarm' : 'state-normal'}">{hasAlarm ? '!' : '●'}</span><span class="asset-name"><strong>{asset.name}</strong><small>{asset.type}</small></span><span class="state-chip {hasAlarm ? 'state-alarm' : hasData ? 'state-normal' : 'state-fault'}">{hasAlarm ? 'Alarm' : hasData ? 'Normal' : 'Fault'}</span><span class="asset-meta">{hasData ? 'Updated now' : 'No live data'}</span></a>
                {/each}</div>
            {/if}
        </section>

        <section class="hmi-panel alarm-panel"><div class="panel-heading"><div><span class="section-label">Priority then recency</span><h2>Alarm state</h2></div><a href="/incidents">Open center →</a></div>
            {#if !alarmList.length}<div class="empty-state clear-state"><span class="state-normal">●</span> No active alarms</div>{:else}<div class="alarm-list">{#each alarmList as alarm, index}<div class="alarm-row {index === 0 ? 'alarm-pulse' : ''}"><span class="alarm-priority {alarm.priority === 'critical' ? 'state-critical' : 'state-warning'}">{alarm.priority || 'warning'}</span><span class="alarm-detail"><strong>{alarm.message}</strong><small>{alarm.asset_id} · {new Date(alarm.timestamp * 1000).toLocaleTimeString()}</small></span></div>{/each}</div>{/if}
        </section>
    </div>
</div>
