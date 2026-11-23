import { createContext, useCallback, useContext, useMemo, useState, type ReactNode } from 'react'
import type { Lang } from '../lib/types.ts'
import { TRANSLATIONS, type Dict } from './translations.ts'

interface LanguageValue {
  lang: Lang
  dict: Dict
  setLang: (lang: Lang) => void
}

const LanguageContext = createContext<LanguageValue | null>(null)

// Same localStorage key as the old static pages, so the saved choice carries over
function readSavedLang(): Lang {
  try {
    return localStorage.getItem('lang') === 'ne' ? 'ne' : 'en'
  } catch {
    return 'en'
  }
}

export function LanguageProvider({ children }: { children: ReactNode }) {
  const [lang, setLangState] = useState<Lang>(readSavedLang)

  const setLang = useCallback((next: Lang) => {
    setLangState(next)
    try {
      localStorage.setItem('lang', next)
    } catch {
      // Storage unavailable (private mode) — the choice just won't persist
    }
  }, [])

  const value = useMemo(() => ({ lang, dict: TRANSLATIONS[lang], setLang }), [lang, setLang])
  return <LanguageContext.Provider value={value}>{children}</LanguageContext.Provider>
}

// eslint-disable-next-line react-refresh/only-export-components
export function useLanguage(): LanguageValue {
  const ctx = useContext(LanguageContext)
  if (!ctx) throw new Error('useLanguage must be used inside <LanguageProvider>')
  return ctx
}
