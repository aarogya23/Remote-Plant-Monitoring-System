import type { DashboardData, Lang, SensorReading } from './types.ts'

export const COLORS = {
  green: '#22c55e',
  red: '#ef4444',
  amber: '#f59e0b',
  blue: '#3b82f6',
  purple: '#8b5cf6',
}

type ColorKind = 'temp' | 'hum' | 'soil' | 'ph' | 'lux' | 'bat' | 'health'

export function colorFor(type: ColorKind, val: number): string {
  switch (type) {
    case 'temp':   return val < 15 || val > 35 ? COLORS.red : COLORS.green
    case 'hum':    return val < 30 || val > 80 ? COLORS.amber : COLORS.blue
    case 'soil':   return val < 30 ? COLORS.red : val < 40 ? COLORS.amber : COLORS.green
    case 'ph':     return val < 5.0 || val > 8.0 ? COLORS.red : val < 5.5 || val > 7.5 ? COLORS.amber : COLORS.green
    case 'lux':    return val === 1 ? COLORS.green : COLORS.red
    case 'bat':    return val < 30 ? COLORS.red : val < 70 ? COLORS.amber : COLORS.green
    case 'health': return val >= 75 ? COLORS.green : val >= 50 ? COLORS.amber : COLORS.red
  }
}

/** Fill in defaults and derive the status labels the dashboard shows. */
export function toDashboardData(d: Partial<SensorReading>): DashboardData {
  // One decimal, like the device sends (the /api/mock values are unrounded)
  const round1 = (v: number | undefined) => Math.round((v || 0) * 10) / 10
  const temp = round1(d.temp), soil = d.soil || 0, ph = d.ph || 0, batPct = d.batPct || 0
  const lux = d.lux !== undefined ? d.lux : 0
  return {
    ...d,
    temp, humidity: round1(d.humidity), soil, ph, lux,
    batPct, batV: d.batV || 0, health: d.health || 0,
    tempStatus: temp < 15 || temp > 35 ? 'Alert' : 'Normal',
    soilStatus: soil < 30 ? 'Dry' : soil < 40 ? 'Moist' : 'Wet',
    phStatus: ph < 5.0 || ph > 8.0 ? 'Critical' : ph < 5.5 || ph > 7.5 ? 'Warning' : 'Neutral',
    batStatus: batPct < 30 ? 'Low' : batPct < 70 ? 'Medium' : 'Good',
    lightStatus: lux === 1 ? 'Light' : 'Dark',
  }
}

/** Same format as the device LCD: HH:MM:SS, or "123h 45m" past 99 hours. */
export function fmtClock(sec: number): string {
  sec = Math.max(0, Math.floor(sec))
  const h = Math.floor(sec / 3600), m = Math.floor((sec % 3600) / 60), s = sec % 60
  if (h >= 100) return h + 'h ' + String(m).padStart(2, '0') + 'm'
  return String(h).padStart(2, '0') + ':' + String(m).padStart(2, '0') + ':' + String(s).padStart(2, '0')
}

/** 3.5 h -> "3h 30m" */
export function fmtHours(h: number | undefined): string {
  const mins = Math.round((h || 0) * 60)
  return Math.floor(mins / 60) + 'h ' + String(mins % 60).padStart(2, '0') + 'm'
}

export function fmtTime(d: Date): string {
  return [d.getHours(), d.getMinutes(), d.getSeconds()].map((n) => String(n).padStart(2, '0')).join(':')
}

/** Share of time in light since power-on (0-100), or null when the device sent no timer data. */
export function lightShareOf(d: SensorReading): number | null {
  if (d.lightPct != null) return +d.lightPct
  const t = (+(d.lightSec ?? 0)) + (+(d.darkSec ?? 0))
  return t > 0 ? Math.round(((d.lightSec ?? 0) * 100) / t) : null
}

export interface Recommendation {
  sensor: string
  val: string
  status: string
  color: string
  action: string
}

