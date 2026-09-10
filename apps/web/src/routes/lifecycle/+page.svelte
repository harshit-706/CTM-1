<script lang="ts">
    import { onMount } from 'svelte';
    import { CheckCircle2, Loader2, Package } from 'lucide-svelte';

    let assets = $state<any[]>([]);
    let loading = $state(true);
    let targetAsset = $derived(assets.length > 0 ? assets[0] : null);

    type State = 'ELIGIBLE' | 'PRECHECK' | 'SCHEDULED' | 'VERIFYING' | 'COMPLETED';
    let updateState = $state<State>('ELIGIBLE');
    let isProcessing = $state(false);

    const states: State[] = ['ELIGIBLE', 'PRECHECK', 'SCHEDULED', 'VERIFYING', 'COMPLETED'];

    onMount(async () => {
        try {
            const res = await fetch('http://localhost:8080/api/v1/assets');
            if (res.ok) {
                const data = await res.json();
                assets = data.assets || [];
            }
        } catch (e) {
            console.error(e);
        } finally {
            loading = false;
        }
    });

    async function advanceState(action: string) {
        isProcessing = true;
        await new Promise(r => setTimeout(r, 1400));
        if (action === 'PRECHECK') updateState = 'PRECHECK';
        else if (action === 'SCHEDULE') updateState = 'SCHEDULED';
        else if (action === 'EXECUTE') {
            updateState = 'VERIFYING';
            setTimeout(() => updateState = 'COMPLETED', 3000);
        }
        isProcessing = false;
    }

    function resetState() {
        updateState = 'ELIGIBLE';
        isProcessing = false;
    }

    function stateIndex(s: State) { return states.indexOf(s); }
    let currentIdx = $derived(stateIndex(updateState));
</script>

<svelte:head>
    <title>Lifecycle Intelligence — CTM-1</title>
</svelte:head>

