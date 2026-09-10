<script lang="ts">
    import { page } from '$app/stores';
    import { onMount, onDestroy } from 'svelte';
    import { liveTelemetry } from '$lib/store';
    import * as echarts from 'echarts';

    let assetId = $page.params.id;
    let data = $derived($liveTelemetry[assetId] || {});

    let chartContainer: HTMLDivElement | undefined;
    let chart: echarts.ECharts | undefined;
    let history: {time: string, key: string, value: number}[] = [];

    const format = (val: number | undefined, dec = 1) => val !== undefined ? val.toFixed(dec) : '--';

    // Build telemetry cards from live data dynamically
    let tagEntries = $derived(Object.entries(data).filter(([k]) => !['lastUpdate', 'status', 'quality'].includes(k)));

    function tagColor(key: string): string {
        if (key.includes('current') || key.includes('power') || key.includes('energy')) return '#3b82f6';
        if (key.includes('temp')) return '#f59e0b';
        if (key.includes('flow') || key.includes('pressure')) return '#06b6d4';
        if (key.includes('vibration')) return '#a855f7';
        if (key.includes('speed') || key.includes('freq') || key.includes('rpm')) return 'var(--neon)';
        return 'rgba(255,255,255,0.7)';
    }
    function tagUnit(key: string): string {
        if (key.includes('_hz') || key.includes('freq')) return 'Hz';
        if (key.includes('_a') || key.includes('current')) return 'A';
        if (key.includes('_kw') || key.includes('power_kw')) return 'kW';
        if (key.includes('kwh')) return 'kWh';
        if (key.includes('_bar') || key.includes('pressure')) return 'bar';
        if (key.includes('_v') || key.includes('voltage')) return 'V';
        if (key.includes('_c') || key.includes('temp')) return '°C';
        if (key.includes('vibration')) return 'mm/s';
        if (key.includes('flow')) return 'm³/h';
        if (key.includes('rpm')) return 'RPM';
        return '';
    }
    function formatLabel(key: string) {
        return key.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase());
    }

    $effect(() => {
        if (data && chart) {
            // Push first numeric entry to chart
            const firstNumeric = Object.entries(data).find(([k, v]) => typeof v === 'number' && !['lastUpdate'].includes(k));
            if (firstNumeric) {
                const [key, value] = firstNumeric;
                const now = new Date().toLocaleTimeString();
                history.push({ time: now, key, value: value as number });
                if (history.length > 60) history.shift();
                chart.setOption({
                    xAxis: { data: history.map(h => h.time) },
                    series: [{ data: history.map(h => h.value), name: formatLabel(key) }]
                });
            }
        }
    });

    onMount(() => {
        if (!chartContainer) return;
        chart = echarts.init(chartContainer, 'dark');
        chart.setOption({
            backgroundColor: 'transparent',
            tooltip: { trigger: 'axis', backgroundColor: '#0d0d12', borderColor: 'rgba(0,230,118,0.3)', textStyle: { color: 'white', fontFamily: 'JetBrains Mono' } },
            grid: { left: '8%', right: '4%', bottom: '12%', top: '10%', containLabel: false },
            xAxis: { type: 'category', data: [], axisLine: { lineStyle: { color: 'rgba(255,255,255,0.08)' } }, axisTick: { show: false }, axisLabel: { color: 'rgba(255,255,255,0.3)', fontSize: 10 } },
            yAxis: { type: 'value', axisLine: { show: false }, axisTick: { show: false }, splitLine: { lineStyle: { color: 'rgba(255,255,255,0.05)' } }, axisLabel: { color: 'rgba(255,255,255,0.3)', fontSize: 10 } },
            series: [{
                name: 'Value',
                type: 'line',
                data: [],
                smooth: true,
                symbol: 'none',
                lineStyle: { color: 'var(--neon)', width: 2 },
                areaStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
                    { offset: 0, color: 'rgba(0,230,118,0.25)' },
                    { offset: 1, color: 'rgba(0,230,118,0.01)' }
                ])}
            }]
        });
        const onResize = () => chart?.resize();
        window.addEventListener('resize', onResize);
        return () => { window.removeEventListener('resize', onResize); chart?.dispose(); };
    });
