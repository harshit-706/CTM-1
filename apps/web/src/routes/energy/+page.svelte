<script lang="ts">
    import { energyStats, connectionStatus } from '$lib/store';
    import * as echarts from 'echarts';
    import { onMount } from 'svelte';

    let chartContainer: HTMLDivElement | undefined;
    let chart: echarts.ECharts | undefined;

    let allAssets = $derived(Object.keys($energyStats));
    let selectedAsset = $state<string | null>(null);
    let targetAssetId = $derived(selectedAsset || (allAssets.length > 0 ? allAssets[0] : null));
    let data = $derived(targetAssetId ? $energyStats[targetAssetId] : { power_kw: 0, production_rate: 0, intensity: 0, deviation_pct: 0 });

    let history: {time: string, value: number}[] = $state([]);

    $effect(() => {
        if (data && data.power_kw && chart) {
            const now = new Date().toLocaleTimeString();
            history.push({ time: now, value: data.intensity });
            if (history.length > 60) history.shift();
            chart.setOption({
                xAxis: { data: history.map(h => h.time) },
                series: [{ data: history.map(h => h.value) }]
            });
        }
    });

    $effect(() => {
        if (targetAssetId) {
            history = [];
            chart?.setOption({ xAxis: { data: [] }, series: [{ data: [] }] });
        }
    });

    onMount(() => {
        if (!chartContainer) return;
        chart = echarts.init(chartContainer, 'dark');
        chart.setOption({
            backgroundColor: 'transparent',
            tooltip: {
                trigger: 'axis',
                backgroundColor: '#0d0d12',
                borderColor: 'rgba(0,230,118,0.3)',
                textStyle: { color: 'white', fontFamily: 'JetBrains Mono', fontSize: 11 }
            },
            grid: { left: '8%', right: '4%', bottom: '12%', top: '10%', containLabel: false },
            xAxis: {
                type: 'category', data: [],
                axisLine: { lineStyle: { color: 'rgba(255,255,255,0.07)' } },
                axisTick: { show: false },
                axisLabel: { color: 'rgba(255,255,255,0.3)', fontSize: 10 }
            },
            yAxis: {
                type: 'value',
                name: 'kWh/unit',
                nameTextStyle: { color: 'rgba(255,255,255,0.3)', fontSize: 10 },
                axisLine: { show: false },
                axisTick: { show: false },
                splitLine: { lineStyle: { color: 'rgba(255,255,255,0.05)' } },
                axisLabel: { color: 'rgba(255,255,255,0.3)', fontSize: 10 }
            },
            series: [{
                name: 'Energy Intensity',
                type: 'line',
                data: [],
                smooth: true,
                symbol: 'none',
                lineStyle: { color: 'var(--neon)', width: 2 },
                areaStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
                    { offset: 0, color: 'rgba(0,230,118,0.3)' },
                    { offset: 1, color: 'rgba(0,230,118,0.01)' }
                ])}
            }]
        });
        const resize = () => chart?.resize();
        window.addEventListener('resize', resize);
        return () => { window.removeEventListener('resize', resize); chart?.dispose(); };
    });

    const fmt = (v: number) => (v || 0).toFixed(2);
    const isHigh = $derived((data?.deviation_pct || 0) > 15);
</script>

<svelte:head>
    <title>Energy Analytics — CTM-1</title>
</svelte:head>

