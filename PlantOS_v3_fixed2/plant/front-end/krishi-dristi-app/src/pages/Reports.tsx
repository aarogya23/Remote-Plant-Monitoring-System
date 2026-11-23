import { useEffect, useState } from 'react'
import LanguageSelect from '../components/LanguageSelect.tsx'
import { useLanguage } from '../i18n/LanguageContext.tsx'
import { PERIOD_BUTTONS, PERIODS, PERIODS_TRANSLATED } from '../i18n/translations.ts'
import { fetchReadings, fetchStats } from '../lib/api.ts'
import { fmtHours, getRowRecommendation, getStatus, lightText, STATUS_COLORS } from '../lib/sensors.ts'
import type { Period, ReportStats, SensorReading } from '../lib/types.ts'

// Each result remembers the period it was fetched for, so a stale response is simply
// ignored and "loading" is just "no result for the current period yet".
type Result<T> = { period: Period; ok: true; value: T } | { period: Period; ok: false; error: string }

const PAGE_SIZES = [10, 20, 50, 100]

export default function Reports() {
  const { lang, dict } = useLanguage()
  const [period, setPeriod] = useState<Period>('today')
  const [stats, setStats] = useState<Result<ReportStats> | null>(null)
  const [readings, setReadings] = useState<Result<SensorReading[]> | null>(null)
  const [page, setPage] = useState(1)
  const [pageSize, setPageSize] = useState(10)
  const [pdfLoading, setPdfLoading] = useState(false)

  useEffect(() => {
    let cancelled = false
    fetchStats(period).then(
      (value) => { if (!cancelled) setStats({ period, ok: true, value }) },
      (e: Error) => { if (!cancelled) setStats({ period, ok: false, error: e.message }) },
    )
    fetchReadings(period).then(
      (value) => { if (!cancelled) setReadings({ period, ok: true, value }) },
      (e: Error) => { if (!cancelled) setReadings({ period, ok: false, error: e.message }) },
    )
    return () => { cancelled = true }
  }, [period])

  const periodLabel = PERIODS_TRANSLATED[lang][period]
  const s = stats?.period === period && stats.ok ? stats.value : null
  const statsFailed = stats?.period === period && !stats.ok
  const rowsResult = readings?.period === period ? readings : null
  const rows = rowsResult?.ok ? rowsResult.value : []

  const totalPages = Math.ceil(rows.length / pageSize) || 1
  const pageRows = rows.slice((page - 1) * pageSize, page * pageSize)

  let countText = dict.loading
  if (rowsResult && !rowsResult.ok) countText = dict.error
  else if (statsFailed) countText = dict.noDataShort
  else if (s) countText = (s.count || 0) + dict.records

  const choosePeriod = (p: Period) => { setPeriod(p); setPage(1) }

  // Exports re-fetch so they always contain the full, current data set
  async function runExport(fmt: 'csv' | 'json' | 'pdf' | 'print') {
    let data: SensorReading[]
    try {
      data = await fetchReadings(period)
    } catch (e) {
      alert((lang === 'en' ? 'Could not load data: ' : 'डाटा लोड हुन सकेन: ') + (e as Error).message)
      return
    }
    if (!data.length) {
      alert(fmt === 'print'
        ? (lang === 'en' ? 'No data to print' : 'प्रिन्ट गर्न कुनै डाटा छैन')
        : (lang === 'en' ? 'No data to export' : 'निर्यात गर्न कुनै डाटा छैन'))
      return
    }
    try {
      // Loaded on demand: jsPDF is large and only needed once someone exports
      const { exportCsv, exportJson, exportPdf, printReport } = await import('../lib/exports.ts')
      if (fmt === 'csv') exportCsv(data, lang, periodLabel)
      if (fmt === 'json') exportJson(data, lang, periodLabel)
      if (fmt === 'pdf') await exportPdf(data, lang, periodLabel, setPdfLoading)
      if (fmt === 'print') printReport(data, lang, periodLabel)
    } catch (e) {
      alert((lang === 'en' ? 'Export failed: ' : 'निर्यात असफल भयो: ') + (e as Error).message)
    }
  }

  const num = (v: number | undefined, digits: number) => (s && v !== undefined ? v.toFixed(digits) : '--')

  return (
    <>
      <div className="topbar">
        <div className="topbar-title">
          <h2>{dict.reportsTitle}</h2>
          <p>{dict.reportsSub}</p>
        </div>
        <div className="topbar-right">
          <LanguageSelect />
        </div>
      </div>

      <div className="content">
        <div className="section-header">
          <div className="section-title">
            <h3>{dict.periodSelection}</h3>
            <p>{dict.choosePeriod}</p>
          </div>
          <div className="period-btns-wrap">
            <div className="period-btns">
              {PERIODS.map((p) => (
                <button key={p} type="button" className={`period-btn${p === period ? ' active' : ''}`} onClick={() => choosePeriod(p)}>
                  {PERIOD_BUTTONS[lang][p]}
                </button>
              ))}
            </div>
          </div>
        </div>

        <div className="export-row">
          <button type="button" className="btn btn-success" onClick={() => runExport('csv')}>📥 CSV</button>
          <button type="button" className="btn btn-primary" onClick={() => runExport('json')}>📥 JSON</button>
          <button type="button" className="btn btn-purple" onClick={() => runExport('pdf')} disabled={pdfLoading}>
            {pdfLoading ? `⏳ ${dict.loadingFont}` : '📄 PDF'}
          </button>
          <button type="button" className="btn btn-warning" onClick={() => runExport('print')}>🖨 {dict.print}</button>
        </div>

        <div className="period-info">
          <span>📊 <span>{dict.showing}</span> <strong>{periodLabel}</strong></span>
          <span className="record-count">{countText}</span>
        </div>

        <div className="cards-grid">
          <div className="card">
            <div className="card-label">{dict.avgTemp}</div>
            <div><span className="card-value stat-temp">{num(s?.averageTemp, 1)}</span><span className="card-unit">°C</span></div>
          </div>
          <div className="card">
            <div className="card-label">{dict.avgHum}</div>
            <div><span className="card-value stat-hum">{num(s?.averageHumidity, 0)}</span><span className="card-unit">%</span></div>
          </div>
          <div className="card">
            <div className="card-label">{dict.avgSoil}</div>
            <div><span className="card-value stat-soil">{num(s?.averageSoil, 0)}</span><span className="card-unit">%</span></div>
          </div>
          <div className="card">
            <div className="card-label">{dict.avgPh}</div>
            <div><span className="card-value stat-ph">{num(s?.averagePh, 2)}</span><span className="card-unit">pH</span></div>
          </div>
          <div className="card">
            <div className="card-label">{dict.avgLight}</div>
            <div><span className="card-value stat-lux">{num(s?.lightSharePct, 0)}</span><span className="card-unit">%</span></div>
            {s && (s.lightHours || s.darkHours) ? (
              <div className="light-hours">☀ {fmtHours(s.lightHours)}&nbsp;&nbsp;☾ {fmtHours(s.darkHours)}</div>
            ) : null}
          </div>
          <div className="card">
            <div className="card-label">{dict.avgBat}</div>
            <div><span className="card-value stat-bat">{num(s?.averageBattery, 0)}</span><span className="card-unit">%</span></div>
          </div>
        </div>

        <div className="chart-card">
          <div className="chart-title">{dict.histTitle}</div>
          <div className="chart-sub">{dict.histSub}</div>
          <div className="scroll-hint">{dict.scrollHint}</div>
          <div className="table-wrap">
            <table className="report-table">
              <thead>
                <tr>
                  <th>{dict.thTime}</th><th>{dict.thTemp}</th><th>{dict.thHum}</th>
                  <th>{dict.thSoil}</th><th>{dict.thPh}</th><th>{dict.thLight}</th>
                  <th>{dict.thBat}</th><th>{dict.thStatus}</th><th>{dict.thRec}</th>
                </tr>
              </thead>
              <tbody>
                {rowsResult && !rowsResult.ok ? (
                  <tr><td colSpan={9} className="table-message table-error">{dict.backendDown} — {rowsResult.error}</td></tr>
                ) : !rows.length ? (
                  <tr><td colSpan={9} className="table-message">{rowsResult ? dict.noData : dict.loading}</td></tr>
                ) : (
                  pageRows.map((row) => {
                    const st = getStatus(row)
                    const rec = getRowRecommendation(row, lang)
                    return (
                      <tr key={row.id ?? row.ts}>
                        <td>{new Date(row.ts!).toLocaleString()}</td>
                        <td>{(+row.temp).toFixed(1)}</td><td>{(+row.humidity).toFixed(0)}</td>
                        <td>{(+row.soil).toFixed(0)}</td><td>{(+row.ph).toFixed(2)}</td>
                        <td>{lightText(row.lux, lang)}</td><td>{(+row.batPct).toFixed(0)}</td>
                        <td style={{ color: STATUS_COLORS[st], fontWeight: 700 }}>{dict[`status${st}`]}</td>
                        <td className="rec-cell" title={rec}>{rec}</td>
                      </tr>
                    )
                  })
                )}
              </tbody>
            </table>
          </div>
        </div>

        <div className="pagination">
          <button type="button" className="btn btn-secondary" onClick={() => setPage((p) => Math.max(1, p - 1))}>{dict.btnPrev}</button>
          <span className="page-info">
            {rows.length ? (lang === 'en' ? `Page ${page} of ${totalPages}` : `पृष्ठ ${page} को ${totalPages}`) : (lang === 'en' ? 'Page --' : 'पृष्ठ --')}
          </span>
          <button type="button" className="btn btn-secondary" onClick={() => setPage((p) => Math.min(totalPages, p + 1))}>{dict.btnNext}</button>
          <select
            className="page-size-select"
            value={pageSize}
            onChange={(e) => { setPageSize(parseInt(e.target.value)); setPage(1) }}
          >
            {PAGE_SIZES.map((n) => <option key={n} value={n}>{n}{dict.perPage}</option>)}
          </select>
        </div>
      </div>

      <div className="ip-footer">ESP32-AGRO &nbsp;|&nbsp; Krishi Drishti &nbsp;|&nbsp; http://192.168.4.1</div>
    </>
  )
}
