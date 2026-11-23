export type Lang = 'en' | 'ne'

/** A reading as returned by /api/latest, /api/mock, /api/stream and /api/report/readings. */
export interface SensorReading {
  id?: number
  ts?: string
  temp: number
  humidity: number
  soil: number
  ph: number
  /** 1 = light, 0 = dark (LDR digital output) */
  lux: number
  batV: number
  batPct: number
  health: number
  /** Light/dark time accumulated on the device since power-on; null on older rows */
  lightSec?: number | null
  darkSec?: number | null
  lightPct?: number | null
  uptimeSec?: number | null
}

/** A reading plus the status labels the dashboard derives from it. */
export interface DashboardData extends SensorReading {
  tempStatus: 'Alert' | 'Normal'
  soilStatus: 'Dry' | 'Moist' | 'Wet'
  phStatus: 'Critical' | 'Warning' | 'Neutral'
  batStatus: 'Low' | 'Medium' | 'Good'
  lightStatus: 'Light' | 'Dark'
}

export interface ReportStats {
  count: number
  averageTemp: number
  averageHumidity: number
  averageSoil: number
  averagePh: number
  averageLux: number
  averageBattery: number
  averageHealth: number
  lightHours?: number
  darkHours?: number
  lightSharePct?: number
}

export type Period =
  | 'today'
  | 'last3days'
  | 'week'
  | 'twoweeks'
  | 'month'
  | 'threemonths'
  | 'sixmonths'
  | 'year'
  | 'all'
