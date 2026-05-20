import React from 'react';
import { View, StyleSheet, Text } from 'react-native';
import Svg, { Line, Polyline } from 'react-native-svg';

interface ChartData {
  tD: number[];
  hD: number[];
  sD: number[];
  pD: number[];
  lD: number[];
}

export default function SparklineChart({ data }: { data: ChartData }) {
  const pts = (arr: number[], lo: number, hi: number) => {
    return arr.map((val, i) => {
      const x = (i / 39) * 600; // max length is 40
      let y = 200 - ((val - lo) / (hi - lo)) * 200;
      y = Math.max(2, Math.min(198, y));
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    }).join(' ');
  };

  return (
    <View style={styles.card}>
      <Text style={styles.title}>Sensor History</Text>
      <Text style={styles.subtitle}>Live rolling window — last 40 readings</Text>
      
      <View style={styles.chartArea}>
        <Svg viewBox="0 0 600 200" preserveAspectRatio="none" style={styles.svg}>
          <Line x1="0" y1="40" x2="600" y2="40" stroke="#f1f5f9" strokeWidth="1" />
          <Line x1="0" y1="80" x2="600" y2="80" stroke="#f1f5f9" strokeWidth="1" />
          <Line x1="0" y1="120" x2="600" y2="120" stroke="#f1f5f9" strokeWidth="1" />
          <Line x1="0" y1="160" x2="600" y2="160" stroke="#f1f5f9" strokeWidth="1" />
          
          <Polyline points={pts(data.tD, 0, 60)} fill="none" stroke="#ef4444" strokeWidth="2" strokeLinejoin="round" />
          <Polyline points={pts(data.hD, 0, 100)} fill="none" stroke="#3b82f6" strokeWidth="2" strokeLinejoin="round" />
          <Polyline points={pts(data.sD, 0, 100)} fill="none" stroke="#22c55e" strokeWidth="2" strokeLinejoin="round" />
          <Polyline points={pts(data.pD, 0, 14)} fill="none" stroke="#8b5cf6" strokeWidth="1.5" strokeLinejoin="round" strokeDasharray="5 2" />
          <Polyline points={pts(data.lD, 0, 80)} fill="none" stroke="#f59e0b" strokeWidth="1.5" strokeLinejoin="round" strokeDasharray="3 2" />
        </Svg>
      </View>
      
      <View style={styles.legend}>
        <LegendItem color="#ef4444" label="Temp (°C)" />
        <LegendItem color="#3b82f6" label="Humidity (%)" />
        <LegendItem color="#22c55e" label="Soil (%)" />
        <LegendItem color="#8b5cf6" label="pH" />
        <LegendItem color="#f59e0b" label="Light (klx)" />
      </View>
    </View>
  );
}

function LegendItem({ color, label }: { color: string; label: string }) {
  return (
    <View style={styles.legendItem}>
      <View style={[styles.legendDot, { backgroundColor: color }]} />
      <Text style={styles.legendText}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#fff',
    borderRadius: 14,
    padding: 20,
    borderWidth: 1,
    borderColor: '#e8ecf0',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.04,
    shadowRadius: 4,
    elevation: 2,
    marginBottom: 16,
  },
  title: {
    fontSize: 15,
    fontWeight: '700',
  },
  subtitle: {
    fontSize: 12,
    color: '#7a8694',
    marginTop: 2,
  },
  chartArea: {
    width: '100%',
    height: 220,
    marginTop: 16,
  },
  svg: {
    width: '100%',
    height: '100%',
  },
  legend: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginTop: 12,
    gap: 14, // Works in newer React Native
  },
  legendItem: {
    flexDirection: 'row',
    alignItems: 'center',
    marginRight: 10,
    marginBottom: 6,
  },
  legendDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 6,
  },
  legendText: {
    fontSize: 12,
    color: '#7a8694',
    fontWeight: '500',
  },
});
