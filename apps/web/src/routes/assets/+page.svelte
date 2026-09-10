<script lang="ts">
    import { onMount } from 'svelte';
    import { liveTelemetry } from '$lib/store';

    interface Asset {
        id: string;
        name: string;
        type: string;
        location: string;
        status: 'ONLINE' | 'OFFLINE' | 'DEGRADED' | 'FAULT';
    }

    let assets = $state<Asset[]>([]);
    let loading = $state(true);
    let loadError = $state('');
    let query = $state('');
    let showFilter = $state(false);
    let filterStatus = $state('ALL');

    async function loadAssets() {
        loading = true;
        loadError = '';
        try {
            const res = await fetch('http://localhost:8080/api/v1/assets');
            if (!res.ok) {
                loadError = 'Edge Core reachable but returned an error.';
                assets = [];
                return;
            }
            const data = await res.json();
            assets = data.assets ?? [];
        } catch (e) {
            loadError = 'Could not reach Edge Core at :8080. Is it running?';
            assets = [];
        } finally {
            loading = false;
        }
    }

    onMount(() => {
        loadAssets();
        const unsub = liveTelemetry.subscribe((msg) => {
            if (msg && msg.type === 'asset.registry.updated') loadAssets();
        });
        return () => unsub();
    });

    let filteredAssets = $derived(assets.filter(a => {
        const matchQ = !query || a.name?.toLowerCase().includes(query.toLowerCase()) || a.id.toLowerCase().includes(query.toLowerCase()) || a.type.toLowerCase().includes(query.toLowerCase());
        const matchS = filterStatus === 'ALL' || a.status === filterStatus;
        return matchQ && matchS;
    }));

    function statusBadgeClass(status: Asset['status']) {
        switch (status) {
            case 'ONLINE':   return 'badge badge-online';
            case 'DEGRADED': return 'badge badge-degraded';
            case 'FAULT':    return 'badge badge-fault';
            default:         return 'badge badge-offline';
        }
    }
    function statusDotColor(status: Asset['status']) {
        switch (status) {
            case 'ONLINE':   return 'var(--neon)';
            case 'DEGRADED': return '#f59e0b';
            case 'FAULT':    return '#ef4444';
            default:         return '#555';
        }
    }
    function typeIcon(type: string) {
        if (type.includes('pump')) return 'M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12';
        if (type.includes('motor')) return 'M12 4c1.657 0 3 .895 3 2s-1.343 2-3 2-3-.895-3-2 1.343-2 3-2zM6 20v-2a4 4 0 014-4h4a4 4 0 014 4v2';
        if (type.includes('vfd') || type.includes('drive')) return 'M13 10V3L4 14h7v7l9-11h-7z';
        if (type.includes('compressor')) return 'M9.663 17h4.673M12 3v1m6.364 1.636l-.707.707M21 12h-1M4 12H3m3.343-5.657l-.707-.707m2.828 9.9a5 5 0 117.072 0l-.548.547A3.374 3.374 0 0014 18.469V19a2 2 0 11-4 0v-.531c0-.895-.356-1.754-.988-2.386l-.548-.547z';
        if (type.includes('sensor')) return 'M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z';
        if (type.includes('meter')) return 'M13 10V3L4 14h7v7l9-11h-7z';
        if (type.includes('plc') || type.includes('pac')) return 'M9 3H5a2 2 0 00-2 2v4m6-6h10a2 2 0 012 2v4M9 3v18m0 0h10a2 2 0 002-2V9M9 21H5a2 2 0 01-2-2V9m0 0h18';
        return 'M9 3H5a2 2 0 00-2 2v4m6-6h10a2 2 0 012 2v4M9 3v18';
    }
</script>

<svelte:head>
    <title>Assets — CTM-1</title>
</svelte:head>

