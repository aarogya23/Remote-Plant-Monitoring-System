import type { Period, ReportStats, SensorReading } from './types.ts'

// Empty by default: in dev, Vite proxies /api to the Spring Boot server (see vite.config.ts).
// Set VITE_API_BASE (e.g. http://10.99.13.74:8080) when the backend lives elsewhere.
export const API_BASE: string = import.meta.env.VITE_API_BASE ?? ''

async function getJson<T>(path: string): Promise<T> {
  const r = await fetch(API_BASE + path)
  if (!r.ok) throw new Error(String(r.status))
  return r.json() as Promise<T>
}

/** Latest reading, or null when the database is empty (204). */
export async function fetchLatest(): Promise<SensorReading | null> {
  const r = await fetch(API_BASE + '/api/latest')
  if (!r.ok) throw new Error(String(r.status))
  if (r.status === 204) return null
  return r.json() as Promise<SensorReading>
}

export const fetchMock = () => getJson<SensorReading>('/api/mock')

export const fetchStats = (period: Period) =>
  getJson<ReportStats>('/api/report/statistics?period=' + period)

export const fetchReadings = (period: Period) =>
  getJson<SensorReading[]>('/api/report/readings?period=' + period)

export const openStream = () => new EventSource(API_BASE + '/api/stream')