export function getRecommendations(d: DashboardData, lang: Lang): Recommendation[] {
  const isEn = lang === 'en'
  const recs: Recommendation[] = []

  // Soil Moisture
  const soilName = isEn ? 'Soil Moisture' : 'माटोको ओस'
  if (d.soil < 30) {
    recs.push({ sensor: soilName, val: d.soil + '%', status: isEn ? 'Low' : 'कम', color: COLORS.red,
      action: isEn ? 'Water the plant immediately. Soil is too dry.' : 'बोटबिरुवामा तुरुन्तै पानी हाल्नुहोस्। माटो धेरै सुक्खा छ।' })
  } else if (d.soil <= 60) {
    recs.push({ sensor: soilName, val: d.soil + '%', status: isEn ? 'Optimal' : 'राम्रो', color: COLORS.green,
      action: isEn ? 'Soil moisture is ideal. No watering needed.' : 'माटोको ओस उपयुक्त छ। पानी हाल्न आवश्यक छैन।' })
  } else {
    recs.push({ sensor: soilName, val: d.soil + '%', status: isEn ? 'High' : 'धेरै', color: COLORS.amber,
      action: isEn ? 'Soil is too wet. Avoid watering to prevent root rot.' : 'माटो धेरै चिसो छ। जरा कुहिनबाट जोगाउन पानी नहाल्नुहोस्।' })
  }

  // Temperature
  const tempName = isEn ? 'Temperature' : 'तापक्रम'
  if (d.temp < 15) {
    recs.push({ sensor: tempName, val: d.temp + '°C', status: isEn ? 'Cold' : 'चिसो', color: COLORS.red,
      action: isEn ? 'Temperature is too low. Move plant to a warmer environment.' : 'तापक्रम धेरै कम छ। बोटलाई न्यानो वातावरणमा सार्नुहोस्।' })
  } else if (d.temp <= 35) {
    recs.push({ sensor: tempName, val: d.temp + '°C', status: isEn ? 'Optimal' : 'राम्रो', color: COLORS.green,
      action: isEn ? 'Temperature is in the optimal range (15-35°C).' : 'तापक्रम उपयुक्त दायरामा छ (१५-३५°C)।' })
  } else {
    recs.push({ sensor: tempName, val: d.temp + '°C', status: isEn ? 'Hot' : 'तातो', color: COLORS.red,
      action: isEn ? 'Temperature is too high. Provide shade and ensure proper ventilation.' : 'तापक्रम धेरै उच्च छ। छहारी दिनुहोस् र हावा आवतजावत मिलाउनुहोस्।' })
  }

  // pH
  const phName = isEn ? 'Soil pH' : 'माटोको pH'
  const phVal = (+d.ph).toFixed(2)
  if (d.ph < 5.5) {
    recs.push({ sensor: phName, val: phVal, status: isEn ? 'Acidic' : 'अम्लीय', color: COLORS.red,
      action: isEn ? 'Soil is acidic. Apply agricultural lime (calcium carbonate) to increase pH.' : 'माटो अम्लीय छ। pH बढाउन कृषि चुन (क्याल्सियम कार्बोनेट) प्रयोग गर्नुहोस्।' })
  } else if (d.ph <= 7.5) {
    recs.push({ sensor: phName, val: phVal, status: isEn ? 'Optimal' : 'राम्रो', color: COLORS.green,
      action: isEn ? 'pH is optimal (5.5-7.5) for most crop varieties.' : 'pH स्तर अधिकांश बालीका लागि उपयुक्त छ (५.५-७.५)।' })
  } else {
    recs.push({ sensor: phName, val: phVal, status: isEn ? 'Alkaline' : 'क्षारीय', color: COLORS.red,
      action: isEn ? 'Soil is alkaline. Add organic compost or elemental sulfur to lower pH.' : 'माटो क्षारीय छ। pH घटाउन प्राङ्गारिक मल वा सल्फर थप्नुहोस्।' })
  }

  // Humidity
  const humName = isEn ? 'Humidity' : 'आर्द्रता'
  if (d.humidity < 40) {
    recs.push({ sensor: humName, val: d.humidity + '%', status: isEn ? 'Low' : 'कम', color: COLORS.amber,
      action: isEn ? 'Air is dry. Mist plant leaves occasionally or use a humidifier.' : 'हावा सुक्खा छ। पातहरूमा पानीको फोहोरा दिनुहोस् वा ह्युमिडिफायर प्रयोग गर्नुहोस्।' })
  } else if (d.humidity <= 70) {
    recs.push({ sensor: humName, val: d.humidity + '%', status: isEn ? 'Optimal' : 'राम्रो', color: COLORS.green,
      action: isEn ? 'Humidity is ideal (40-70%).' : 'आर्द्रता उपयुक्त छ (४०-७०%)।' })
  } else {
    recs.push({ sensor: humName, val: d.humidity + '%', status: isEn ? 'High' : 'बढी', color: COLORS.amber,
      action: isEn ? 'Humidity is high. Ensure good spacing and air flow to prevent fungus.' : 'आर्द्रता धेरै छ। ढुसी लाग्न नदिन बिरुवाहरूबीच पर्याप्त दूरी र हावाको बहाव मिलाउनुहोस्।' })
  }

  // Light
  const lightName = isEn ? 'Light' : 'प्रकाश'
  const share = lightShareOf(d)
  const shareNote = share != null && d.uptimeSec
    ? (isEn ? ' Light ' + share + '% of the last ' + fmtClock(d.uptimeSec) + '.'
            : ' पछिल्लो ' + fmtClock(d.uptimeSec) + ' मा ' + share + '% समय उज्यालो।')
    : ''
  if (d.lux === 0) {
    recs.push({ sensor: lightName, val: isEn ? 'Dark' : 'अध्यारो', status: isEn ? 'Low Light' : 'कम प्रकाश', color: COLORS.red,
      action: (isEn ? 'Too dark. Ensure the plant gets direct/indirect sunlight, or use grow lights.' : 'धेरै अध्यारो भयो। बिरुवालाई पर्याप्त सूर्यको प्रकाशमा राख्नुहोस् वा ग्रो लाइट प्रयोग गर्नुहोस्।') + shareNote })
  } else {
    recs.push({ sensor: lightName, val: isEn ? 'Light' : 'उज्यालो', status: isEn ? 'Adequate' : 'पर्याप्त', color: COLORS.green,
      action: (isEn ? 'Light conditions are sufficient for photosynthesis.' : 'प्रकाशको अवस्था प्रकाश संश्लेषणका लागि पर्याप्त छ।') + shareNote })
  }

  // Battery
  if (d.batPct < 20) {
    recs.push({ sensor: isEn ? 'Battery' : 'ब्याट्री', val: d.batPct + '%', status: isEn ? 'Critical' : 'कमजोर', color: COLORS.red,
      action: isEn ? 'Battery is critically low. Plug in charger immediately to keep device running.' : 'ब्याट्री एकदम कम छ। उपकरण बन्द हुन नदिन तुरुन्तै चार्जमा राख्नुहोस्।' })
  }

  return recs
}

