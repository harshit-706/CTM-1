<script lang="ts">
    import { onMount, tick } from 'svelte';
    import { browser } from '$app/environment';
    import { SvelteFlow, Controls, Background, BackgroundVariant } from '@xyflow/svelte';
    import '@xyflow/svelte/dist/style.css';
    import IndustrialNode from '$lib/components/IndustrialNode.svelte';
    import IndustrialIcons from '$lib/components/IndustrialIcons.svelte';
    import { Save, AlertTriangle, X, Sparkles } from 'lucide-svelte';
    import { clearStores, aiGeneratedTopology } from '$lib/store';

    const nodeTypes = { industrial: IndustrialNode };

    // Watch for AI-generated topology from Astra AI assistant
    $effect(() => {
        if ($aiGeneratedTopology) {
            const topData = $aiGeneratedTopology;
            if (topData.nodes && Array.isArray(topData.nodes)) {
                nodes = topData.nodes.map((n: any, i: number) => ({
                    id: n.id || `node-${i+1}`,
                    type: 'industrial',
                    position: { x: n.x ?? (80 + i * 180), y: n.y ?? 180 },
                    data: {
                        label: n.label || n.type,
                        type: n.type,
                        manufacturer: n.manufacturer,
                        model: n.model
                    }
                }));
                idCounter = nodes.length + 1;
            }
            if (topData.edges && Array.isArray(topData.edges)) {
                edges = topData.edges.map((e: any) => ({
                    id: `e${e.source}-${e.target}`,
                    source: e.source,
                    target: e.target,
                    animated: true,
                    style: 'stroke: var(--neon); stroke-width: 2px;'
                }));
            }
            publishedStatus = `✓ Astra AI generated ${nodes.length} assets & ${edges.length} connections`;
            publishedOk = true;
            setTimeout(() => { publishedStatus = ''; publishedOk = false; }, 6000);
            aiGeneratedTopology.set(null);
        }
    });

    const palette = [
        { category: 'Control', color: '#3b82f6', items: [
            { type: 'plc', label: 'PLC Controller' },
            { type: 'pac', label: 'PAC Controller' },
            { type: 'remote_io', label: 'Remote I/O' },
            { type: 'edge_gateway', label: 'Edge Gateway' }
        ]},
        { category: 'Electrical', color: '#f59e0b', items: [
            { type: 'breaker', label: 'Circuit Breaker' },
            { type: 'vfd', label: 'VFD Drive' },
            { type: 'soft_starter', label: 'Soft Starter' },
            { type: 'energy_meter', label: 'Energy Meter' }
        ]},
        { category: 'Mechanical', color: '#00e676', items: [
            { type: 'motor', label: 'AC Motor' },
            { type: 'pump', label: 'Centrifugal Pump' },
            { type: 'compressor', label: 'Compressor' },
            { type: 'fan', label: 'Industrial Fan' }
        ]},
        { category: 'Sensors', color: '#a855f7', items: [
            { type: 'temp_sensor', label: 'Temperature' },
            { type: 'pressure_sensor', label: 'Pressure' },
            { type: 'flow_sensor', label: 'Flow' },
            { type: 'vibration_sensor', label: 'Vibration' }
        ]},
        { category: 'HMI / SCADA', color: '#6b7280', items: [
            { type: 'hmi', label: 'Operator HMI' },
            { type: 'scada', label: 'SCADA System' }
        ]}
    ];

    // Vendor Database Mapping
    const vendorDB: Record<string, { mfr: string, models: string[] }[]> = {
        'plc': [
            { mfr: 'Siemens', models: ['S7-1500', 'S7-1200', 'LOGO! 8'] },
            { mfr: 'Allen-Bradley', models: ['ControlLogix 5580', 'CompactLogix 5380', 'Micro800'] },
            { mfr: 'Schneider Electric', models: ['Modicon M580', 'Modicon M340'] }
        ],
        'pac': [
            { mfr: 'Beckhoff', models: ['CX2040', 'CX5130'] },
            { mfr: 'Omron', models: ['NX102', 'NJ501'] }
        ],
        'vfd': [
            { mfr: 'Danfoss', models: ['VLT AutomationDrive FC 302', 'VLT AQUA Drive FC 202'] },
            { mfr: 'ABB', models: ['ACS880', 'ACS580'] },
            { mfr: 'Siemens', models: ['SINAMICS G120', 'SINAMICS S120'] }
        ],
        'motor': [
            { mfr: 'WEG', models: ['W22 Premium Efficiency', 'W21'] },
            { mfr: 'Siemens', models: ['SIMOTICS SD', 'SIMOTICS GP'] },
            { mfr: 'ABB', models: ['M3BP', 'M2BAX'] }
        ],
        'pump': [
            { mfr: 'Grundfos', models: ['CR/CRI/CRN', 'MAGNA3'] },
            { mfr: 'KSB', models: ['Etanorm', 'Amarex'] }
        ],
        'temp_sensor': [
            { mfr: 'Endress+Hauser', models: ['iTHERM TM411', 'Omnigrad M'] },
            { mfr: 'Rosemount', models: ['3144P', '644'] }
        ],
        'flow_sensor': [
            { mfr: 'Endress+Hauser', models: ['Proline Promass', 'Proline Promag'] },
            { mfr: 'Krohne', models: ['OPTIFLUX 4300', 'WATERFLUX 3070'] }
        ]
    };

    let nodes = $state<{id: string, type: string, position: any, data: any}[]>([]);
    let edges = $state<any[]>([]);
    let isPublishing = $state(false);
    let publishedStatus = $state('');
    let publishedOk = $state(false);
    let idCounter = $state(1);
    let validationErrors = $state<string[]>([]);

    // Drop Modal State
    let pendingDrop = $state<{ type: string, label: string, position: any } | null>(null);
    let selectedMfr = $state('');
    let selectedModel = $state('');
    let availableModels = $derived(
        pendingDrop ? (vendorDB[pendingDrop.type]?.find(v => v.mfr === selectedMfr)?.models || []) : []
    );

    // Watch selectedMfr to reset selectedModel
    $effect(() => {
        if (selectedMfr) selectedModel = '';
    });


    const allowedConnections: Record<string, string[]> = {
        'plc': ['vfd', 'soft_starter', 'remote_io', 'scada', 'edge_gateway'],
        'pac': ['vfd', 'soft_starter', 'remote_io', 'scada', 'edge_gateway'],
        'remote_io': ['vfd', 'soft_starter', 'temp_sensor', 'pressure_sensor', 'flow_sensor', 'vibration_sensor'],
        'breaker': ['vfd', 'soft_starter', 'energy_meter', 'motor'],
        'vfd': ['motor'],
        'soft_starter': ['motor'],
        'motor': ['pump', 'compressor', 'fan', 'vibration_sensor', 'temp_sensor'],
        'pump': ['pressure_sensor', 'flow_sensor', 'vibration_sensor'],
        'compressor': ['pressure_sensor', 'temp_sensor', 'vibration_sensor'],
        'fan': ['vibration_sensor'],
        'temp_sensor': ['plc', 'pac', 'remote_io', 'edge_gateway'],
        'pressure_sensor': ['plc', 'pac', 'remote_io', 'edge_gateway'],
        'flow_sensor': ['plc', 'pac', 'remote_io', 'edge_gateway'],
        'vibration_sensor': ['plc', 'pac', 'remote_io', 'edge_gateway'],
        'energy_meter': ['plc', 'pac', 'edge_gateway'],
        'edge_gateway': ['scada', 'hmi']
    };

    function onDragStart(event: DragEvent, type: string, label: string) {
        if (event.dataTransfer) {
            event.dataTransfer.setData('application/svelteflow/type', type);
            event.dataTransfer.setData('application/svelteflow/label', label);
            event.dataTransfer.effectAllowed = 'move';
        }
    }
    function onDragOver(event: DragEvent) {
        event.preventDefault();
        if (event.dataTransfer) event.dataTransfer.dropEffect = 'move';
    }
    
    function onDrop(event: DragEvent) {
        event.preventDefault();
        const type = event.dataTransfer?.getData('application/svelteflow/type');
        const label = event.dataTransfer?.getData('application/svelteflow/label');
        if (!type || !label) return;
        
        const bounds = (event.currentTarget as Element).getBoundingClientRect();
        const position = { x: event.clientX - bounds.left - 80, y: event.clientY - bounds.top - 30 };
        
        pendingDrop = { type, label, position };
        selectedMfr = '';
        selectedModel = '';
    }

    function confirmDrop() {
        if (!pendingDrop) return;
        nodes = [...nodes, {
            id: `${pendingDrop.type}-${idCounter++}`,
            type: 'industrial',
            position: pendingDrop.position,
            data: { 
                label: `${pendingDrop.label} ${idCounter - 1}`, 
                type: pendingDrop.type,
                manufacturer: selectedMfr || undefined,
                model: selectedModel || undefined
            }
        }];
        pendingDrop = null;
    }

    function cancelDrop() {
        pendingDrop = null;
    }

    const onConnect = (connection: any) => {
        const src = nodes.find(n => n.id === connection.source);
        const tgt = nodes.find(n => n.id === connection.target);
        if (!src || !tgt) return;
        const allowed = allowedConnections[src.data.type] || [];
        if (allowed.includes(tgt.data.type)) {
            edges = [...edges, { ...connection, id: `e${connection.source}-${connection.target}`, animated: true, style: 'stroke: var(--neon); stroke-width: 2px;' }];
            validationErrors = [];
        } else {
            validationErrors = [`Cannot connect ${src.data.type} → ${tgt.data.type}. Allowed: ${allowed.join(', ') || 'none'}`];
            setTimeout(() => validationErrors = [], 4000);
        }
    };

    function clearCanvas() {
        if (nodes.length === 0) return;
        if (confirm('Clear all nodes and edges?')) {
            nodes = []; edges = []; idCounter = 1;
        }
    }

    async function publishTopology() {
        isPublishing = true;
        publishedOk = false;
        publishedStatus = 'Validating...';
        const payload = {
            nodes: nodes.map(n => ({ id: n.id, type: n.data.type, label: n.data.label })),
            edges
        };
        try {
            const res = await fetch('http://localhost:8080/api/v1/topology', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });
            if (res.ok) {
                clearStores();
                publishedStatus = `✓ ${nodes.length} assets published`;
                publishedOk = true;
            } else {
                publishedStatus = '✗ Edge Core error';
            }
        } catch {
            publishedStatus = '✗ Edge Core unreachable';
        }
        isPublishing = false;
        setTimeout(() => { publishedStatus = ''; publishedOk = false; }, 5000);
    }