<div style="height:100%; overflow-y:auto; padding: 28px 32px;">
    <!-- Header -->
    <div style="display:flex; align-items:flex-start; justify-content:space-between; margin-bottom:24px;">
        <div>
            <h1 style="font-size:1.75rem; font-weight:700; color:white; letter-spacing:-0.02em; margin:0;">Plant Assets</h1>
            <p style="color:rgba(255,255,255,0.4); font-size:13px; margin:4px 0 0;">Manage and monitor all connected equipment.</p>
        </div>
        <a href="/topology" class="btn btn-neon" style="text-decoration:none;" id="create-asset-btn">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>
            Create new
        </a>
    </div>

    <!-- Search & Filter bar -->
    <div style="display:flex; gap:10px; margin-bottom:20px; align-items:center;">
        <div style="position:relative; flex:1; max-width:320px;">
            <svg style="position:absolute;left:10px;top:50%;transform:translateY(-50%);opacity:0.4;" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
            <input
                type="text"
                bind:value={query}
                placeholder="Search by name or type..."
                class="industrial-input"
                style="width:100%; padding-left:32px;"
            />
        </div>

        <!-- Filter dropdown toggle -->
        <div style="position:relative;">
            <button
                class="btn btn-ghost"
                onclick={() => showFilter = !showFilter}
                id="filter-assets-btn"
            >
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="22 3 2 3 10 12.46 10 19 14 21 14 12.46 22 3"/></svg>
                Filter {filterStatus !== 'ALL' ? `· ${filterStatus}` : ''}
            </button>
            {#if showFilter}
                <div style="position:absolute;top:calc(100% + 6px);left:0;background:#0d0d12;border:1px solid rgba(255,255,255,0.1);border-radius:8px;padding:6px;z-index:100;min-width:140px;">
                    {#each ['ALL', 'ONLINE', 'OFFLINE', 'DEGRADED', 'FAULT'] as s}
                        <button
                            class="btn {filterStatus === s ? 'btn-outline-neon' : 'btn-ghost'}"
                            style="width:100%; justify-content:flex-start; margin-bottom:2px; font-size:12px;"
                            onclick={() => { filterStatus = s; showFilter = false; }}
                        >
                            {s}
                        </button>
                    {/each}
                </div>
            {/if}
        </div>

        <button class="btn btn-ghost" onclick={loadAssets} id="refresh-assets-btn" title="Refresh">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="23 4 23 10 17 10"/><path d="M20.49 15a9 9 0 11-2.12-9.36L23 10"/></svg>
        </button>

        <span style="font-size:12px; color:rgba(255,255,255,0.3); margin-left:auto;">{filteredAssets.length} asset{filteredAssets.length === 1 ? '' : 's'}</span>
    </div>

    {#if loadError}
        <div style="display:flex;align-items:center;gap:10px;padding:12px 16px;border-radius:8px;border:1px solid rgba(239,68,68,0.3);background:rgba(239,68,68,0.06);color:#f87171;font-size:13px;margin-bottom:16px;">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z"/></svg>
            {loadError}
        </div>
    {/if}

    <!-- Assets Table -->
    <div class="panel" style="overflow:hidden;">
        <table style="width:100%; border-collapse:collapse; font-size:13px;">
            <thead>
                <tr style="border-bottom:1px solid rgba(255,255,255,0.06);">
                    <th style="padding:12px 20px; text-align:left; font-size:10px; font-weight:700; letter-spacing:0.08em; text-transform:uppercase; color:rgba(255,255,255,0.3);">Asset</th>
                    <th style="padding:12px 20px; text-align:left; font-size:10px; font-weight:700; letter-spacing:0.08em; text-transform:uppercase; color:rgba(255,255,255,0.3);">Type</th>
                    <th style="padding:12px 20px; text-align:left; font-size:10px; font-weight:700; letter-spacing:0.08em; text-transform:uppercase; color:rgba(255,255,255,0.3);">Location</th>
                    <th style="padding:12px 20px; text-align:left; font-size:10px; font-weight:700; letter-spacing:0.08em; text-transform:uppercase; color:rgba(255,255,255,0.3);">Status</th>
                    <th style="padding:12px 20px; text-align:right; font-size:10px; font-weight:700; letter-spacing:0.08em; text-transform:uppercase; color:rgba(255,255,255,0.3);">Actions</th>
                </tr>
            </thead>
            <tbody>
                {#if loading}
                    <tr>
                        <td colspan="5" style="padding:48px; text-align:center; color:rgba(255,255,255,0.25);">
                            <div style="display:flex;align-items:center;justify-content:center;gap:8px;">
                                <div style="width:14px;height:14px;border:2px solid rgba(0,230,118,0.4);border-top-color:var(--neon);border-radius:50%;animation:spin 0.8s linear infinite;"></div>
                                Loading assets...
                            </div>
                        </td>
                    </tr>
                {:else if filteredAssets.length === 0}
                    <tr>
                        <td colspan="5" style="padding:48px; text-align:center; color:rgba(255,255,255,0.2); font-size:13px;">
                            No assets found.
                            {#if !loadError}<a href="/topology" style="color:var(--neon);text-decoration:none;">Build one in Topology →</a>{/if}
                        </td>
                    </tr>
                {:else}
                    {#each filteredAssets as asset (asset.id)}
                        <tr style="border-bottom:1px solid rgba(255,255,255,0.04); transition:background 0.15s; cursor:pointer;" onmouseover={(e) => (e.currentTarget as HTMLElement).style.background='rgba(255,255,255,0.03)'} onmouseout={(e) => (e.currentTarget as HTMLElement).style.background='transparent'}>
                            <td style="padding:14px 20px;">
                                <div style="display:flex;align-items:center;gap:10px;">
                                    <div style="width:32px;height:32px;border-radius:7px;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.07);display:flex;align-items:center;justify-content:center;flex-shrink:0;">
                                        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="rgba(255,255,255,0.5)" stroke-width="1.75">
                                            <path d={typeIcon(asset.type)}/>
                                        </svg>
                                    </div>
                                    <span style="font-weight:600; color:white;">{asset.name || asset.id}</span>
                                </div>
                            </td>
                            <td style="padding:14px 20px; color:rgba(255,255,255,0.5); font-family:var(--font-mono); font-size:11px; text-transform:uppercase;">{asset.type}</td>
                            <td style="padding:14px 20px; color:rgba(255,255,255,0.45);">{asset.location || '—'}</td>
                            <td style="padding:14px 20px;">
                                <div style="display:flex;align-items:center;gap:6px;">
                                    <div style="width:6px;height:6px;border-radius:50%;background:{statusDotColor(asset.status)};flex-shrink:0;box-shadow:0 0 6px {statusDotColor(asset.status)};"></div>
                                    <span class={statusBadgeClass(asset.status)}>{asset.status}</span>
                                </div>
                            </td>
                            <td style="padding:14px 20px; text-align:right;">
                                <a href={`/assets/${asset.id}`} class="btn btn-ghost" style="font-size:11px;padding:4px 10px;text-decoration:none;">
                                    View Details
                                </a>
                            </td>
                        </tr>
                    {/each}
                {/if}
            </tbody>
        </table>
    </div>
</div>

<style>
@keyframes spin { to { transform: rotate(360deg); } }
</style>
