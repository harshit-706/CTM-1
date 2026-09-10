export interface Asset {
    id: string;
    name: string;
    type: string;
    vendor: string;
    model: string;
    location: string;
    parentId?: string;
    health: number; // 0-100
    status: 'ONLINE' | 'OFFLINE' | 'DEGRADED' | 'FAULT';
    protocol: string;
    endpoint: string;
    firmware: string;
    configurationVersion: string;
    tags: Tag[];
    sensors: Sensor[];
}

export interface Tag {
    id: string;
    name: string;
    unit: string;
    type: 'number' | 'boolean' | 'string';
}

export interface Sensor {
    id: string;
    type: string; // e.g. "temperature", "vibration"
}

export interface TelemetryEvent {
    timestamp_ns: number;
    asset_id: string;
    tag_id: string;
    value: number; // Simplified for prototype
    quality: 'GOOD' | 'BAD' | 'UNCERTAIN';
    source: string; // e.g. "SIMULATED"
}

export interface Alarm {
    id: string;
    asset_id: string;
    priority: 'LOW' | 'MEDIUM' | 'HIGH' | 'CRITICAL';
    message: string;
    state: 'ACTIVE' | 'ACKNOWLEDGED' | 'CLEARED';
    timestamp: number;
}

export interface Incident {
    id: string;
    asset_id: string;
    title: string;
    timeline: IncidentEvent[];
    hypothesis: string;
    status: 'OPEN' | 'CLOSED';
}

export interface IncidentEvent {
    timestamp: number;
    description: string;
    type: 'TELEMETRY_DEVIATION' | 'ALARM' | 'STATE_CHANGE' | 'PRODUCTION_IMPACT';
}
