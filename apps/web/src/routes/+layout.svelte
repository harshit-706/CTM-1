<script lang="ts">
    import '../app.css';
    import { onMount } from 'svelte';
    import { page } from '$app/stores';
    import { connectionStatus, activeAlarms, connectWebSocket } from '$lib/store';
    import AiAssistant from '$lib/components/AiAssistant.svelte';

    let { children } = $props();

    onMount(() => {
        connectWebSocket();
    });

    // Nav items
    const navItems = [
        { href: '/',           icon: 'M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6', label: 'Command Center' },
        { href: '/assets',     icon: 'M9 3H5a2 2 0 00-2 2v4m6-6h10a2 2 0 012 2v4M9 3v18m0 0h10a2 2 0 002-2V9M9 21H5a2 2 0 01-2-2V9m0 0h18', label: 'Assets' },
        { href: '/incidents',  icon: 'M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z', label: 'Incidents' },
        { href: '/energy',     icon: 'M13 10V3L4 14h7v7l9-11h-7z', label: 'Energy' },
        { href: '/topology',   icon: 'M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zM14 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zM14 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z', label: 'Topology' },
        { href: '/lifecycle',  icon: 'M9 12l2 2 4-4M7.835 4.697a3.42 3.42 0 001.946-.806 3.42 3.42 0 014.438 0 3.42 3.42 0 001.946.806 3.42 3.42 0 013.138 3.138 3.42 3.42 0 00.806 1.946 3.42 3.42 0 010 4.438 3.42 3.42 0 00-.806 1.946 3.42 3.42 0 01-3.138 3.138 3.42 3.42 0 00-1.946.806 3.42 3.42 0 01-4.438 0 3.42 3.42 0 00-1.946-.806 3.42 3.42 0 01-3.138-3.138 3.42 3.42 0 00-.806-1.946 3.42 3.42 0 010-4.438 3.42 3.42 0 00.806-1.946 3.42 3.42 0 013.138-3.138z', label: 'Lifecycle' },
        { href: '/simulator',  icon: 'M9.75 17L9 20l-1 1h8l-1-1-.75-3M3 13h18M5 17h14a2 2 0 002-2V5a2 2 0 00-2-2H5a2 2 0 00-2 2v10a2 2 0 002 2z', label: 'Simulator' },
    ];

    let currentPath = $derived($page.url.pathname);
    let alarmCount = $derived(Object.keys($activeAlarms).length);

    function isActive(href: string) {
        if (href === '/') return currentPath === '/';
        return currentPath.startsWith(href);
    }
</script>

<div class="flex h-screen w-screen overflow-hidden" style="background: var(--panel-bg); font-family: var(--font-sans);">
    <!-- ── Sidebar ── -->
    <aside style="
        width: 56px;
        min-width: 56px;
        background: rgba(6,6,10,0.97);
        border-right: 1px solid rgba(255,255,255,0.06);
        display: flex;
        flex-direction: column;
        align-items: center;
        padding: 12px 0;
        gap: 4px;
        z-index: 50;
    ">
        <!-- Logo -->
        <div style="
            width: 34px; height: 34px;
            background: var(--neon);
            border-radius: 8px;
            display: flex; align-items: center; justify-content: center;
            margin-bottom: 16px;
            box-shadow: 0 0 16px var(--neon-glow);
            flex-shrink: 0;
        ">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#060a0a" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                <polygon points="12 2 22 8.5 22 15.5 12 22 2 15.5 2 8.5 12 2"/>
                <line x1="12" y1="22" x2="12" y2="15.5"/>
                <polyline points="22 8.5 12 15.5 2 8.5"/>
            </svg>
        </div>

        <!-- Nav Items -->
        {#each navItems as item}
            {@const active = isActive(item.href)}
            <a
                href={item.href}
                class="nav-item {active ? 'active' : ''}"
                title={item.label}
                aria-label={item.label}
            >
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round">
                    <path d={item.icon}/>
                </svg>
            </a>
        {/each}

        <!-- Spacer -->
        <div style="flex: 1;"></div>

        <!-- Connection Status -->
        <div style="padding: 12px 0; display: flex; flex-direction: column; align-items: center; gap: 4px;">
            <div
                style="
                    width: 8px; height: 8px; border-radius: 50%;
                    background: {$connectionStatus === 'CONNECTED' ? 'var(--neon)' : '#f59e0b'};
                    box-shadow: 0 0 8px {$connectionStatus === 'CONNECTED' ? 'var(--neon-glow)' : 'rgba(245,158,11,0.5)'};
                    transition: all 0.3s;
                "
                title={$connectionStatus}
            ></div>
            <span style="font-size: 9px; color: rgba(255,255,255,0.25); letter-spacing: 0.05em; writing-mode: vertical-lr; transform: rotate(180deg);">
                {$connectionStatus}
            </span>
        </div>
    </aside>

    <!-- ── Main Content ── -->
    <main style="flex: 1; overflow: hidden; display: flex; flex-direction: column;">
        <div class="persistent-chrome">
            <span>PLANT 01 / CTM-1</span>
            <span class="chrome-connection"><i class:connected={$connectionStatus === 'CONNECTED'}></i>{$connectionStatus}</span>
            <a href="/incidents" class:has-alarms={alarmCount > 0}>{alarmCount} active alarm{alarmCount === 1 ? '' : 's'}</a>
            <span class="chrome-user">OPERATOR / LOCAL</span>
        </div>
        {@render children()}
    </main>
    <AiAssistant />
</div>

