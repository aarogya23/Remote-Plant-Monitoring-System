import { useLanguage } from '../../i18n/LanguageContext.tsx'
import type { Recommendation } from '../../lib/sensors.ts'

export default function Recommendations({ recs }: { recs: Recommendation[] | null }) {
  const { dict } = useLanguage()
  return (
    <div className="card recommendations-card">
      <div className="chart-title">{dict.recsTitle}</div>
      <div className="chart-sub">{dict.recsSub}</div>
      <div className="rec-list">
        {recs && recs.length === 0 && <div className="rec-all-ok">✔ {dict.allOptimal}</div>}
        {recs?.map((rec) => (
          <div key={rec.sensor} className="rec-item" style={{ borderLeftColor: rec.color }}>
            <div>
              <strong className="rec-sensor">{rec.sensor} ({rec.val}):</strong>
              <span className="rec-action">{rec.action}</span>
            </div>
            <span className="rec-badge" style={{ background: rec.color + '15', color: rec.color }}>{rec.status}</span>
          </div>
        ))}
      </div>
    </div>
  )
}
