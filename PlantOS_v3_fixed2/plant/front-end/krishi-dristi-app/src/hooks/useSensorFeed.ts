import { useCallback, useEffect, useRef, useState } from 'react'
import { fetchLatest, fetchMock, openStream } from '../lib/api.ts'
import { toDashboardData } from '../lib/sensors.ts'
import type { DashboardData, SensorReading } from '../lib/types.ts'

export const HISTORY_SIZE = 40

export interface HistoryPoint {
  temp: number
  humidity: number
  soil: number
  ph: number
  lux: number
}

export type ConnState = 'online' | 'reconnecting'

const EMPTY_HISTORY: HistoryPoint[] = Array.from({ length: HISTORY_SIZE }, () => ({
  temp: 0, humidity: 0, soil: 0, ph: 7, lux: 0,
}))

/**
 * Live sensor data: loads /api/latest once, then follows the /api/stream SSE feed,
 * reopening it 3 s after an error. After 3 failures in a row the state flips to
 * "reconnecting" until data flows again.
 */
export function useSensorFeed() {
  const [data, setData] = useState<DashboardData | null>(null)
  const [readingAt, setReadingAt] = useState<number | null>(null)
  const [history, setHistory] = useState<HistoryPoint[]>(EMPTY_HISTORY)
  const [conn, setConn] = useState<ConnState>('online')
  const fails = useRef(0)
  const stream = useRef<EventSource | null>(null)

  const apply = useCallback((raw: Partial<SensorReading>) => {
    const d = toDashboardData(raw)
    fails.current = 0
    setConn('online')
    setData(d)
    // Use the server timestamp when there is one (/api/latest can be an old row);
    // stream messages carry no ts, so they count as "now"
    const now = Date.now()
    const ts = d.ts ? Date.parse(d.ts) : NaN
    setReadingAt(Number.isNaN(ts) ? now : Math.min(ts, now))
    setHistory((h) =>
      [...h, { temp: d.temp, humidity: d.humidity, soil: d.soil, ph: d.ph, lux: d.lux }].slice(-HISTORY_SIZE),
    )
  }, [])

  const fail = useCallback(() => {
    fails.current++
    if (fails.current >= 3) setConn('reconnecting')
  }, [])

  useEffect(() => {
    let cancelled = false
    let retry: ReturnType<typeof setTimeout> | undefined

    fetchLatest()
      .then((d) => { if (!cancelled && d) apply(d) })
      .catch((err) => { console.error('Fetch error:', err); if (!cancelled) fail() })

    const connect = () => {
      const es = openStream()
      stream.current = es
      es.onmessage = (e) => {
        try {
          apply(JSON.parse(e.data))
        } catch (ex) {
          console.error('Parse error:', ex)
        }
      }
      es.onerror = () => {
        fail()
        es.close()
        if (!cancelled) retry = setTimeout(connect, 3000)
      }
    }
    connect()

    return () => {
      cancelled = true
      clearTimeout(retry)
      stream.current?.close()
      stream.current = null
    }
  }, [apply, fail])

  /** Ask the backend for a random reading (the "Test Data" button). */
  const generateTestData = useCallback(async () => {
    try {
      const d = await fetchMock()
      // The backend also broadcasts the mock over SSE; only apply it here when the
      // stream is down, so the chart doesn't get the same point twice.
      if (stream.current?.readyState !== EventSource.OPEN) apply(d)
    } catch (err) {
      console.error('Mock data error:', err)
    }
  }, [apply])

  return { data, readingAt, history, conn, generateTestData }
}
