import axios from 'axios';

const API_BASE_URL = '/api';

export interface SystemMetric {
    timestamp: number;
    cpu: number;
    memory: number;
    net_rx: number;
    net_tx: number;
    disk_read: number;
    disk_write: number;
}

export type TimeSpan = '30d' | '7d' | '1d' | '1h' | '1m' | 'realtime';

export const fetchHistory = async (span: TimeSpan): Promise<SystemMetric[]> => {
    try {
        const response = await axios.get<SystemMetric[]>(`${API_BASE_URL}/history`, {
            params: { span }
        });
        return response.data;
    } catch (error) {
        console.error("Failed to fetch history:", error);
        return [];
    }
};
