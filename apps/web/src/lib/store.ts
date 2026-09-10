import { writable, get } from 'svelte/store';

export const liveTelemetry = writable<Record<string, any>>({});
export const connectionStatus = writable<'CONNECTING' | 'CONNECTED' | 'DISCONNECTED'>('CONNECTING');
export const activeAlarms = writable<Record<string, any>>({});
export const incidents = writable<Record<string, any>>({});
export const energyStats = writable<Record<string, any>>({});
export const aiGeneratedTopology = writable<{ nodes: any[], edges: any[] } | null>(null);

let ws: WebSocket;

export function clearStores() {
    liveTelemetry.set({});
    activeAlarms.set({});
    incidents.set({});
    energyStats.set({});
}

export function connectWebSocket() {
    // Explicitly hit the ONE authoritative runtime endpoint at 8080
    ws = new WebSocket('ws://localhost:8080/live');

    ws.onopen = () => {
        connectionStatus.set('CONNECTED');
    };

    ws.onmessage = (event) => {
        try {
            const msg = JSON.parse(event.data);
            if (msg.type === 'telemetry.update') {
                liveTelemetry.update(store => {
                    const assetId = msg.payload.asset_id;
                    if (!store[assetId]) {
                        store[assetId] = {};
                    }
                    for (const [key, value] of Object.entries(msg.payload.tags)) {
                        store[assetId][key] = value;
                    }
                    store[assetId].lastUpdate = msg.timestamp;
                    return store;
                });
            } else if (msg.type === 'alarm.created' || msg.type === 'alarm.updated') {
                activeAlarms.update(store => {
                    store[msg.payload.id] = msg.payload;
                    return store;
                });
            } else if (msg.type === 'alarm.cleared') {
                activeAlarms.update(store => {
                    delete store[msg.payload.id];
                    return store;
                });
            } else if (msg.type === 'incident.created') {
                incidents.update(store => {
                    store[msg.payload.id] = msg.payload;
                    return store;
                });
            } else if (msg.type === 'energy.update') {
                energyStats.update(store => {
                    store[msg.payload.asset_id] = msg.payload;
                    return store;
                });
            }
        } catch (e) {
            console.error("Failed to parse websocket message", e);
        }
    };

    ws.onclose = () => {
        connectionStatus.set('DISCONNECTED');
        setTimeout(connectWebSocket, 3000);
    };
    
    ws.onerror = () => {
        connectionStatus.set('DISCONNECTED');
    };
}