<div style="height:100%; overflow-y:auto; padding:28px 32px;">
    <!-- Header -->
    <div style="margin-bottom:24px;">
        <h1 style="font-size:1.75rem; font-weight:700; color:white; letter-spacing:-0.02em; margin:0;">Lifecycle Intelligence</h1>
        <p style="color:rgba(255,255,255,0.4); font-size:13px; margin:4px 0 0;">Manage firmware, configurations, and maintenance via state-machine orchestration.</p>
    </div>

    <!-- Firmware Updates Card -->
    <div class="panel" style="margin-bottom:16px; overflow:hidden;">
        <div style="padding:18px 20px; border-bottom:1px solid rgba(255,255,255,0.06); display:flex;align-items:center;justify-content:space-between;">
            <div style="display:flex;align-items:center;gap:8px;">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="rgba(255,255,255,0.5)" stroke-width="2"><path d="M21 15v4a2 2 0 01-2 2H5a2 2 0 01-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
                <h2 style="font-size:14px;font-weight:600;color:white;margin:0;">Firmware Updates</h2>
            </div>
            {#if loading}
                <span style="font-size:12px;color:rgba(255,255,255,0.3);">Loading...</span>
            {:else if targetAsset}
                <span class="badge badge-building">1 Pending Action</span>
            {:else}
                <span class="badge badge-online">Up to date</span>
            {/if}
        </div>

        <div style="padding:24px;">
            {#if loading}
                <div style="text-align:center;padding:32px;color:rgba(255,255,255,0.25);font-size:13px;display:flex;align-items:center;justify-content:center;gap:8px;">
                    <div style="width:14px;height:14px;border:2px solid rgba(0,230,118,0.3);border-top-color:var(--neon);border-radius:50%;animation:spin 0.8s linear infinite;"></div>
                    Loading lifecycle data...
                </div>
            {:else if !targetAsset}
                <div style="text-align:center;padding:32px;color:rgba(255,255,255,0.2);font-size:13px;">No assets configured. <a href="/topology" style="color:var(--neon);text-decoration:none;">Build topology first →</a></div>
            {:else}
                <div style="display:flex;gap:32px;">
                    <!-- Left: Update Info + Actions -->
                    <div style="flex:1;">
                        <!-- Update Target -->
                        <div style="display:flex;align-items:flex-start;gap:14px;margin-bottom:20px;">
                            <div style="width:40px;height:40px;border-radius:10px;background:rgba(59,130,246,0.12);border:1px solid rgba(59,130,246,0.25);display:flex;align-items:center;justify-content:center;flex-shrink:0;">
                                <Package size={18} color="#3b82f6" />
                            </div>
                            <div>
                                <h3 style="font-size:15px;font-weight:600;color:white;margin:0 0 6px;">{targetAsset.name} Controller</h3>
                                <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px 24px;">
                                    <div style="font-size:12px;color:rgba(255,255,255,0.4);">Current: <span style="color:white;font-family:var(--font-mono);">v1.2.0</span></div>
                                    <div style="font-size:12px;color:rgba(255,255,255,0.4);">Target: <span style="color:var(--neon);font-family:var(--font-mono);font-weight:600;">v1.5.0-LTS</span></div>
                                    <div style="font-size:12px;color:rgba(255,255,255,0.4);">Risk: <span style="color:#f59e0b;">Medium</span></div>
                                    <div style="font-size:12px;color:rgba(255,255,255,0.4);">Est. downtime: <span style="color:white;">45 s</span></div>
                                </div>
                            </div>
                        </div>

                        <!-- Orchestration Actions -->
                        <div style="padding:16px;border-radius:8px;border:1px solid rgba(255,255,255,0.07);background:rgba(0,0,0,0.3);">
                            <div class="section-label" style="margin-bottom:12px;">Orchestration Actions</div>
                            <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
                                {#if updateState === 'ELIGIBLE'}
                                    <button
                                        class="btn btn-ghost"
                                        onclick={() => advanceState('PRECHECK')}
                                        disabled={isProcessing}
                                        id="run-precheck-btn"
                                    >
                                        {#if isProcessing}
                                            <Loader2 size={13} class="animate-spin" />
                                            Running Pre-checks...
                                        {:else}
                                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="9 11 12 14 22 4"/><path d="M21 12v7a2 2 0 01-2 2H5a2 2 0 01-2-2V5a2 2 0 012-2h11"/></svg>
                                            Run Pre-checks
                                        {/if}
                                    </button>
                                {:else if updateState === 'PRECHECK'}
                                    <div class="badge badge-online" style="margin-right:4px;">✓ Pre-checks passed</div>
                                    <button
                                        class="btn btn-outline-neon"
                                        onclick={() => advanceState('SCHEDULE')}
                                        disabled={isProcessing}
                                        id="schedule-maintenance-btn"
                                    >
                                        <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="4" width="18" height="18" rx="2"/><line x1="16" y1="2" x2="16" y2="6"/><line x1="8" y1="2" x2="8" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/></svg>
                                        Schedule Maintenance
                                    </button>
                                {:else if updateState === 'SCHEDULED'}
                                    <div class="badge badge-building" style="margin-right:4px;">Scheduled</div>
                                    <button
                                        class="btn btn-danger"
                                        onclick={() => advanceState('EXECUTE')}
                                        disabled={isProcessing}
                                        id="execute-flash-btn"
                                    >
                                        {#if isProcessing}
                                            <Loader2 size={13} class="animate-spin" /> Executing...
                                        {:else}
                                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="5 3 19 12 5 21 5 3"/></svg>
                                            Execute Flash Now
                                        {/if}
                                    </button>
                                {:else if updateState === 'VERIFYING'}
                                    <div style="display:flex;align-items:center;gap:8px;color:#f59e0b;font-size:13px;">
                                        <Loader2 size={14} class="animate-spin" />
                                        Verifying checksums and run state...
                                    </div>
                                {:else if updateState === 'COMPLETED'}
                                    <div style="display:flex;align-items:center;gap:8px;color:var(--neon);font-size:13px;">
                                        <CheckCircle2 size={14} />
                                        Update completed successfully
                                    </div>
                                    <button class="btn btn-ghost" onclick={resetState} style="margin-left:8px;" id="reset-lifecycle-btn">
                                        Reset
                                    </button>
                                {/if}
                            </div>
                        </div>
                    </div>

                    <!-- Right: State Machine Visualizer -->
                    <div style="width:200px;border-left:1px solid rgba(255,255,255,0.06);padding-left:24px;flex-shrink:0;">
                        <div class="section-label" style="margin-bottom:14px;">State Machine</div>
                        <div style="display:flex;flex-direction:column;gap:0;">
                            {#each states as state, i}
                                {@const isCurrent = updateState === state}
                                {@const isPast = currentIdx > i}
                                <div style="display:flex;align-items:center;gap:10px;position:relative;">
                                    <div style="
                                        width:10px;height:10px;border-radius:50%;flex-shrink:0;
                                        background:{isCurrent ? 'var(--neon)' : isPast ? 'rgba(0,230,118,0.4)' : 'rgba(255,255,255,0.1)'};
                                        box-shadow:{isCurrent ? '0 0 10px var(--neon), 0 0 20px var(--neon-glow)' : 'none'};
                                        transition:all 0.3s;
                                    "></div>
                                    <span style="font-size:11px;font-weight:{isCurrent ? '700' : '400'};color:{isCurrent ? 'var(--neon)' : isPast ? 'rgba(0,230,118,0.5)' : 'rgba(255,255,255,0.25)'};font-family:var(--font-mono);transition:color 0.3s;">
                                        {state}
                                    </span>
                                </div>
                                {#if i < states.length - 1}
                                    <div style="width:1px;height:20px;margin-left:4px;background:{isPast ? 'rgba(0,230,118,0.3)' : 'rgba(255,255,255,0.07)'};transition:background 0.3s;"></div>
                                {/if}
                            {/each}
                        </div>
                    </div>
                </div>
            {/if}
        </div>
    </div>
</div>

<style>
@keyframes spin { to { transform: rotate(360deg); } }
</style>
