import React, { useEffect, useState, useRef } from 'react';
import { fetchHistory, SystemMetric, TimeSpan } from '../api/stats';
import MetricChart from './MetricChart';
import { Activity, Server, Network, HardDrive, Clock } from 'lucide-react';
import clsx from 'clsx';

const Dashboard: React.FC = () => {
    const [metrics, setMetrics] = useState<SystemMetric[]>([]);
    const [span, setSpan] = useState<TimeSpan>('realtime');
    const [loading, setLoading] = useState(true);
    const intervalRef = useRef<number | null>(null);

    const loadData = async () => {
        try {
            const data = await fetchHistory(span);
            setMetrics(data);
        } catch (e) {
            console.error(e);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        loadData();
        
        // Setup polling
        if (intervalRef.current) clearInterval(intervalRef.current);
        
        let intervalTime = 5000;
        if (span === 'realtime') intervalTime = 1000;
        else if (span === '1h') intervalTime = 10000;
        else intervalTime = 60000;

        intervalRef.current = window.setInterval(loadData, intervalTime);

        return () => {
            if (intervalRef.current) clearInterval(intervalRef.current);
        };
    }, [span]);

    const formatBytes = (bytes: number) => {
        if (bytes === 0) return '0 B/s';
        const k = 1024;
        const sizes = ['B/s', 'KB/s', 'MB/s', 'GB/s'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    };

    const spans: { label: string; value: TimeSpan }[] = [
        { label: '实时', value: 'realtime' },
        { label: '1小时', value: '1h' },
        { label: '1天', value: '1d' },
        { label: '7天', value: '7d' },
        { label: '30天', value: '30d' },
    ];

    return (
        <div className="min-h-screen bg-black text-zinc-100 p-8 font-sans">
            <header className="mb-8 flex flex-col md:flex-row md:items-center justify-between gap-4">
                <div>
                    <h1 className="text-3xl font-bold tracking-tight flex items-center gap-2">
                        <Server className="w-8 h-8 text-indigo-500" />
                        服务器监控
                    </h1>
                    <p className="text-zinc-500 mt-1">实时系统性能指标</p>
                </div>
                
                <div className="flex bg-zinc-900 p-1 rounded-lg border border-zinc-800">
                    {spans.map((s) => (
                        <button
                            key={s.value}
                            onClick={() => setSpan(s.value)}
                            className={clsx(
                                "px-4 py-2 text-sm font-medium rounded-md transition-all",
                                span === s.value 
                                    ? "bg-indigo-600 text-white shadow-sm" 
                                    : "text-zinc-400 hover:text-white hover:bg-zinc-800"
                            )}
                        >
                            {s.label}
                        </button>
                    ))}
                </div>
            </header>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                <MetricChart 
                    title="CPU 使用率" 
                    data={metrics} 
                    dataKey="cpu" 
                    color="#818cf8" 
                    unit="%" 
                />
                <MetricChart 
                    title="内存使用率" 
                    data={metrics} 
                    dataKey="memory" 
                    color="#34d399" 
                    unit="%" 
                />
                <MetricChart 
                    title="网络流量" 
                    data={metrics} 
                    dataKey="net_rx" 
                    dataKey2="net_tx"
                    color="#f472b6" 
                    color2="#60a5fa"
                    unit="B/s" 
                    formatValue={formatBytes}
                />
                <MetricChart 
                    title="磁盘 I/O" 
                    data={metrics} 
                    dataKey="disk_read" 
                    dataKey2="disk_write"
                    color="#fbbf24" 
                    color2="#f87171"
                    unit="Sectors/s" 
                    formatValue={(v) => `${v.toFixed(0)} IOPS`} // Approx as sectors for now
                />
            </div>

            <footer className="mt-12 text-center text-zinc-600 text-sm">
                <p>运行于 Linux • 基于 C & React 构建</p>
            </footer>
        </div>
    );
};

export default Dashboard;
