import { jsPDF } from 'jspdf'
import { TRANSLATIONS } from '../i18n/translations.ts'
import { getRowRecommendation, getStatus, lightText } from './sensors.ts'
import type { Lang, SensorReading } from './types.ts'

const DEVANAGARI_FONT_URL =
  'https://cdn.jsdelivr.net/gh/googlefonts/noto-fonts@master/hinted/ttf/NotoSansDevanagari/NotoSansDevanagari-Regular.ttf'

let cachedFontBase64: string | null = null

function statusText(r: SensorReading, lang: Lang): string {
  const s = getStatus(r)
  return TRANSLATIONS[lang][`status${s}` as const] || s
}

function slug(label: string): string {
  return label.replace(/\s+/g, '-').toLowerCase()
}

function download(blob: Blob, name: string) {
  const a = Object.assign(document.createElement('a'), { href: URL.createObjectURL(blob), download: name })
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(a.href)
}

function escapeHtml(s: string): string {
  return s.replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[c]!)
}

const fixed = (v: number, digits: number) => (+v).toFixed(digits)

export function exportCsv(rows: SensorReading[], lang: Lang, periodLabel: string) {
  const isNe = lang === 'ne'
  const lines = [
    isNe
      ? 'समय (Timestamp),तापक्रम °C,आर्द्रता %,माटोको ओस %,pH,प्रकाश,स्थिति,ब्याट्री %,सुझाव'
      : 'Timestamp,Temperature (°C),Humidity (%),Soil Moisture (%),pH,Light,Status,Battery (%),Recommendation',
  ]
  rows.forEach((r) => {
    const rec = getRowRecommendation(r, lang)
    lines.push([
      new Date(r.ts!).toLocaleString(), fixed(r.temp, 1), fixed(r.humidity, 0), fixed(r.soil, 0), fixed(r.ph, 2),
      lightText(r.lux, lang), isNe ? statusText(r, lang) : getStatus(r), fixed(r.batPct, 0),
      `"${rec.replace(/"/g, '""')}"`,
    ].join(','))
  })
  if (isNe) {
    // BOM so Excel opens the Devanagari text as UTF-8
    download(new Blob([new Uint8Array([0xef, 0xbb, 0xbf]), lines.join('\n')], { type: 'text/csv;charset=utf-8' }), `कृषि-रिपोर्ट-${periodLabel}.csv`)
  } else {
    download(new Blob([lines.join('\n')], { type: 'text/csv;charset=utf-8' }), `plant-report-${slug(periodLabel)}.csv`)
  }
}

export function exportJson(rows: SensorReading[], lang: Lang, periodLabel: string) {
  const isNe = lang === 'ne'
  const readings = rows.map((r) =>
    isNe
      ? {
          'समय': new Date(r.ts!).toLocaleString(), 'तापक्रम_°C': fixed(r.temp, 1), 'आर्द्रता_%': fixed(r.humidity, 0),
          'माटोको_ओस_%': fixed(r.soil, 0), 'pH': fixed(r.ph, 2), 'प्रकाश': lightText(r.lux, 'ne'),
          'स्थिति': statusText(r, 'ne'), 'ब्याट्री_%': fixed(r.batPct, 0), 'सुझाव': getRowRecommendation(r, 'ne'),
        }
      : {
          timestamp: new Date(r.ts!).toLocaleString(), temperature_C: fixed(r.temp, 1), humidity_pct: fixed(r.humidity, 0),
          soil_moisture_pct: fixed(r.soil, 0), pH: fixed(r.ph, 2), light: lightText(r.lux, 'en'), status: getStatus(r),
          battery_pct: fixed(r.batPct, 0), recommendation: getRowRecommendation(r, 'en'),
        },
  )
  const payload = isNe
    ? { 'उत्पादन_मिति': new Date().toLocaleString(), 'अवधि': periodLabel, 'भाषा': 'नेपाली', 'कुल_रेकर्ड': rows.length, 'रिडिङहरू': readings }
    : { generatedAt: new Date().toLocaleString(), period: periodLabel, language: 'English', totalRecords: rows.length, readings }
  download(
    new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' }),
    isNe ? `कृषि-रिपोर्ट-${periodLabel}.json` : `plant-report-${slug(periodLabel)}.json`,
  )
}

async function loadDevanagariFont(): Promise<string> {
  if (cachedFontBase64) return cachedFontBase64
  const blob = await (await fetch(DEVANAGARI_FONT_URL)).blob()
  cachedFontBase64 = await new Promise<string>((resolve, reject) => {
    const reader = new FileReader()
    reader.onloadend = () => resolve(String(reader.result).split(',')[1])
    reader.onerror = reject
    reader.readAsDataURL(blob)
  })
  return cachedFontBase64
}