</script>

<svelte:head>
    <title>Asset: {assetId} — CTM-1</title>
</svelte:head>

<div style="height:100%; overflow-y:auto; padding:28px 32px;">
    <!-- Header -->
    <div style="display:flex; align-items:center; justify-content:space-between; margin-bottom:24px;">
        <div style="display:flex;align-items:center;gap:14px;">
            <a href="/assets" class="btn btn-ghost" style="padding:6px 10px; text-decoration:none;" title="Back to Assets">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="15 18 9 12 15 6"/></svg>
            </a>
            <div>
                <div style="display:flex;align-items:center;gap:8px;">
                    <h1 style="font-size:1.5rem; font-weight:700; color:white; margin:0; font-family:var(--font-mono);">{assetId}</h1>
                    <span class="badge badge-simulated">SIMULATED</span>
                </div>
                <p style="color:rgba(255,255,255,0.4); font-size:13px; margin:2px 0 0;">Live telemetry and health monitoring</p>
            </div>
        </div>
        <div style="display:flex;align-items:center;gap:8px;">
            {#if Object.keys(data).length > 0}
                <div style="display:flex;align-items:center;gap:6px;padding:6px 12px;border-radius:20px;border:1px solid rgba(0,230,118,0.25);background:rgba(0,230,118,0.08);">
                    <div class="neon-dot" style="width:6px;height:6px;"></div>
                    <span style="font-size:12px;font-weight:600;color:var(--neon);">ONLINE</span>
                </div>
            {:else}
                <div style="display:flex;align-items:center;gap:6px;padding:6px 12px;border-radius:20px;border:1px solid rgba(255,255,255,0.1);background:rgba(255,255,255,0.04);">
                    <div style="width:6px;height:6px;border-radius:50%;background:#555;"></div>
                    <span style="font-size:12px;font-weight:600;color:rgba(255,255,255,0.4);">WAITING</span>
                </div>
            {/if}
        </div>
    </div>

    <!-- Telemetry Cards Grid -->
    {#if tagEntries.length > 0}
        <div style="display:grid; grid-template-columns:repeat(auto-fill, minmax(160px, 1fr)); gap:12px; margin-bottom:24px;">
            {#each tagEntries as [key, value]}
                {@const color = tagColor(key)}
                {@const unit = tagUnit(key)}
                <div class="telem-card" style="border-color:rgba(255,255,255,0.06);">
                    <div class="telem-label">{formatLabel(key)}</div>
                    <div class="telem-value" style="color:{color};">
                        {typeof value === 'number' ? format(value) : value}
                        {#if unit}<span class="telem-unit">{unit}</span>{/if}
                    </div>
                </div>
            {/each}
        </div>
    {:else}
        <div style="text-align:center; padding:32px; border:1px dashed rgba(255,255,255,0.07); border-radius:10px; margin-bottom:24px; color:rgba(255,255,255,0.25); font-size:13px;">
            Waiting for live telemetry from Edge Core WebSocket...
        </div>
    {/if}

    <!-- Live Chart -->
    <div class="panel" style="padding:20px;">
        <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:16px;">
            <h2 style="font-size:14px;font-weight:600;color:white;margin:0;">Live Telemetry Chart</h2>
            {#if history.length === 0}
                <span class="badge badge-simulated">AWAITING DATA</span>
            {:else}
                <span style="font-size:11px;color:rgba(255,255,255,0.3);font-family:var(--font-mono);">{history.length} samples</span>
            {/if}
        </div>
        <div bind:this={chartContainer} style="width:100%; height:240px;"></div>
    </div>
</div>
