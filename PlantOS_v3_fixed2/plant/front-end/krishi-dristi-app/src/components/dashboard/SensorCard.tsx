import type { ReactNode } from 'react'

interface Props {
  icon: string
  label: string
  value: string
  unit?: string
  color: string
  /** Text in the top-right corner of the card */
  delta: string
  barPct: number
  /** Coloured status text next to the dot; omitted on cards that only show a note */
  status?: string
  note?: string
  /** Rendered between the value and the bar (e.g. the light/dark timers) */
  children?: ReactNode
}

export default function SensorCard({ icon, label, value, unit, color, delta, barPct, status, note, children }: Props) {
  return (
    <div className="card">
      <div className="card-header">
        <div className="card-icon">{icon}</div>
        <span className="card-delta" style={{ color }}>{delta}</span>
      </div>
      <div className="card-label">{label}</div>
      <div>
        {/* Keyed on the value so the flash animation replays on every update */}
        <span key={value} className="card-value updated" style={{ color }}>{value}</span>
        {unit && <span className="card-unit">{unit}</span>}
      </div>
      {children}
      <div className="card-bar-track">
        <div className="card-bar-fill" style={{ width: `${Math.max(0, Math.min(barPct, 100))}%`, background: color }} />
      </div>
      <div className="card-sub">
        <div className="status-dot" style={{ background: color }} />
        {status && <span style={{ color }}>{status}</span>}
        {note && <span className="card-note">{status ? ' ' : ''}{note}</span>}
      </div>
    </div>
  )
}
