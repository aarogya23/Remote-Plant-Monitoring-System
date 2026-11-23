import { useLanguage } from '../../i18n/LanguageContext.tsx'

export interface Metric {
  icon: string
  name: string
  barPct: number
  color: string
  value: string
}

const CIRC = 339.3 // 2π × r(54)

export default function HealthPanel({ health, color, metrics }: { health: number | null; color: string; metrics: Metric[] }) {
  const { dict } = useLanguage()
  const offset = CIRC - ((health ?? 0) / 100) * CIRC

  return (
    <div className="health-card">
      <div className="health-title">{dict.plantHealth}</div>
      <div className="health-sub">{dict.compositeScore}</div>
      <div className="health-ring-wrap">
        <svg width="150" height="150" viewBox="0 0 150 150">
          <circle cx="75" cy="75" r="54" fill="none" stroke="#f1f5f9" strokeWidth="11" />
          <circle
            cx="75" cy="75" r="54" fill="none" stroke={color} strokeWidth="11"
            strokeLinecap="round" strokeDasharray={CIRC} strokeDashoffset={offset.toFixed(1)}
            transform="rotate(-90 75 75)"
            style={{ transition: 'stroke-dashoffset .8s ease,stroke .4s' }}
          />
          <text x="75" y="69" textAnchor="middle" fontFamily="'DM Mono',monospace" fontSize="26" fontWeight="700" fill={color}>
            {health == null ? '--%' : health + '%'}
          </text>
          <text x="75" y="88" textAnchor="middle" fontFamily="'DM Sans',sans-serif" fontSize="11" fontWeight="500" fill="#94a3b8">
            {dict.healthText}
          </text>
        </svg>
      </div>
      <div className="health-metrics">
        {metrics.map((m) => (
          <div key={m.name} className="metric-row">
            <span className="metric-icon">{m.icon}</span>
            <span className="metric-name">{m.name}</span>
            <div className="metric-bar">
              <div className="metric-bar-fill" style={{ width: `${Math.max(0, Math.min(m.barPct, 100))}%`, background: m.color }} />
            </div>
            <span className="metric-val">{m.value}</span>
          </div>
        ))}
      </div>
    </div>
  )
}
