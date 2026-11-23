import { useMemo } from 'react'
import { useFeed } from '../components/Layout.tsx'
import HealthPanel, { type Metric } from '../components/dashboard/HealthPanel.tsx'
import LightTimers from '../components/dashboard/LightTimers.tsx'
import Recommendations from '../components/dashboard/Recommendations.tsx'
import SensorCard from '../components/dashboard/SensorCard.tsx'
import SensorHistoryChart from '../components/dashboard/SensorHistoryChart.tsx'
import LanguageSelect from '../components/LanguageSelect.tsx'
import { useNow } from '../hooks/useNow.ts'
import { useLanguage } from '../i18n/LanguageContext.tsx'
import { COLORS, colorFor, fmtTime, getRecommendations, lightShareOf } from '../lib/sensors.ts'

const ALERT_STYLES = {
  red: { borderColor: COLORS.red, background: '#fef2f2', color: '#dc2626' },
  amber: { borderColor: COLORS.amber, background: '#fffbeb', color: '#b45309' },
  green: { borderColor: COLORS.green, background: '#f0fdf4', color: '#16a34a' },
}

export default function Dashboard() {
  const { lang, dict } = useLanguage()
  const { data: d, readingAt, history, generateTestData } = useFeed()
  const now = useNow(1000)

  const recs = useMemo(() => (d ? getRecommendations(d, lang) : null), [d, lang])

  // Colours fall back to the page's initial colours until the first reading arrives
  const tc = d ? colorFor('temp', d.temp) : COLORS.green
  const hc = d ? colorFor('hum', d.humidity) : COLORS.blue
  const sc = d ? colorFor('soil', d.soil) : COLORS.green
  const pc = d ? colorFor('ph', d.ph) : COLORS.green
  const lc = d ? colorFor('lux', d.lux) : COLORS.green
  const bc = d ? colorFor('bat', d.batPct) : COLORS.green
  const hc2 = d ? colorFor('health', d.health) : COLORS.green

  const share = d ? lightShareOf(d) : null
  const luxBarPct = share ?? (d?.lux === 1 ? 100 : 0)
  const lightState = d ? dict[d.lightStatus] : '--'
  const lightSub = !d ? '--'
    : share != null ? `${share}% ${dict.lightShare}`
    : d.lux === 1 ? dict.LightPresent : dict.Dark

  const metrics: Metric[] = [
    { icon: '🌡', name: dict.metTemp, barPct: d ? (d.temp / 50) * 100 : 0, color: d ? tc : COLORS.red, value: d ? d.temp + '°C' : '--°C' },
    { icon: '💧', name: dict.metHum, barPct: d?.humidity ?? 0, color: hc, value: d ? d.humidity + '%' : '--%' },
    { icon: '🌿', name: dict.metSoil, barPct: d?.soil ?? 0, color: sc, value: d ? d.soil + '%' : '--%' },
    { icon: '⚖', name: dict.metPh, barPct: d ? (d.ph / 14) * 100 : 0, color: d ? pc : COLORS.purple, value: d ? (+d.ph).toFixed(2) : '--' },
    { icon: '☀', name: dict.metLight, barPct: luxBarPct, color: lc, value: !d ? '--' : share != null ? share + '%' : lightState },
    { icon: '🔋', name: dict.metBattery, barPct: d?.batPct ?? 0, color: bc, value: d ? d.batPct + '%' : '--%' },
  ]

  let alert: { style: keyof typeof ALERT_STYLES; icon: string; msg: string } = { style: 'green', icon: '🌿', msg: dict.healthyMsg }
  if (d) {
    if (d.soil < 30) alert = { style: 'red', icon: '⚠️', msg: dict.waterMsg }
    else if (d.ph < 5.0 || d.ph > 8.5) alert = { style: 'amber', icon: '⚠️', msg: dict.phMsg }
    else if (d.batPct < 20) alert = { style: 'amber', icon: '⚠️', msg: dict.batMsg }
    else alert = { style: 'green', icon: '✅', msg: dict.healthyMsg }
  }

  return (
    <>
      <div className="topbar">
        <div className="topbar-title">
          <h2>{dict.dashboardTitle}</h2>
          <p>{dict.lastRead}{readingAt ? fmtTime(new Date(readingAt)) : '--:--:--'}</p>
        </div>
        <div className="topbar-right">
          <LanguageSelect />
          <button type="button" className="btn btn-primary" onClick={generateTestData}>{dict.testData}</button>
          <div className="time-display">{fmtTime(new Date(now))}</div>
          <div className="live-badge"><div className="live-dot" /> <span>{dict.live}</span></div>
        </div>
      </div>

      <div className="content">
        <div className="wifi-info">
          📡 {dict.hotspot}: <strong>{dict.hotspotName}</strong> &nbsp;|&nbsp; {dict.password}: <strong>plant1234</strong>
          &nbsp;|&nbsp; {dict.open}: <strong>http://192.168.4.1</strong>
        </div>

        <div className="pin-info">
          📌 {dict.pinSoil} → GPIO34 &nbsp;|&nbsp; pH → GPIO35 &nbsp;|&nbsp; {dict.pinBattery} → GPIO33 &nbsp;|&nbsp; DHT22 → GPIO4
          &nbsp;|&nbsp; LDR → GPIO2
        </div>

        <div className="alert-banner" style={ALERT_STYLES[alert.style]}>
          <span className="alert-icon">{alert.icon}</span>
          <span>{alert.msg}</span>
        </div>

        <Recommendations recs={recs} />

        <div className="cards-grid">
          <SensorCard
            icon="🌡" label={dict.tempLabel} value={d ? String(d.temp) : '--'} unit="°C" color={tc}
            delta={d ? dict[d.tempStatus] : dict.Normal} barPct={d ? (d.temp / 50) * 100 : 0}
            status={d ? dict[d.tempStatus] : dict.Normal} note={dict.optimalTemp}
          />
          <SensorCard
            icon="💧" label={dict.humLabel} value={d ? String(d.humidity) : '--'} unit="%" color={hc}
            delta={d ? d.humidity + '%' : '--%'} barPct={d?.humidity ?? 0} note={dict.optimalHum}
          />
          <SensorCard
            icon="🌿" label={dict.soilLabel} value={d ? String(d.soil) : '--'} unit="%" color={sc}
            delta={d ? dict[d.soilStatus] : dict.Moist} barPct={d?.soil ?? 0}
            status={d ? dict[d.soilStatus] : dict.Moist} note={dict.optimalSoil}
          />
        </div>

        <div className="cards-grid">
          <SensorCard
            icon="⚖" label={dict.phLabel} value={d ? (+d.ph).toFixed(2) : '--'} unit="pH" color={pc}
            delta={d ? dict[d.phStatus] : dict.Neutral} barPct={d ? (d.ph / 14) * 100 : 0}
            status={d ? dict[d.phStatus] : dict.Neutral} note={dict.optimalPh}
          />
          <SensorCard
            icon="☀" label={dict.lightLabel} value={lightState} color={lc}
            delta={lightState} barPct={luxBarPct}
            status={lightSub} note={share != null ? dict.optimalLight : undefined}
          >
            <LightTimers reading={d} readingAt={readingAt} />
          </SensorCard>
          <SensorCard
            icon="🔋" label={dict.batteryLabel} value={d ? String(d.batPct) : '--'} unit="%" color={bc}
            delta={d ? dict[d.batStatus] : dict.Good} barPct={d?.batPct ?? 0}
            status={d ? (+d.batV).toFixed(2) + 'V' : '--V'} note={dict.batterySub}
          />
        </div>

        <div className="bottom-row">
          <SensorHistoryChart history={history} />
          <HealthPanel health={d ? d.health : null} color={hc2} metrics={metrics} />
        </div>

        <div className="ip-footer">
          ESP32-AGRO &nbsp;|&nbsp; Krishi Drishti &nbsp;|&nbsp; {window.location.protocol + '//' + window.location.host}
        </div>
      </div>
    </>
  )
}