</script>

<svelte:head>
    <title>Topology Designer — CTM-1</title>
</svelte:head>

<div style="height:100%; display:flex; flex-direction:column; overflow:hidden; position: relative;">
    <!-- Header -->
    <div style="padding:16px 24px; border-bottom:1px solid rgba(255,255,255,0.06); background:rgba(6,6,10,0.8); backdrop-filter:blur(12px); display:flex; align-items:center; justify-content:space-between; flex-shrink:0; z-index:10;">
        <div>
            <h1 style="font-size:1.1rem; font-weight:700; color:white; margin:0; letter-spacing:-0.01em;">Topology Designer</h1>
            <p style="font-size:11px; color:rgba(255,255,255,0.35); margin:2px 0 0;">Drag components from the palette · connect with validated edges · publish</p>
        </div>
        <div style="display:flex; align-items:center; gap:12px;">
            {#if nodes.length > 0}
                <span style="font-size:12px; color:rgba(255,255,255,0.35);">{nodes.length} node{nodes.length !== 1 ? 's' : ''} · {edges.length} edge{edges.length !== 1 ? 's' : ''}</span>
            {/if}
            {#if publishedStatus}
                <span style="font-size:12px; font-weight:600; color:{publishedOk ? 'var(--neon)' : '#f87171'};">{publishedStatus}</span>
            {/if}
            <button
                class="btn btn-ghost"
                onclick={clearCanvas}
                disabled={nodes.length === 0}
                id="clear-topology-btn"
            >
                <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 01-2 2H8a2 2 0 01-2-2L5 6"/><path d="M10 11v6m4-6v6"/></svg>
                Clear
            </button>
            <button
                onclick={publishTopology}
                disabled={isPublishing || nodes.length === 0}
                class="btn {nodes.length > 0 ? 'btn-neon' : 'btn-ghost'}"
                id="publish-topology-btn"
            >
                <Save size={14} />
                {isPublishing ? 'Publishing...' : 'Publish Configuration'}
            </button>
        </div>
    </div>

    <!-- Validation error banner -->
    {#if validationErrors.length > 0}
        <div style="padding:8px 24px; background:rgba(239,68,68,0.08); border-bottom:1px solid rgba(239,68,68,0.2); display:flex;align-items:center;gap:8px;">
            <AlertTriangle size={14} color="#f87171" />
            <span style="font-size:12px; color:#f87171;">{validationErrors[0]}</span>
        </div>
    {/if}

    <div style="flex:1; display:flex; overflow:hidden;">
        <!-- Palette Sidebar -->
        <div style="width:240px; background:rgba(6,6,10,0.95); border-right:1px solid rgba(255,255,255,0.06); display:flex; flex-direction:column; overflow-y:auto; flex-shrink:0; padding:16px;">
            <div class="section-label" style="margin-bottom:12px;">Industrial Components</div>
            <div style="display:flex; flex-direction:column; gap:16px;">
                {#each palette as cat}
                    <div>
                        <div style="font-size:10px; font-weight:700; letter-spacing:0.08em; text-transform:uppercase; color:{cat.color}; margin-bottom:6px; opacity:0.8;">{cat.category}</div>
                        <div style="display:flex; flex-direction:column; gap:3px;">
                            {#each cat.items as item}
                                <div
                                    role="button"
                                    tabindex="0"
                                    draggable="true"
                                    ondragstart={(e) => onDragStart(e, item.type, item.label)}
                                    style="
                                        display:flex;align-items:center;gap:8px;
                                        padding:7px 10px; border-radius:6px;
                                        border:1px solid rgba(255,255,255,0.06);
                                        background:rgba(255,255,255,0.03);
                                        cursor:grab; transition:all 0.15s; font-size:12px; color:rgba(255,255,255,0.7);
                                    "
                                    onmouseover={(e) => { (e.currentTarget as HTMLElement).style.borderColor = cat.color + '55'; (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.06)'; (e.currentTarget as HTMLElement).style.color = 'white'; }}
                                    onmouseout={(e) => { (e.currentTarget as HTMLElement).style.borderColor = 'rgba(255,255,255,0.06)'; (e.currentTarget as HTMLElement).style.background = 'rgba(255,255,255,0.03)'; (e.currentTarget as HTMLElement).style.color = 'rgba(255,255,255,0.7)'; }}
                                >
                                    <div style="width:16px;height:16px;display:flex;align-items:center;justify-content:center;color:{cat.color};flex-shrink:0;">
                                        <IndustrialIcons type={item.type} className="w-4 h-4" />
                                    </div>
                                    {item.label}
                                </div>
                            {/each}
                        </div>
                    </div>
                {/each}
            </div>

            <!-- Connection Rules -->
            <div style="margin-top:auto;padding-top:16px;">
                <div style="border-radius:8px;border:1px solid rgba(0,230,118,0.15);background:rgba(0,230,118,0.04);padding:12px;">
                    <div style="font-size:10px;font-weight:700;letter-spacing:0.08em;text-transform:uppercase;color:var(--neon);margin-bottom:8px;opacity:0.8;">Connection Rules</div>
                    <ul style="list-style:none;padding:0;margin:0;display:flex;flex-direction:column;gap:3px;">
                        <li style="font-size:10px;color:rgba(255,255,255,0.4);">PLC / PAC → VFD / Motor</li>
                        <li style="font-size:10px;color:rgba(255,255,255,0.4);">VFD → Motor</li>
                        <li style="font-size:10px;color:rgba(255,255,255,0.4);">Motor → Pump / Fan</li>
                        <li style="font-size:10px;color:rgba(255,255,255,0.4);">Pump → Sensors</li>
                        <li style="font-size:10px;color:rgba(255,255,255,0.4);">Sensor → Edge Gateway</li>
                    </ul>
                </div>
            </div>
        </div>

        <!-- Canvas -->
        <div style="flex:1; background:#07070c; position:relative;" ondragover={onDragOver} ondrop={onDrop}>
            {#if browser}
                <SvelteFlow {nodes} {edges} {nodeTypes} onconnect={onConnect} fitView>
                    <Controls />
                    <Background variant={BackgroundVariant.Dots} gap={28} size={1} />
                </SvelteFlow>
            {/if}
            {#if nodes.length === 0}
                <div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;pointer-events:none;">
                    <div style="text-align:center;padding:32px;border:1px dashed rgba(255,255,255,0.08);border-radius:12px;background:rgba(0,0,0,0.3);">
                        <div style="font-size:32px;margin-bottom:8px;opacity:0.2;">⬡</div>
                        <p style="color:rgba(255,255,255,0.3);font-size:13px;margin:0;">Drag components from the palette to begin</p>
                        <p style="color:rgba(255,255,255,0.18);font-size:11px;margin:6px 0 0;">Connect nodes · Validate · Publish to Edge Core</p>
                    </div>
                </div>
            {/if}
        </div>
    </div>

    <!-- Configuration Modal for Drops -->
    {#if pendingDrop}
        <div style="position:absolute;inset:0;background:rgba(0,0,0,0.7);backdrop-filter:blur(4px);z-index:100;display:flex;align-items:center;justify-content:center;">
            <div class="panel" style="width:100%;max-width:420px;padding:0;overflow:hidden;box-shadow:0 0 40px rgba(0,0,0,0.8), 0 0 0 1px rgba(0,230,118,0.3);">
                <!-- Modal Header -->
                <div style="padding:16px 20px;border-bottom:1px solid rgba(255,255,255,0.06);display:flex;align-items:center;justify-content:space-between;background:rgba(255,255,255,0.02);">
                    <div style="display:flex;align-items:center;gap:10px;">
                        <div style="width:32px;height:32px;border-radius:8px;background:rgba(0,230,118,0.1);color:var(--neon);display:flex;align-items:center;justify-content:center;">
                            <IndustrialIcons type={pendingDrop.type} className="w-5 h-5" />
                        </div>
                        <div>
                            <h2 style="font-size:14px;font-weight:600;color:white;margin:0;">Configure {pendingDrop.label}</h2>
                            <p style="font-size:11px;color:rgba(255,255,255,0.4);margin:0;">Define specifications for this node</p>
                        </div>
                    </div>
                    <button class="btn btn-ghost" style="padding:6px;" onclick={cancelDrop}>
                        <X size={16} />
                    </button>
                </div>

                <!-- Modal Body -->
                <div style="padding:20px;display:flex;flex-direction:column;gap:16px;">
                    {#if vendorDB[pendingDrop.type]}
                        <div>
                            <label style="display:block;font-size:11px;color:rgba(255,255,255,0.5);margin-bottom:6px;text-transform:uppercase;letter-spacing:0.05em;">Manufacturer</label>
                            <select bind:value={selectedMfr} class="industrial-input industrial-select" style="width:100%;">
                                <option value="" disabled selected>Select Manufacturer...</option>
                                {#each vendorDB[pendingDrop.type] as v}
                                    <option value={v.mfr}>{v.mfr}</option>
                                {/each}
                                <option value="Generic">Generic / Other</option>
                            </select>
                        </div>

                        {#if selectedMfr && selectedMfr !== 'Generic'}
                            <div>
                                <label style="display:block;font-size:11px;color:rgba(255,255,255,0.5);margin-bottom:6px;text-transform:uppercase;letter-spacing:0.05em;">Model Designation</label>
                                <select bind:value={selectedModel} class="industrial-input industrial-select" style="width:100%;">
                                    <option value="" disabled selected>Select Model...</option>
                                    {#each availableModels as mod}
                                        <option value={mod}>{mod}</option>
                                    {/each}
                                </select>
                            </div>
                        {/if}
                    {:else}
                        <div style="font-size:13px;color:rgba(255,255,255,0.5);text-align:center;padding:12px 0;">
                            Generic component. No vendor-specific models available.
                        </div>
                    {/if}

                    <!-- Generic Spec Tag Input -->
                    <div>
                        <label style="display:block;font-size:11px;color:rgba(255,255,255,0.5);margin-bottom:6px;text-transform:uppercase;letter-spacing:0.05em;">Additional Specifications</label>
                        <input type="text" class="industrial-input" style="width:100%;" placeholder="e.g., 480V, 50HP, IP67..." />
                    </div>
                </div>

                <!-- Modal Footer -->
                <div style="padding:16px 20px;border-top:1px solid rgba(255,255,255,0.06);display:flex;align-items:center;justify-content:flex-end;gap:10px;background:rgba(0,0,0,0.2);">
                    <button class="btn btn-ghost" onclick={cancelDrop}>Cancel</button>
                    <button class="btn btn-neon" onclick={confirmDrop}>Place Component</button>
                </div>
            </div>
        </div>
    {/if}
</div>
