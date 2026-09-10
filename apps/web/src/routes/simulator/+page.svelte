<script lang="ts">
    import { onMount, onDestroy } from 'svelte';

    const SIM_API = 'http://localhost:8082';

    let assets = $state<any[]>([]);
    let selectedId = $state<string>('');
    let assetState = $state<any>(null);
    let scenarios = $state<any>({ available: [], active: {} });
    let events = $state<any[]>([]);
    let edgeOnline = $state(false);
    let filterQuery = $state('');
    let filterType = $state('all');

    let pollInterval: ReturnType<typeof setInterval>;

    let selectedAsset = $derived(assets.find(a => a.id === selectedId));
    let filteredAssets = $derived(assets.filter(a => {
        const mq = !filterQuery || a.id.toLowerCase().includes(filterQuery.toLowerCase()) || a.label?.toLowerCase().includes(filterQuery.toLowerCase());
        const mt = filterType === 'all' || a.type === filterType;
        return mq && mt;
    }));
    let assetTypes = $derived([...new Set(assets.map(a => a.type))]);

    async function fetchJson(url: string) {
        try {
            const res = await fetch(url);
            if (res.ok) return await res.json();
        } catch { /* swallow */ }
        return null;
    }
    async function postJson(url: string, body: any = {}) {
        try {
            const res = await fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
            if (res.ok) return await res.json();
        } catch { /* swallow */ }
        return null;
    }

    async function poll() {
        const [aList, health, sc, ev] = await Promise.all([
            fetchJson(`${SIM_API}/api/v1/assets`),
            fetchJson(`${SIM_API}/health`),
            fetchJson(`${SIM_API}/api/v1/scenarios`),
            fetchJson(`${SIM_API}/api/v1/events`),
        ]);
        if (aList) assets = aList;
        if (health) edgeOnline = health.edge_online;
        if (sc) scenarios = sc;
        if (ev) events = ev;
        if (selectedId) {
            const st = await fetchJson(`${SIM_API}/api/v1/assets/${selectedId}`);
            if (st) assetState = st;
        }
    }

    onMount(() => { poll(); pollInterval = setInterval(poll, 1000); });
    onDestroy(() => { if (pollInterval) clearInterval(pollInterval); });

    async function sendCmd(cmd: string, extra: any = {}) {
        if (!selectedId) return;
        await postJson(`${SIM_API}/api/v1/control`, { asset_id: selectedId, command: cmd, ...extra });
        setTimeout(poll, 200);
    }
    async function startScenario(id: string) {
        if (!selectedId) return;
        await postJson(`${SIM_API}/api/v1/scenarios/${id}/start`, { target: selectedId });
        setTimeout(poll, 200);
    }
    async function stopScenario(id: string) {
        await postJson(`${SIM_API}/api/v1/scenarios/${id}/stop`, {});
        setTimeout(poll, 200);
    }
    async function globalReset() {
        await postJson(`${SIM_API}/api/v1/reset`);
        setTimeout(poll, 200);
    }

    function statusColor(running: boolean, faults: string[]) {
        if (faults.length > 0) return '#ef4444';
        return running ? 'var(--neon)' : '#555';
    }
    function statusLabel(running: boolean, faults: string[]) {
        if (faults.length > 0) return 'FAULT';
        return running ? 'RUNNING' : 'STOPPED';
    }
    function formatTag(k: string) { return k.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase()); }
    function tagUnit(k: string) {
        if (k.includes('_hz') || k.includes('freq')) return 'Hz';
        if (k.includes('_a') || k.includes('current')) return 'A';
        if (k.includes('_kw')) return 'kW';
        if (k.includes('kwh')) return 'kWh';
        if (k.includes('_bar') || k.includes('pressure')) return 'bar';
        if (k.includes('_v') || k.includes('voltage')) return 'V';
        if (k.includes('_c') || k.includes('temp')) return '°C';
        if (k.includes('vibration')) return 'mm/s';
        if (k.includes('flow')) return 'm³/h';
        if (k.includes('rpm')) return 'RPM';
        return '';
    }
    function tagColor(k: string) {
        if (k.includes('current') || k.includes('power') || k.includes('voltage')) return '#3b82f6';
        if (k.includes('temp')) return '#f59e0b';
        if (k.includes('flow') || k.includes('pressure')) return '#06b6d4';
        if (k.includes('vibration')) return '#a855f7';
        if (k.includes('speed') || k.includes('freq') || k.includes('rpm')) return 'var(--neon)';
        return 'rgba(255,255,255,0.75)';
    }
