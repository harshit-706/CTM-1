<script lang="ts">
    import { incidents } from '$lib/store';
    import { goto } from '$app/navigation';

    let incidentsList = $derived(Object.values($incidents));
    let selectedId = $state<string | null>(null);

    function investigate(incident: any) {
        goto(`/assets/${incident.asset_id}`);
    }

    function acknowledgeIncident(id: string) {
        // In a real system, would POST to edge core
        incidents.update(store => {
            if (store[id]) {
                store[id].acknowledged = true;
            }
            return store;
        });
    }
</script>

<svelte:head>
    <title>Incidents — CTM-1</title>
</svelte:head>

<div style="height:100%; overflow-y:auto; padding:28px 32px;">
    <!-- Header -->
    <div style="display:flex; align-items:flex-start; justify-content:space-between; margin-bottom:24px;">
        <div>
            <h1 style="font-size:1.75rem; font-weight:700; color:white; letter-spacing:-0.02em; margin:0;">Incident Correlation</h1>
            <p style="color:rgba(255,255,255,0.4); font-size:13px; margin:4px 0 0;">Dynamically correlated event timelines grouping anomalies and process deviations.</p>
        </div>
        {#if incidentsList.length > 0}
            <span class="badge badge-fault" style="font-size:11px; padding:4px 10px;">{incidentsList.length} OPEN</span>
        {/if}
    </div>

    {#if incidentsList.length === 0}
        <div style="text-align:center;padding:64px 32px;border:1px dashed rgba(255,255,255,0.07);border-radius:12px;color:rgba(255,255,255,0.2);">
            <svg style="width:40px;height:40px;margin:0 auto 12px;opacity:0.3;" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z"/></svg>
            <p style="font-size:14px;margin:0;">No active incidents</p>
            <p style="font-size:12px;margin:6px 0 0;color:rgba(255,255,255,0.15);">Waiting for correlated events from Edge Core...</p>
        </div>
    {/if}

    <div style="display:flex; flex-direction:column; gap:16px;">
        {#each incidentsList as incident}
            {@const isAcked = incident.acknowledged}
            <div class="panel" style="padding:24px; border-color:{isAcked ? 'rgba(255,255,255,0.06)' : 'rgba(239,68,68,0.2)'}; transition:border-color 0.3s;">
                <!-- Incident Header -->
                <div style="display:flex; align-items:flex-start; justify-content:space-between; margin-bottom:20px;">
                    <div>
                        <div style="display:flex; align-items:center; gap:8px; margin-bottom:6px;">
                            {#if !isAcked}
                                <span class="badge badge-error">OPEN INCIDENT</span>
                            {:else}
                                <span class="badge badge-ready">ACKNOWLEDGED</span>
                            {/if}
                            <h2 style="font-size:16px; font-weight:700; color:white; margin:0;">{incident.title}</h2>
                        </div>
                        <p style="font-size:12px; color:rgba(255,255,255,0.35); margin:0;">
                            Asset: <span style="font-family:var(--font-mono); color:rgba(255,255,255,0.5);">{incident.asset_id}</span>
                            &nbsp;·&nbsp; ID: <span style="font-family:var(--font-mono); color:rgba(255,255,255,0.4);">{incident.id}</span>
                        </p>
                    </div>
                    <div style="display:flex;gap:8px;">
                        {#if !isAcked}
                            <button
                                class="btn btn-ghost"
                                onclick={() => acknowledgeIncident(incident.id)}
                                id="ack-incident-{incident.id}"
                            >
                                <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
                                Acknowledge
                            </button>
                        {/if}
                        <button
                            class="btn btn-outline-neon"
                            onclick={() => investigate(incident)}
                            id="investigate-incident-{incident.id}"
                        >
                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
                            Investigate
                        </button>
                    </div>
                </div>

                <!-- Leading Hypothesis -->
                <div style="margin-bottom:20px;">
                    <div class="section-label" style="margin-bottom:8px;">Leading Hypothesis</div>
                    <div style="padding:12px 16px; border-radius:8px; background:rgba(245,158,11,0.06); border:1px solid rgba(245,158,11,0.2);">
                        <div style="display:flex; align-items:flex-start; gap:8px;">
                            <svg style="flex-shrink:0;margin-top:2px;" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#f59e0b" stroke-width="2"><path d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z"/></svg>
                            <span style="font-size:13px; color:#fbbf24; font-style:italic;">{incident.hypothesis}</span>
                        </div>
                    </div>
                </div>

                <!-- Correlated Event Timeline -->
                <div>
                    <div class="section-label" style="margin-bottom:12px;">Correlated Event Timeline</div>
                    <div class="timeline-line" style="margin-left:4px;">
                        {#each incident.timeline || [] as event}
                            <div style="position:relative; margin-bottom:18px;">
                                <div class="timeline-dot {event.type === 'ALARM' ? 'timeline-dot-red' : ''}"></div>
                                <div style="font-size:10px; color:rgba(255,255,255,0.3); margin-bottom:3px; font-family:var(--font-mono);">
                                    {new Date(event.timestamp / 1000000).toLocaleTimeString()}
                                </div>
                                <div style="font-size:13px; font-weight:500; color:{event.type === 'ALARM' ? '#f87171' : 'rgba(255,255,255,0.8)'};">
                                    {event.description}
                                </div>
                                {#if event.type === 'ALARM'}
                                    <span class="badge badge-fault" style="margin-top:4px;">ALARM</span>
                                {/if}
                            </div>
                        {/each}
                        {#if !incident.timeline || incident.timeline.length === 0}
                            <div style="color:rgba(255,255,255,0.2);font-size:12px;">No timeline events yet.</div>
                        {/if}
                    </div>
                </div>
            </div>
        {/each}
    </div>
</div>
