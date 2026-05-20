import React, { useEffect, useRef } from 'react';
import { View, Text, StyleSheet, Animated } from 'react-native';
import Svg, { Circle } from 'react-native-svg';

const AnimatedCircle = Animated.createAnimatedComponent(Circle);

interface HealthRingProps {
  healthPct: number;
  color: string;
}

export default function HealthRing({ healthPct, color }: HealthRingProps) {
  const radius = 54;
  const circumference = 2 * Math.PI * radius; // ~339.3
  
  const animatedValue = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    Animated.timing(animatedValue, {
      toValue: healthPct,
      duration: 800,
      useNativeDriver: true,
    }).start();
  }, [healthPct]);

  const strokeDashoffset = animatedValue.interpolate({
    inputRange: [0, 100],
    outputRange: [circumference, 0],
  });

  return (
    <View style={styles.container}>
      <Svg width="150" height="150" viewBox="0 0 150 150">
        <Circle 
          cx="75" 
          cy="75" 
          r={radius} 
          fill="none" 
          stroke="#f1f5f9" 
          strokeWidth="11" 
        />
        <AnimatedCircle
          cx="75"
          cy="75"
          r={radius}
          fill="none"
          stroke={color}
          strokeWidth="11"
          strokeLinecap="round"
          strokeDasharray={`${circumference} ${circumference}`}
          strokeDashoffset={strokeDashoffset}
          transform="rotate(-90 75 75)"
          origin="75, 75"
        />
      </Svg>
      <View style={styles.textContainer}>
        <Text style={[styles.healthText, { color }]}>{healthPct}%</Text>
        <Text style={styles.healthLabel}>HEALTH</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    justifyContent: 'center',
    alignItems: 'center',
    marginVertical: 14,
    position: 'relative',
  },
  textContainer: {
    position: 'absolute',
    alignItems: 'center',
    justifyContent: 'center',
  },
  healthText: {
    fontFamily: 'Courier',
    fontSize: 26,
    fontWeight: '700',
  },
  healthLabel: {
    fontSize: 11,
    fontWeight: '500',
    color: '#94a3b8',
    marginTop: 2,
  },
});
