import { HISTORY_SIZE, type HistoryPoint } from '../../hooks/useSensorFeed.ts'
import { useLanguage } from '../../i18n/LanguageContext.tsx'
import { COLORS } from '../../lib/sensors.ts'

function pts(data: number[], lo: number, hi: number): string {
  return data
    .map((v, i) => {
      const x = (i / (HISTORY_SIZE - 1)) * 600
      const y = Math.max(2, Math.min(198, 200 - ((v - lo) / (hi - lo)) * 200))
      return x.toFixed(1) + ',' + y.toFixed(1)
    })
    .join(' ')
}

export default function SensorHistoryChart({ history }: { history: HistoryPoint[] }) {
  const { dict } = useLanguage()
  const col = (k: keyof HistoryPoint) => history.map((p) => p[k])

  const legend = [
    { color: COLORS.red, label: dict.legendTemp },
    { color: COLORS.blue, label: dict.legendHum },
    { color: COLORS.green, label: dict.legendSoil },
    { color: COLORS.purple, label: dict.legendPh },
    { color: COLORS.amber, label: dict.legendLight },
  ]

  return (
    <div className="chart-card">
      <div className="chart-title">{dict.sensorHistory}</div>
      <div className="chart-sub">{dict.chartSub}</div>
      <div className="chart-area">
        <svg className="sparkline" viewBox="0 0 600 200" preserveAspectRatio="none">
          {[40, 80, 120, 160].map((y) => (
            <line key={y} x1="0" y1={y} x2="600" y2={y} stroke="#f1f5f9" strokeWidth="1" />
          ))}
          <polyline points={pts(col('temp'), 0, 60)} fill="none" stroke={COLORS.red} strokeWidth="2" strokeLinejoin="round" />
          <polyline points={pts(col('humidity'), 0, 100)} fill="none" stroke={COLORS.blue} strokeWidth="2" strokeLinejoin="round" />
          <polyline points={pts(col('soil'), 0, 100)} fill="none" stroke={COLORS.green} strokeWidth="2" strokeLinejoin="round" />
          <polyline points={pts(col('ph'), 0, 14)} fill="none" stroke={COLORS.purple} strokeWidth="1.5" strokeLinejoin="round" strokeDasharray="5 2" />
          <polyline points={pts(col('lux').map((v) => v * 80), 0, 80)} fill="none" stroke={COLORS.amber} strokeWidth="1.5" strokeLinejoin="round" strokeDasharray="3 2" />
        </svg>
      </div>
      <div className="legend">
        {legend.map((l) => (
          <div key={l.color} className="legend-item">
            <div className="legend-dot" style={{ background: l.color }} /> {l.label}
          </div>
        ))}
      </div>
    </div>
  )
}