// ── Report-table helpers ────────────────────────────────────────

export type RowStatus = 'Good' | 'Alert' | 'Warn' | 'Dry'

export const STATUS_COLORS: Record<RowStatus, string> = {
  Good: COLORS.green, Alert: COLORS.red, Warn: COLORS.amber, Dry: COLORS.amber,
}

export function getStatus(row: SensorReading): RowStatus {
  if (row.soil < 30) return 'Dry'
  if (row.temp > 35 || row.temp < 15) return 'Alert'
  if (row.ph > 8 || row.ph < 5) return 'Warn'
  return 'Good'
}

export function getRowRecommendation(r: SensorReading, lang: Lang): string {
  const isEn = lang === 'en'
  const recs: string[] = []
  if (r.soil < 30)          recs.push(isEn ? 'Water plant now' : 'तुरुन्तै पानी हाल्नुहोस्')
  else if (r.soil > 60)     recs.push(isEn ? 'Avoid watering (soil wet)' : 'पानी नहाल्नुहोस् (माटो चिसो छ)')
  if (r.temp < 15)          recs.push(isEn ? 'Move to warmer spot' : 'न्यानो ठाउँमा सार्नुहोस्')
  else if (r.temp > 35)     recs.push(isEn ? 'Provide shade/cooling' : 'छहारी वा चिसो व्यवस्था गर्नुहोस्')
  if (r.ph < 5.5)           recs.push(isEn ? 'Add agricultural lime' : 'कृषि चुन थप्नुहोस्')
  else if (r.ph > 7.5)      recs.push(isEn ? 'Add organic compost' : 'प्राङ्गारिक मल थप्नुहोस्')
  if (r.humidity < 40)      recs.push(isEn ? 'Mist leaves' : 'पातमा पानी छर्कनुहोस्')
  else if (r.humidity > 70) recs.push(isEn ? 'Improve ventilation' : 'हावा आवतजावत बढाउनुहोस्')
  if (r.lux === 0)          recs.push(isEn ? 'Increase light' : 'उज्यालो बढाउनुहोस्')
  if (r.batPct < 20)        recs.push(isEn ? 'Charge battery' : 'चार्ज गर्नुहोस्')
  return recs.length === 0 ? (isEn ? 'Conditions optimal' : 'अवस्था सामान्य छ') : recs.join('; ')
}

export function lightText(lux: number, lang: Lang): string {
  return lux === 1 ? (lang === 'en' ? 'Light' : 'उज्यालो') : (lang === 'en' ? 'Dark' : 'अध्यारो')
}
