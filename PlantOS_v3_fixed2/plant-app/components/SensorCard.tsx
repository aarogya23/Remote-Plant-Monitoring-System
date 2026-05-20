import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

interface SensorCardProps {
  title: string;
  value: string;
  unit: string;
  icon: string;
  color: string;
  progress: number;
  status: string;
  statusText: string;
  optimalText: string;
}

export default function SensorCard({
  title,
  value,
  unit,
  icon,
  color,
  progress,
  status,
  statusText,
  optimalText,
}: SensorCardProps) {
  return (
    <View style={styles.card}>
      <View style={styles.cardHeader}>
        <View style={styles.cardIcon}>
          <Text style={{ fontSize: 18 }}>{icon}</Text>
        </View>
        <Text style={[styles.cardDelta, { color }]}>{status}</Text>
      </View>
      <Text style={styles.cardLabel}>{title}</Text>
      <View style={{ flexDirection: 'row', alignItems: 'flex-end', marginBottom: 10 }}>
        <Text style={[styles.cardValue, { color }]}>{value}</Text>
        <Text style={styles.cardUnit}>{unit}</Text>
      </View>
      
      <View style={styles.cardBarTrack}>
        <View style={[styles.cardBarFill, { backgroundColor: color, width: `${progress}%` }]} />
      </View>
      
      <View style={styles.cardSub}>
        <View style={[styles.statusDot, { backgroundColor: color }]} />
        <Text style={[styles.statusText, { color }]}>{statusText}</Text>
        <Text style={styles.optimalText}>{optimalText}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#fff',
    borderRadius: 14,
    padding: 16,
    borderWidth: 1,
    borderColor: '#e8ecf0',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.04,
    shadowRadius: 4,
    elevation: 2,
    marginBottom: 16,
    width: '48%', // Allows 2 cards per row if using flexWrap
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
    marginBottom: 12,
  },
  cardIcon: {
    width: 40,
    height: 40,
    borderRadius: 11,
    backgroundColor: '#f8fafc',
    borderWidth: 1,
    borderColor: '#e8ecf0',
    alignItems: 'center',
    justifyContent: 'center',
  },
  cardDelta: {
    fontSize: 11.5,
    fontWeight: '700',
    fontFamily: 'Courier',
  },
  cardLabel: {
    fontSize: 12,
    color: '#7a8694',
    fontWeight: '500',
    marginBottom: 2,
  },
  cardValue: {
    fontSize: 34,
    fontWeight: '800',
    fontFamily: 'Courier',
    lineHeight: 34,
  },
  cardUnit: {
    fontSize: 15,
    fontWeight: '500',
    color: '#7a8694',
    marginLeft: 2,
    paddingBottom: 2,
  },
  cardBarTrack: {
    height: 5,
    backgroundColor: '#f1f5f9',
    borderRadius: 10,
    overflow: 'hidden',
    width: '100%',
  },
  cardBarFill: {
    height: '100%',
    borderRadius: 10,
  },
  cardSub: {
    marginTop: 10,
    flexDirection: 'row',
    alignItems: 'center',
    flexWrap: 'wrap',
  },
  statusDot: {
    width: 7,
    height: 7,
    borderRadius: 3.5,
    marginRight: 6,
  },
  statusText: {
    fontSize: 11.5,
    fontWeight: '600',
    marginRight: 4,
  },
  optimalText: {
    fontSize: 11,
    color: '#7a8694',
    fontWeight: '400',
  },
});
