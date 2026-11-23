import { useNow } from '../../hooks/useNow.ts'
import { useLanguage } from '../../i18n/LanguageContext.tsx'
import { fmtClock } from '../../lib/sensors.ts'
import type { SensorReading } from '../../lib/types.ts'

interface Props {
  reading: SensorReading | null
  /** When the reading was taken (ms) — the base for local ticking */
  readingAt: number | null
}

/**
 * Light/dark clock. The device sends running totals every ~10 s; between packets we keep
 * counting locally into whichever state it last reported, so it ticks like the LCD.
 */
export default function LightTimers({ reading, readingAt }: Props) {
  const { dict } = useLanguage()
  const now = useNow(1000)

  const hasClock = reading != null && readingAt != null && reading.lightSec != null && reading.darkSec != null
  let light = '--:--:--', dark = '--:--:--'
  const isLight = hasClock && reading.lux === 1
  if (hasClock) {
    // Stop extrapolating if the device goes quiet, so a dead link doesn't inflate the totals
    const extra = Math.min((now - readingAt) / 1000, 60)
    light = fmtClock(+reading.lightSec! + (isLight ? extra : 0))
    dark = fmtClock(+reading.darkSec! + (isLight ? 0 : extra))
  }

  return (
    <div className="light-timers">
      <div className={`light-timer light-timer-on${hasClock && isLight ? ' active' : ''}`} title={dict.lightTimerTitle}>
        <span className="lt-icon">☀</span><span>{light}</span>
      </div>
      <div className={`light-timer dark-timer-on${hasClock && !isLight ? ' active' : ''}`} title={dict.darkTimerTitle}>
        <span className="lt-icon">☾</span><span>{dark}</span>
      </div>
    </div>
  )
}