</script>

<svelte:head>
    <title>Simulator Control Room — CTM-1</title>
</svelte:head>

<div style="height:100%; display:flex; flex-direction:column; overflow:hidden;">
    <!-- Header bar -->
    <div style="padding:12px 20px; border-bottom:1px solid rgba(255,255,255,0.06); display:flex;align-items:center;justify-content:space-between;flex-shrink:0;background:rgba(6,6,10,0.8);backdrop-filter:blur(10px);">
        <div>
            <h1 style="font-size:1rem;font-weight:700;color:white;margin:0;">Virtual Plant Simulator</h1>
            <p style="font-size:11px;color:rgba(255,255,255,0.3);margin:2px 0 0;">Engineering control surface — NOT operator HMI</p>
        </div>
        <div style="display:flex;align-items:center;gap:12px;">
            <div style="display:flex;align-items:center;gap:6px;">
                {#if edgeOnline}
                    <div class="neon-dot-sm"></div>
                    <span style="font-size:11px;font-weight:600;color:var(--neon);">EDGE CONNECTED</span>
                {:else}
                    <div style="width:6px;height:6px;border-radius:50%;background:#ef4444;box-shadow:0 0 6px rgba(239,68,68,0.6);"></div>
                    <span style="font-size:11px;font-weight:600;color:#f87171;">EDGE DISCONNECTED</span>
                {/if}
            </div>
            <span style="font-size:11px;color:rgba(255,255,255,0.25);">{assets.length} assets</span>
            <button
                onclick={globalReset}
                class="btn btn-ghost"
                style="font-size:11px;padding:5px 10px;"
                id="global-reset-btn"
            >
                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="1 4 1 10 7 10"/><path d="M3.51 15a9 9 0 102.13-9.36L1 10"/></svg>
                Reset All
            </button>
            <span class="badge badge-simulated">ENGINEERING SURFACE</span>
        </div>
    </div>

    <!-- 3-column body -->
    <div style="flex:1;display:flex;overflow:hidden;">
        <!-- Left: Asset Inventory -->
        <div style="width:240px;border-right:1px solid rgba(255,255,255,0.06);display:flex;flex-direction:column;flex-shrink:0;background:rgba(6,6,10,0.6);">
            <div style="padding:10px;border-bottom:1px solid rgba(255,255,255,0.05);display:flex;flex-direction:column;gap:6px;">
                <input type="text" placeholder="Search assets..." bind:value={filterQuery}
                    class="industrial-input" style="font-size:11px;padding:6px 10px;" />
                <select bind:value={filterType} class="industrial-input industrial-select" style="font-size:11px;padding:5px 10px;">
                    <option value="all">All Types</option>
                    {#each assetTypes as t}
                        <option value={t}>{t.replace(/_/g, ' ').toUpperCase()}</option>
                    {/each}
                </select>
            </div>
            <div style="flex:1;overflow-y:auto;">
                {#each filteredAssets as a}
                    {@const color = statusColor(a.running, a.faults || [])}
                    <button
                        onclick={() => { selectedId = a.id; }}
                        style="width:100%;text-align:left;padding:10px 12px;border-bottom:1px solid rgba(255,255,255,0.03);transition:background 0.12s;background:{selectedId === a.id ? 'rgba(0,230,118,0.06)' : 'transparent'};border-left:{selectedId === a.id ? '2px solid var(--neon)' : '2px solid transparent'};"
                        onmouseover={(e) => { if (selectedId !== a.id) (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.03)'; }}
                        onmouseout={(e) => { if (selectedId !== a.id) (e.currentTarget as HTMLElement).style.background = 'transparent'; }}
                    >
                        <div style="display:flex;align-items:center;gap:7px;margin-bottom:3px;">
                            <div style="width:6px;height:6px;border-radius:50%;background:{color};box-shadow:0 0 5px {color};flex-shrink:0;"></div>
                            <span style="font-size:12px;font-weight:600;color:{selectedId === a.id ? 'white' : 'rgba(255,255,255,0.75)'};font-family:var(--font-mono);">{a.id}</span>
                        </div>
                        <div style="display:flex;justify-content:space-between;padding-left:13px;">
                            <span style="font-size:10px;color:rgba(255,255,255,0.3);text-transform:uppercase;">{a.type?.replace(/_/g, ' ')}</span>
                            <span style="font-size:10px;font-weight:700;color:{color};">{statusLabel(a.running, a.faults || [])}</span>
                        </div>
                    </button>
                {/each}
                {#if filteredAssets.length === 0}
                    <div style="padding:24px 12px;text-align:center;color:rgba(255,255,255,0.2);font-size:12px;">
                        No assets found.<br/>
                        <a href="/topology" style="color:var(--neon);text-decoration:none;font-size:11px;">Publish topology first →</a>
                    </div>
                {/if}
            </div>
        </div>

        <!-- Center: Asset Detail -->
        <div style="flex:1;overflow-y:auto;">
            {#if selectedId && assetState}
                {@const hasFaults = assetState.faults?.length > 0}
                <div style="padding:20px;display:flex;flex-direction:column;gap:16px;">
                    <!-- Identity -->
                    <div style="display:flex;align-items:center;justify-content:space-between;">
                        <div>
                            <h2 style="font-size:1.1rem;font-weight:700;color:white;margin:0;font-family:var(--font-mono);">{assetState.asset_id}</h2>
                            <p style="font-size:12px;color:rgba(255,255,255,0.35);margin:3px 0 0;">{assetState.label} · {assetState.type?.replace(/_/g, ' ').toUpperCase()}</p>
                        </div>
                        <div style="display:flex;align-items:center;gap:8px;">
                            <span class="badge badge-simulated">SIMULATED</span>
                            <div style="display:flex;align-items:center;gap:6px;padding:5px 10px;border-radius:20px;border:1px solid {hasFaults ? 'rgba(239,68,68,0.3)' : assetState.running ? 'rgba(0,230,118,0.25)' : 'rgba(255,255,255,0.1)'};background:{hasFaults ? 'rgba(239,68,68,0.08)' : assetState.running ? 'rgba(0,230,118,0.08)' : 'rgba(255,255,255,0.04)'};">
                                <div style="width:6px;height:6px;border-radius:50%;background:{statusColor(assetState.running, assetState.faults || [])};{assetState.running && !hasFaults ? 'animation:pulse-neon 2s infinite;' : ''}"></div>
                                <span style="font-size:11px;font-weight:700;color:{statusColor(assetState.running, assetState.faults || [])};">{statusLabel(assetState.running, assetState.faults || [])}</span>
                            </div>
                        </div>
                    </div>

                    <!-- Active Faults -->
                    {#if assetState.faults?.length > 0}
                        <div style="padding:12px 14px;border-radius:8px;border:1px solid rgba(239,68,68,0.25);background:rgba(239,68,68,0.05);">
                            <div style="display:flex;align-items:center;gap:6px;color:#f87171;font-size:12px;font-weight:600;margin-bottom:8px;">
                                <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z"/></svg>
                                Active Faults
                            </div>
                            <div style="display:flex;flex-wrap:wrap;gap:6px;">
                                {#each assetState.faults as f}
                                    <span class="badge badge-fault" style="font-family:var(--font-mono);">{f}</span>
                                {/each}
                            </div>
                        </div>
                    {/if}

                    <!-- Live Telemetry Grid -->
                    <div>
                        <div class="section-label" style="margin-bottom:10px;">Live Telemetry</div>
                        <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:10px;">
                            {#each Object.entries(assetState.tags || {}).filter(([k]) => !['status','quality'].includes(k)) as [key, value]}
                                <div class="telem-card">
                                    <div class="telem-label">{formatTag(key)}</div>
                                    <div class="telem-value" style="font-size:1.25rem;color:{tagColor(key)};">
                                        {typeof value === 'number' ? (value as number).toFixed(1) : value}
                                        <span class="telem-unit">{tagUnit(key)}</span>
                                    </div>
                                </div>
                            {/each}
                        </div>
                    </div>

                    {#if assetState.last_update}
                        <div style="font-size:10px;color:rgba(255,255,255,0.2);font-family:var(--font-mono);">
                            Last update: {new Date(assetState.last_update * 1000).toLocaleTimeString()} · Edge: {edgeOnline ? 'CONNECTED' : 'DISCONNECTED'}
                        </div>
                    {/if}
                </div>
            {:else}
                <div style="display:flex;align-items:center;justify-content:center;height:100%;flex-direction:column;gap:12px;color:rgba(255,255,255,0.2);">
                    <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1" opacity="0.3"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>
                    <p style="font-size:13px;margin:0;">Select an asset to view live telemetry</p>
                </div>
            {/if}
        </div>

        <!-- Right: Controls & Scenarios -->
        <div style="width:260px;border-left:1px solid rgba(255,255,255,0.06);display:flex;flex-direction:column;flex-shrink:0;overflow-y:auto;background:rgba(6,6,10,0.6);">
            {#if selectedId}
                <div style="padding:14px;display:flex;flex-direction:column;gap:14px;">
                    <!-- Start / Stop / Reset -->
                    <div>
                        <div class="section-label" style="margin-bottom:8px;">Asset Control</div>
                        <div style="display:flex;gap:6px;">
                            <button onclick={() => sendCmd('start')} class="btn btn-start" style="flex:1;font-size:12px;padding:7px;" id="start-asset-btn">
                                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="5 3 19 12 5 21 5 3"/></svg>
                                START
                            </button>
                            <button onclick={() => sendCmd('stop')} class="btn btn-stop" style="flex:1;font-size:12px;padding:7px;" id="stop-asset-btn">
                                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/></svg>
                                STOP
                            </button>
                            <button onclick={() => sendCmd('reset')} class="btn btn-ghost" style="padding:7px 10px;" id="reset-asset-btn" title="Reset">
                                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="1 4 1 10 7 10"/><path d="M3.51 15a9 9 0 102.13-9.36L1 10"/></svg>
                            </button>
                        </div>
                    </div>

                    <!-- Speed Inputs -->
                    {#if selectedAsset?.inputs}
                        <div>
                            <div class="section-label" style="margin-bottom:8px;">Operating Inputs</div>
                            {#each Object.entries(selectedAsset.inputs) as [inputKey, spec]}
                                {@const s = spec as any}
                                <div style="margin-bottom:12px;">
                                    <div style="display:flex;justify-content:space-between;font-size:11px;margin-bottom:4px;">
                                        <span style="color:rgba(255,255,255,0.45);">{formatTag(inputKey)}</span>
                                        <span style="color:white;font-family:var(--font-mono);">{assetState?.tags?.[inputKey]?.toFixed?.(1) ?? s.default} {s.unit}</span>
                                    </div>
                                    <div style="display:flex;align-items:center;gap:6px;">
                                        <span style="font-size:10px;color:rgba(255,255,255,0.2);">{s.min}</span>
                                        <input type="range" min={s.min} max={s.max} step={s.step}
                                            value={assetState?.[inputKey] ?? s.default}
                                            onchange={(e) => sendCmd('set_speed', { speed: parseFloat((e.target as HTMLInputElement).value) })}
                                            style="flex:1;" />
                                        <span style="font-size:10px;color:rgba(255,255,255,0.2);">{s.max}</span>
                                    </div>
                                </div>
                            {/each}
                        </div>
                    {/if}

                    <!-- Scenarios -->
                    <div>
                        <div class="section-label" style="margin-bottom:8px;">Scenarios</div>
                        <div style="display:flex;flex-direction:column;gap:4px;">
                            {#each scenarios.available || [] as sc}
                                {@const isActive = sc.id in (scenarios.active || {})}
                                <button
                                    onclick={() => isActive ? stopScenario(sc.id) : startScenario(sc.id)}
                                    class="btn {isActive ? 'btn-amber' : 'btn-ghost'}"
                                    style="justify-content:space-between;font-size:11px;padding:7px 10px;"
                                    id="scenario-{sc.id}"
                                >
                                    <span>{sc.label}</span>
                                    {#if isActive}
                                        <span style="font-family:var(--font-mono);font-size:10px;">{scenarios.active[sc.id]?.elapsed_s?.toFixed(0)}s</span>
                                    {/if}
                                </button>
                            {/each}
                        </div>
                    </div>

                    <!-- Fault Injection -->
                    <div>
                        <div class="section-label" style="margin-bottom:8px;">Fault Injection</div>
                        <div style="display:grid;grid-template-columns:1fr 1fr;gap:5px;">
                            {#each [
                                { fault: 'overcurrent', label: 'Overcurrent', color: '#ef4444' },
                                { fault: 'vfd_trip', label: 'VFD Trip', color: '#ef4444' },
                                { fault: 'vibration_drift', label: 'Vibration', color: '#f59e0b' },
                                { fault: 'energy_waste', label: 'Energy Waste', color: '#f59e0b' },
                                { fault: 'comm_loss', label: 'Comm Loss', color: '#f97316' },
                                { fault: 'sensor_failure', label: 'Sensor Fail', color: '#f97316' },
                            ] as fi}
                                <button
                                    onclick={() => sendCmd('inject_fault', { fault: fi.fault })}
                                    id="fault-{fi.fault}"
                                    style="padding:7px 8px;border-radius:6px;border:1px solid {fi.color}33;background:{fi.color}14;color:{fi.color};font-size:10px;font-weight:600;cursor:pointer;transition:all 0.15s;font-family:var(--font-sans);"
                                    onmouseover={(e) => (e.currentTarget as HTMLElement).style.background = fi.color + '28'}
                                    onmouseout={(e) => (e.currentTarget as HTMLElement).style.background = fi.color + '14'}
                                >
                                    {fi.label}
                                </button>
                            {/each}
                        </div>
                    </div>
                </div>
            {:else}
                <div style="display:flex;align-items:center;justify-content:center;height:100%;color:rgba(255,255,255,0.2);font-size:12px;text-align:center;padding:20px;">
                    Select an asset to access controls and scenarios
                </div>
            {/if}
        </div>
    </div>

    <!-- Event Log (terminal) -->
    <div class="terminal" style="height:120px;flex-shrink:0;border-radius:0;border-left:none;border-right:none;border-bottom:none;display:flex;flex-direction:column;">
        <div style="padding:6px 14px;border-bottom:1px solid rgba(0,230,118,0.1);display:flex;align-items:center;justify-content:space-between;flex-shrink:0;">
            <span style="font-size:10px;font-weight:700;letter-spacing:0.1em;color:rgba(0,230,118,0.5);">▶ EVENT LOG</span>
            <span style="font-size:10px;color:rgba(0,230,118,0.3);">{events.length} events</span>
        </div>
        <div style="flex:1;overflow-y:auto;padding:6px 14px;display:flex;flex-direction:column-reverse;">
            <div>
                {#each events.toReversed() as ev}
                    <div style="padding:1px 0;">
                        <span style="color:rgba(0,230,118,0.4);">{ev.ts}</span>
                        <span style="color:rgba(0,230,118,0.8);margin-left:8px;">{ev.msg}</span>
                    </div>
                {/each}
                {#if events.length === 0}
                    <div style="color:rgba(0,230,118,0.2);">[System] Waiting for events...</div>
                {/if}
            </div>
        </div>
    </div>
</div>