/** `onFontLoading` lets the caller show a "Loading Font..." state while the Nepali font downloads. */
export async function exportPdf(
  rows: SensorReading[], lang: Lang, periodLabel: string, onFontLoading?: (loading: boolean) => void,
) {
  const doc = new jsPDF({ unit: 'pt', format: 'a4' })
  const isNe = lang === 'ne'
  const pageW = doc.internal.pageSize.getWidth()
  const pageH = doc.internal.pageSize.getHeight()
  const marginL = 40, marginR = 40
  const usableW = pageW - marginL - marginR

  let fontName = 'helvetica'
  if (isNe) {
    onFontLoading?.(true)
    try {
      doc.addFileToVFS('NotoSansDevanagari-Regular.ttf', await loadDevanagariFont())
      doc.addFont('NotoSansDevanagari-Regular.ttf', 'NotoSansDevanagari', 'normal')
      fontName = 'NotoSansDevanagari'
    } catch (err) {
      console.error('Failed to load Devanagari font:', err)
      alert('Error loading Devanagari font. Exporting with standard font.')
    } finally {
      onFontLoading?.(false)
    }
  }
  const ensureFont = () => doc.setFont(fontName, 'normal')
  ensureFont()

  let y = 40
  const titleText = isNe ? `कृषि दृष्टि — ${periodLabel} रिपोर्ट` : `Krishi Drishti — ${periodLabel} Report`
  doc.setFontSize(15); doc.setTextColor(45, 122, 58); doc.text(titleText, marginL, y); y += 20
  doc.setFontSize(9); doc.setTextColor(100, 100, 100)
  doc.text((isNe ? 'उत्पादन मिति: ' : 'Generated: ') + new Date().toLocaleString(), marginL, y); y += 13
  doc.text((isNe ? 'कुल रेकर्ड: ' : 'Total records: ') + rows.length, marginL, y); y += 13
  doc.text(isNe ? 'भाषा: नेपाली' : 'Language: English', marginL, y); y += 16
  doc.setDrawColor(200, 200, 200); doc.line(marginL, y, pageW - marginR, y); y += 12
  doc.setTextColor(30, 30, 30)

  const cols = isNe
    ? [{ label: 'समय', w: usableW * .18 }, { label: 'तापक्रम °C', w: usableW * .07 }, { label: 'आर्द्रता %', w: usableW * .07 }, { label: 'माटो %', w: usableW * .07 }, { label: 'pH', w: usableW * .06 }, { label: 'प्रकाश', w: usableW * .08 }, { label: 'स्थिति', w: usableW * .08 }, { label: 'ब्याट्री %', w: usableW * .07 }, { label: 'सुझाव', w: usableW * .32 }]
    : [{ label: 'Timestamp', w: usableW * .17 }, { label: 'Temp °C', w: usableW * .07 }, { label: 'Humidity %', w: usableW * .08 }, { label: 'Soil %', w: usableW * .07 }, { label: 'pH', w: usableW * .06 }, { label: 'Light', w: usableW * .07 }, { label: 'Status', w: usableW * .07 }, { label: 'Battery %', w: usableW * .07 }, { label: 'Recommendation', w: usableW * .34 }]

  const rowH = 16
  const drawHeader = () => {
    doc.setFillColor(232, 245, 233); doc.rect(marginL, y - rowH + 4, usableW, rowH, 'F')
    doc.setFontSize(7.5); doc.setTextColor(27, 94, 32)
    let hx = marginL
    cols.forEach((c) => { doc.text(c.label, hx + 2, y, { maxWidth: c.w - 4 }); hx += c.w })
    y += rowH; doc.setDrawColor(200, 220, 200)
    doc.line(marginL, y - rowH + 4, pageW - marginR, y - rowH + 4); doc.line(marginL, y, pageW - marginR, y)
    doc.setFontSize(7); doc.setTextColor(30, 30, 30)
  }
  drawHeader()

  rows.forEach((r, idx) => {
    const rec = getRowRecommendation(r, lang)
    const recLines = doc.splitTextToSize(rec, cols[8].w - 4).length
    const dynRowH = Math.max(rowH, recLines * 8 + 4)

    if (y + dynRowH > pageH - 40) {
      doc.addPage(); ensureFont(); y = 40
      drawHeader()
    }

    if (idx % 2 === 0) { doc.setFillColor(247, 249, 246); doc.rect(marginL, y, usableW, dynRowH, 'F') }
    const cellData = [
      new Date(r.ts!).toLocaleString(), fixed(r.temp, 1), fixed(r.humidity, 0), fixed(r.soil, 0), fixed(r.ph, 2),
      lightText(r.lux, lang), statusText(r, lang), fixed(r.batPct, 0), rec,
    ]
    let cx = marginL
    cellData.forEach((cell, ci) => {
      const cellLines: string[] = doc.splitTextToSize(String(cell), cols[ci].w - 4)
      cellLines.forEach((ln, li) => doc.text(ln, cx + 2, y + 9 + li * 8))
      cx += cols[ci].w
    })
    doc.setDrawColor(230, 235, 230); doc.line(marginL, y + dynRowH, pageW - marginR, y + dynRowH)
    y += dynRowH
  })

  const totalPages = doc.getNumberOfPages()
  for (let p = 1; p <= totalPages; p++) {
    doc.setPage(p); ensureFont(); doc.setFontSize(8); doc.setTextColor(150, 150, 150)
    doc.text(isNe ? 'कृषि दृष्टि | ESP32-AGRO' : 'Krishi Drishti | ESP32-AGRO', marginL, pageH - 20)
    doc.text(isNe ? `पृष्ठ ${p} / ${totalPages}` : `Page ${p} / ${totalPages}`, pageW - marginR, pageH - 20, { align: 'right' })
  }
  doc.save(isNe ? `कृषि-रिपोर्ट-${periodLabel}.pdf` : `plant-report-${slug(periodLabel)}.pdf`)
}