<div style="height:100%; overflow-y:auto; padding:28px 32px;">
    <!-- Header -->
    <div style="display:flex; align-items:flex-start; justify-content:space-between; margin-bottom:24px;">
        <div>
            <h1 style="font-size:1.75rem; font-weight:700; color:white; letter-spacing:-0.02em; margin:0;">Energy Analytics</h1>
            <p style="color:rgba(255,255,255,0.4); font-size:13px; margin:4px 0 0;">Production-contextualized energy efficiency and baseline deviations.</p>
        </div>
        {#if allAssets.length > 0}
            <select
                bind:value={selectedAsset}
                class="industrial-input industrial-select"
                style="min-width:160px;"
            >
                {#each allAssets as assetId}
                    <option value={assetId}>{assetId}</option>
                {/each}
            </select>
        {/if}
    </div>

    {#if !targetAssetId}
        <div style="text-align:center;padding:64px 32px;border:1px dashed rgba(255,255,255,0.07);border-radius:12px;color:rgba(255,255,255,0.2);">
            <svg style="width:40px;height:40px;margin:0 auto 12px;opacity:0.3;" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M13 10V3L4 14h7v7l9-11h-7z"/></svg>
            <p style="font-size:14px;margin:0;">
                {$connectionStatus === 'CONNECTED' ? 'Waiting for energy telemetry...' : 'Edge Core disconnected'}
            </p>
        </div>
    {:else}
        <!-- Asset indicator -->
        <div style="display:flex;align-items:center;gap:8px;margin-bottom:20px;">
            <span class="section-label">Viewing asset:</span>
            <span style="font-size:13px;font-weight:600;color:var(--neon);font-family:var(--font-mono);">{targetAssetId}</span>
        </div>

        <!-- KPI Grid -->
        <div style="display:grid;grid-template-columns:repeat(3,1fr);gap:16px;margin-bottom:20px;">
            <!-- Production Rate -->
            <div class="panel" style="padding:20px;">
                <div style="display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:12px;">
                    <span class="section-label">Production Rate</span>
                    <div style="width:28px;height:28px;border-radius:7px;background:rgba(0,230,118,0.1);display:flex;align-items:center;justify-content:center;">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="var(--neon)" stroke-width="2"><rect x="2" y="7" width="20" height="14" rx="2"/><path d="M16 21V5a2 2 0 00-2-2h-4a2 2 0 00-2 2v16"/></svg>
                    </div>
                </div>
                <div class="telem-value">{fmt(data.production_rate)}<span class="telem-unit">units/h</span></div>
            </div>

            <!-- Active Power -->
            <div class="panel" style="padding:20px;">
                <div style="display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:12px;">
                    <span class="section-label">Active Power</span>
                    <div style="width:28px;height:28px;border-radius:7px;background:rgba(59,130,246,0.1);display:flex;align-items:center;justify-content:center;">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#3b82f6" stroke-width="2"><path d="M13 10V3L4 14h7v7l9-11h-7z"/></svg>
                    </div>
                </div>
                <div class="telem-value" style="color:#60a5fa;">{fmt(data.power_kw)}<span class="telem-unit">kW</span></div>
            </div>

            <!-- Energy Intensity (with deviation alert) -->
            <div class="panel" style="padding:20px; {isHigh ? 'border-color:rgba(239,68,68,0.3);' : ''}">
                <div style="display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:12px;">
                    <span class="section-label">Energy Intensity</span>
                    <div style="width:28px;height:28px;border-radius:7px;background:rgba(245,158,11,0.1);display:flex;align-items:center;justify-content:center;">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#f59e0b" stroke-width="2"><rect x="1" y="6" width="22" height="12" rx="2"/><line x1="23" y1="13" x2="23" y2="11"/></svg>
                    </div>
                </div>
                <div class="telem-value" style="color:{isHigh ? '#f87171' : 'white'};">{fmt(data.intensity)}<span class="telem-unit">kWh/unit</span></div>
                <div style="margin-top:10px;font-size:12px;font-weight:600;color:{isHigh ? '#f87171' : 'var(--neon)'};">
                    {isHigh ? '⚠ ' : ''}Deviation: {data.deviation_pct > 0 ? '+' : ''}{fmt(data.deviation_pct)}% vs baseline
                </div>
            </div>
        </div>

        <!-- Chart Panel -->
        <div class="panel" style="padding:20px;">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:16px;">
                <h2 style="font-size:14px;font-weight:600;color:white;margin:0;">Live Energy Intensity</h2>
                <div style="display:flex;align-items:center;gap:6px;">
                    <div class="neon-dot-sm"></div>
                    <span style="font-size:11px;color:rgba(255,255,255,0.3);">Live · kWh/unit</span>
                </div>
            </div>
            <div bind:this={chartContainer} style="width:100%;height:280px;"></div>
        </div>
    {/if}
</div>
