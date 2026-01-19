import React from 'react';
import {
    LineChart,
    Line,
    XAxis,
    YAxis,
    CartesianGrid,
    Tooltip,
    ResponsiveContainer,
    AreaChart,
    Area
} from 'recharts';
import { SystemMetric } from '../api/stats';
import clsx from 'clsx';

interface MetricChartProps {
    title: string;
    data: SystemMetric[];
    dataKey: keyof SystemMetric;
    dataKey2?: keyof SystemMetric; // For RX/TX or Read/Write
    color: string;
    color2?: string;
    unit: string;
    formatValue?: (val: number) => string;
}

const MetricChart: React.FC<MetricChartProps> = ({ 
    title, data, dataKey, dataKey2, color, color2, unit, formatValue 
}) => {
    
    const formatXAxis = (tickItem: number) => {
        const date = new Date(tickItem * 1000);
        return date.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });
    };

    const formatTooltip = (value: number) => {
        if (formatValue) return formatValue(value);
        return `${value.toFixed(2)}${unit}`;
    };

    return (
        <div className="bg-zinc-900 p-4 rounded-xl border border-zinc-800 shadow-lg">
            <h3 className="text-zinc-400 text-sm font-medium mb-4">{title}</h3>
            <div className="h-64 w-full">
                <ResponsiveContainer width="100%" height="100%">
                    <AreaChart data={data}>
                        <defs>
                            <linearGradient id={`color${dataKey}`} x1="0" y1="0" x2="0" y2="1">
                                <stop offset="5%" stopColor={color} stopOpacity={0.3}/>
                                <stop offset="95%" stopColor={color} stopOpacity={0}/>
                            </linearGradient>
                            {dataKey2 && color2 && (
                                <linearGradient id={`color${dataKey2}`} x1="0" y1="0" x2="0" y2="1">
                                    <stop offset="5%" stopColor={color2} stopOpacity={0.3}/>
                                    <stop offset="95%" stopColor={color2} stopOpacity={0}/>
                                </linearGradient>
                            )}
                        </defs>
                        <CartesianGrid strokeDasharray="3 3" stroke="#333" vertical={false} />
                        <XAxis 
                            dataKey="timestamp" 
                            tickFormatter={formatXAxis} 
                            stroke="#666" 
                            fontSize={12}
                            tickLine={false}
                            axisLine={false}
                        />
                        <YAxis 
                            stroke="#666" 
                            fontSize={12}
                            tickLine={false}
                            axisLine={false}
                            tickFormatter={(val) => formatValue ? formatValue(val) : val.toFixed(0)}
                        />
                        <Tooltip 
                            contentStyle={{ backgroundColor: '#18181b', borderColor: '#27272a', borderRadius: '8px' }}
                            itemStyle={{ color: '#e4e4e7' }}
                            labelFormatter={(label) => new Date(label * 1000).toLocaleString()}
                            formatter={formatTooltip}
                        />
                        <Area 
                            type="monotone" 
                            dataKey={dataKey} 
                            stroke={color} 
                            fillOpacity={1} 
                            fill={`url(#color${dataKey})`} 
                            strokeWidth={2}
                            isAnimationActive={false}
                        />
                        {dataKey2 && color2 && (
                            <Area 
                                type="monotone" 
                                dataKey={dataKey2} 
                                stroke={color2} 
                                fillOpacity={1} 
                                fill={`url(#color${dataKey2})`} 
                                strokeWidth={2}
                                isAnimationActive={false}
                            />
                        )}
                    </AreaChart>
                </ResponsiveContainer>
            </div>
        </div>
    );
};

export default MetricChart;
