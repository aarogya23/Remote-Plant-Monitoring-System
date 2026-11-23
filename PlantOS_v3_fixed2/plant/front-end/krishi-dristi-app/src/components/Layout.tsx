import { useCallback, useEffect, useState } from 'react'
import { NavLink, Outlet, useLocation, useOutletContext } from 'react-router-dom'
import logo from '../assets/logo-transparent.png'
import { useSensorFeed } from '../hooks/useSensorFeed.ts'
import { useLanguage } from '../i18n/LanguageContext.tsx'
import LanguageSelect from './LanguageSelect.tsx'

export type FeedContext = ReturnType<typeof useSensorFeed>

// eslint-disable-next-line react-refresh/only-export-components
export function useFeed(): FeedContext {
  return useOutletContext<FeedContext>()
}

export default function Layout() {
  const { dict } = useLanguage()
  const { pathname } = useLocation()
  const feed = useSensorFeed()
  const [navOpen, setNavOpen] = useState(false)
  const closeNav = useCallback(() => setNavOpen(false), [])

  const isReports = pathname.startsWith('/reports')

  useEffect(() => {
    const onResize = () => { if (window.innerWidth > 960) setNavOpen(false) }
    window.addEventListener('resize', onResize)
    return () => window.removeEventListener('resize', onResize)
  }, [])

  const online = feed.conn === 'online'

  return (
    <div className={`app-shell ${isReports ? 'theme-reports' : 'theme-dashboard'}${navOpen ? ' nav-open' : ''}`}>
      <aside className="sidebar">
        <div className="logo">
          <img className="brand-logo" src={logo} alt="Krishi Drishti logo" />
        </div>
        <div className="nav-section">
          <div className="nav-label">{dict.mainLabel}</div>
          <NavLink className="nav-item" to="/" end onClick={closeNav}>
            <span className="nav-icon">☷</span> <span>{dict.dashboardNav}</span>
          </NavLink>
          <NavLink className="nav-item" to="/reports" onClick={closeNav}>
            <span className="nav-icon">📈</span> <span>{dict.reportsNav}</span>
          </NavLink>
          <a className="nav-item" href="#" onClick={(e) => { e.preventDefault(); closeNav() }}>
            <span className="nav-icon">🔔</span> <span>{dict.alertsNav}</span>
          </a>
        </div>
        <div className="nav-section">
          <div className="nav-label">{dict.settingsLabel}</div>
          <a className="nav-item" href="#" onClick={(e) => { e.preventDefault(); closeNav() }}>
            <span className="nav-icon">⚙</span> <span>{dict.configNav}</span>
          </a>
          <a className="nav-item" href="#" onClick={(e) => { e.preventDefault(); closeNav() }}>
            <span className="nav-icon">🔌</span> <span>{dict.deviceNav}</span>
          </a>
        </div>
        <div className="sidebar-footer">
          <div className="device-chip">
            <div className="device-dot" style={online ? undefined : { background: '#f59e0b' }} />
            <div>
              <div className="device-name">ESP32-AGRO</div>
              <div className="device-status" style={{ color: online ? undefined : '#f59e0b' }}>
                • {online ? dict.online : dict.reconnect}
              </div>
            </div>
          </div>
        </div>
      </aside>

      <div className="nav-overlay" onClick={closeNav} />

      <div className="main">
        <div className="mobile-nav">
          <div className="mobile-brand">
            <img className="brand-logo" src={logo} alt="Krishi Drishti logo" />
          </div>
          <div className="mobile-nav-right">
            {isReports && <LanguageSelect compact />}
            <button className="hamburger-btn" type="button" onClick={() => setNavOpen((o) => !o)} aria-label="Toggle navigation">
              <span>☰</span>
            </button>
          </div>
        </div>
        <Outlet context={feed} />
      </div>
    </div>
  )
}
