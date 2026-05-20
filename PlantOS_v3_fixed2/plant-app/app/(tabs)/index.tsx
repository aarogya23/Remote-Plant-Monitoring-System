import React, { useState, useEffect } from 'react';
import { StyleSheet, View, Text, ScrollView, SafeAreaView, Platform } from 'react-native';
import EventSource from 'react-native-sse';
import SensorCard from '../../components/SensorCard';
import SparklineChart from '../../components/SparklineChart';
import HealthRing from '../../components/HealthRing';

// ==========================================
// IMPORTANT: REPLACE THIS WITH YOUR API IP!
// Example: 'http://192.168.1.100:8080/api/stream'
// ==========================================
const API_URL = 'http://10.21.19.184:8080/api/stream';

interface SensorData {
  temp: number;
  humidity: number;
  soil: number;
  ph: number;
  lux: number;
  batPct: number;
  batV: number;
  health: number;
  tempStatus: string;
  soilStatus: string;
  phStatus: string;
  lightStatus?: string;
  batStatus: string;
}

export default function DashboardScreen() {
  const [data, setData] = useState<SensorData | null>(null);
  const [status, setStatus] = useState('Connecting...');

  const [chartData, setChartData] = useState({
    tD: Array(40).fill(0),
    hD: Array(40).fill(0),
    sD: Array(40).fill(0),
    pD: Array(40).fill(7),
    lD: Array(40).fill(0),
  });

  const colorFor = (type: string, val: number) => {
    if (type === 'temp') return (val < 15 || val > 35) ? '#ef4444' : '#22c55e';
    if (type === 'hum') return (val < 30 || val > 80) ? '#f59e0b' : '#3b82f6';
    if (type === 'soil') return val < 30 ? '#ef4444' : val < 40 ? '#f59e0b' : '#22c55e';
    if (type === 'ph') return (val < 5.0 || val > 8.0) ? '#ef4444' : (val < 5.5 || val > 7.5) ? '#f59e0b' : '#22c55e';
    if (type === 'lux') return val < 100 ? '#ef4444' : val < 500 ? '#f59e0b' : '#22c55e';
    if (type === 'bat') return val < 30 ? '#ef4444' : val < 70 ? '#f59e0b' : '#22c55e';
    if (type === 'health') return val >= 75 ? '#22c55e' : val >= 50 ? '#f59e0b' : '#ef4444';
    return '#22c55e';
  };

  const luxLabel = (v: number) => {
    if (v < 100) return 'Very dark';
    if (v < 500) return 'Dim';
    if (v < 2000) return 'Indoor';
    if (v < 10000) return 'Bright';
    if (v < 50000) return 'Sunny';
    return 'Intense';
  };

  useEffect(() => {
    const handleNewData = (json: SensorData) => {
      setStatus('• Online');
      setData(json);

      setChartData(prev => {
        const newTD = [...prev.tD.slice(1), json.temp];
        const newHD = [...prev.hD.slice(1), json.humidity];
        const newSD = [...prev.sD.slice(1), json.soil];
        const newPD = [...prev.pD.slice(1), json.ph];
        const newLD = [...prev.lD.slice(1), json.lux / 1000];
        return { tD: newTD, hD: newHD, sD: newSD, pD: newPD, lD: newLD };
      });
    };

    // 1. Initial fetch to get the latest data immediately
    const fetchLatest = async () => {
      try {
        const response = await fetch(API_URL.replace('/stream', '/latest'));
        if (response.ok) {
          const json = await response.json();
          handleNewData(json);
        }
      } catch (error) {
        setStatus('• Reconnecting...');
      }
    };

    fetchLatest();

    // 2. Connect to the SSE stream for live real-time pushed updates
    const es = new EventSource(API_URL);

    es.addEventListener('message', (event) => {
      if (event.data) {
        try {
          const json = JSON.parse(event.data);
          handleNewData(json);
        } catch (e) {
          console.error('Error parsing SSE data', e);
        }
      }
    });

    es.addEventListener('error', (event) => {
      setStatus('• Reconnecting...');
    });

    return () => {
      es.removeAllEventListeners();
      es.close();
    };
  }, []);

  const getAlert = () => {
    if (!data) return { icon: '⏳', msg: 'Waiting for data...', color: '#7a8694', bg: '#f8fafc', border: '#e8ecf0' };
    if (data.soil < 30) return { icon: '⚠️', msg: 'WATER YOUR PLANT NOW!', color: '#dc2626', bg: '#fef2f2', border: '#ef4444' };
    if (data.ph < 5.0 || data.ph > 8.5) return { icon: '⚠️', msg: 'Soil pH out of range — check fertiliser!', color: '#b45309', bg: '#fffbeb', border: '#f59e0b' };
    if (data.batPct < 20) return { icon: '⚠️', msg: 'Battery low — charge soon!', color: '#b45309', bg: '#fffbeb', border: '#f59e0b' };
    return { icon: '✅', msg: 'Plant is healthy & happy', color: '#16a34a', bg: '#f0fdf4', border: '#22c55e' };
  };

  const alert = getAlert();

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.topbar}>
        <View>
          <Text style={styles.topbarTitle}>Plant Dashboard</Text>
          <Text style={styles.topbarSub}>Last reading: {new Date().toLocaleTimeString()}</Text>
        </View>
        <View style={styles.liveBadge}>
          <View style={styles.liveDot} />
          <Text style={styles.liveText}>Live</Text>
        </View>
      </View>

      <ScrollView contentContainerStyle={styles.scrollContent}>

        <View style={styles.deviceChip}>
          <View style={[styles.deviceDot, { backgroundColor: status === '• Online' ? '#22c55e' : '#f59e0b' }]} />
          <View>
            <Text style={styles.deviceName}>ESP32-AGRO</Text>
            <Text style={[styles.deviceStatus, { color: status === '• Online' ? '#22c55e' : '#f59e0b' }]}>{status}</Text>
          </View>
        </View>

        <View style={[styles.alertBanner, { backgroundColor: alert.bg, borderColor: alert.border }]}>
          <Text style={{ fontSize: 18 }}>{alert.icon}</Text>
          <Text style={[styles.alertMsg, { color: alert.color }]}>{alert.msg}</Text>
        </View>

        {data ? (
          <>
            <View style={styles.cardsGrid}>
              <SensorCard
                title="Temperature"
                value={data.temp.toString()}
                unit="°C"
                icon="🌡️"
                color={colorFor('temp', data.temp)}
                progress={Math.min((data.temp / 50) * 100, 100)}
                status={data.tempStatus || 'Normal'}
                statusText={data.tempStatus || 'Normal'}
                optimalText="Optimal 15–35°C"
              />
              <SensorCard
                title="Humidity"
                value={data.humidity.toString()}
                unit="%"
                icon="💧"
                color={colorFor('hum', data.humidity)}
                progress={data.humidity}
                status={`${data.humidity}%`}
                statusText="Normal"
                optimalText="Optimal 40–70%"
              />
              <SensorCard
                title="Soil Moisture"
                value={data.soil.toString()}
                unit="%"
                icon="🌱"
                color={colorFor('soil', data.soil)}
                progress={data.soil}
                status={data.soilStatus || 'Moist'}
                statusText={data.soilStatus || 'Moist'}
                optimalText="Optimal 35–65%"
              />
              <SensorCard
                title="Soil pH"
                value={Number(data.ph).toFixed(2)}
                unit="pH"
                icon="⚖️"
                color={colorFor('ph', data.ph)}
                progress={(data.ph / 14) * 100}
                status={data.phStatus || 'Neutral'}
                statusText={data.phStatus || 'Neutral'}
                optimalText="Optimal 5.5–7.5"
              />
              <SensorCard
                title="Light"
                value={data.lux.toString()}
                unit="lx"
                icon="☀️"
                color={colorFor('lux', data.lux)}
                progress={Math.min((data.lux / 80000) * 100, 100)}
                status={data.lightStatus || luxLabel(data.lux)}
                statusText={luxLabel(data.lux)}
                optimalText="500–20k lx ideal"
              />
              <SensorCard
                title="Battery"
                value={data.batPct.toString()}
                unit="%"
                icon="🔋"
                color={colorFor('bat', data.batPct)}
                progress={data.batPct}
                status={data.batStatus || 'Good'}
                statusText={`${Number(data.batV).toFixed(2)}V`}
                optimalText="7.4V nominal"
              />
            </View>

            <SparklineChart data={chartData} />

            <View style={styles.healthCard}>
              <Text style={styles.chartTitle}>Plant Health</Text>
              <Text style={styles.chartSub}>Composite score — all 6 sensors</Text>
              <HealthRing healthPct={data.health} color={colorFor('health', data.health)} />
            </View>
          </>
        ) : (
          <View style={{ padding: 40, alignItems: 'center' }}>
            <Text style={{ color: '#7a8694' }}>Waiting for sensor data...</Text>
            <Text style={{ color: '#7a8694', marginTop: 10, fontSize: 12 }}>Check your API_URL ({API_URL})</Text>
          </View>
        )}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f4f6f9',
    paddingTop: Platform.OS === 'android' ? 25 : 0,
  },
  topbar: {
    backgroundColor: '#fff',
    padding: 16,
    borderBottomWidth: 1,
    borderColor: '#e8ecf0',
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  topbarTitle: {
    fontSize: 20,
    fontWeight: '700',
    letterSpacing: -0.4,
  },
  topbarSub: {
    fontSize: 12,
    color: '#7a8694',
    marginTop: 2,
    fontFamily: 'Courier',
  },
  liveBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#f0fdf4',
    borderWidth: 1,
    borderColor: '#bbf7d0',
    paddingVertical: 5,
    paddingHorizontal: 12,
    borderRadius: 20,
  },
  liveDot: {
    width: 7,
    height: 7,
    backgroundColor: '#22c55e',
    borderRadius: 3.5,
    marginRight: 6,
  },
  liveText: {
    fontSize: 12,
    fontWeight: '600',
    color: '#16a34a',
  },
  scrollContent: {
    padding: 16,
    paddingBottom: 40,
  },
  deviceChip: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#fff',
    borderWidth: 1,
    borderColor: '#e8ecf0',
    padding: 12,
    borderRadius: 10,
    marginBottom: 16,
  },
  deviceDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 10,
  },
  deviceName: {
    fontSize: 12,
    fontWeight: '600',
  },
  deviceStatus: {
    fontSize: 11,
    fontWeight: '500',
  },
  alertBanner: {
    padding: 14,
    borderRadius: 12,
    borderWidth: 1.5,
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
  },
  alertMsg: {
    fontSize: 14,
    fontWeight: '600',
    marginLeft: 10,
  },
  cardsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  healthCard: {
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
  },
  chartTitle: {
    fontSize: 15,
    fontWeight: '700',
  },
  chartSub: {
    fontSize: 12,
    color: '#7a8694',
    marginTop: 2,
  },
});