export function printReport(rows: SensorReading[], lang: Lang, periodLabel: string) {
  const isNe = lang === 'ne'
  const titleText = isNe ? `कृषि दृष्टि — ${periodLabel} रिपोर्ट` : `Krishi Drishti — ${periodLabel} Report`
  const tableHeaders = isNe
    ? '<tr><th>समय</th><th>तापक्रम °C</th><th>आर्द्रता %</th><th>माटो %</th><th>pH</th><th>प्रकाश</th><th>स्थिति</th><th>ब्याट्री %</th><th>सुझाव</th></tr>'
    : '<tr><th>Timestamp</th><th>Temp °C</th><th>Humidity %</th><th>Soil %</th><th>pH</th><th>Light</th><th>Status</th><th>Battery %</th><th>Recommendation</th></tr>'
  const tableRows = rows.map((r) => {
    const s = getStatus(r)
    const sc = s === 'Good' ? '#16a34a' : s === 'Alert' ? '#dc2626' : '#d97706'
    return `<tr><td>${new Date(r.ts!).toLocaleString()}</td><td>${fixed(r.temp, 1)}</td><td>${fixed(r.humidity, 0)}</td><td>${fixed(r.soil, 0)}</td><td>${fixed(r.ph, 2)}</td><td>${lightText(r.lux, lang)}</td><td style="color:${sc};font-weight:700">${statusText(r, lang)}</td><td>${fixed(r.batPct, 0)}</td><td>${escapeHtml(getRowRecommendation(r, lang))}</td></tr>`
  }).join('')

  const w = window.open('', '_blank', 'height=700,width=1100')
  if (!w) { alert(isNe ? 'पपअप ब्लक भयो' : 'Pop-up blocked — allow pop-ups to print'); return }
  w.document.write(`<!DOCTYPE html><html><head><meta charset="UTF-8"><title>${escapeHtml(titleText)}</title>
    <style>@import url('https://fonts.googleapis.com/css2?family=Noto+Sans+Devanagari&display=swap');body{font-family:'Noto Sans Devanagari',Arial,sans-serif;margin:24px;color:#1c2b1a}h1{color:#2d7a3a;font-size:18px;margin-bottom:6px}.meta{font-size:12px;color:#6b7280;margin-bottom:16px}table{border-collapse:collapse;width:100%;font-size:11px}th{background:#e8f5e9;color:#1b5e20;padding:8px 10px;border:1px solid #c8e6c9;text-align:left;white-space:nowrap}td{padding:7px 10px;border:1px solid #e5e7eb;vertical-align:top}tr:nth-child(even) td{background:#f7f9f6}@media print{body{margin:10px}h1{font-size:14px}table{font-size:9px}th,td{padding:4px 6px}}</style></head><body>
    <h1>${escapeHtml(titleText)}</h1><div class="meta">${isNe ? 'उत्पादन गरिएको मिति' : 'Generated'}: ${new Date().toLocaleString()} &nbsp;|&nbsp; ${isNe ? 'कुल रेकर्डहरू' : 'Records'}: ${rows.length}</div>
    <table>${tableHeaders}${tableRows}</table></body></html>`)
  w.document.close()
  w.focus()
  setTimeout(() => w.print(), 600)
}
