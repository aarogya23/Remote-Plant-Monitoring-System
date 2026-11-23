import { useLanguage } from '../i18n/LanguageContext.tsx'
import type { Lang } from '../lib/types.ts'

/** English / Nepali switcher; `compact` shows the short EN / ने labels used in the mobile bar. */
export default function LanguageSelect({ compact = false }: { compact?: boolean }) {
  const { lang, setLang } = useLanguage()
  return (
    <select
      className="lang-select"
      value={lang}
      onChange={(e) => setLang(e.target.value as Lang)}
      aria-label="Language"
    >
      <option value="en">{compact ? 'EN' : 'English'}</option>
      <option value="ne">{compact ? 'ने' : 'नेपाली'}</option>
    </select>
  )
}
