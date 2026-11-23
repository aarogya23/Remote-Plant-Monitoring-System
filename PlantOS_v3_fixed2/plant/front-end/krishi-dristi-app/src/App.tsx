import { Navigate, Route, Routes } from 'react-router-dom'
import Layout from './components/Layout.tsx'
import { LanguageProvider } from './i18n/LanguageContext.tsx'
import Dashboard from './pages/Dashboard.tsx'
import Reports from './pages/Reports.tsx'

function App() {
  return (
    <LanguageProvider>
      <Routes>
        <Route element={<Layout />}>
          <Route index element={<Dashboard />} />
          <Route path="reports" element={<Reports />} />
          {/* Old static-page URLs keep working */}
          <Route path="index.html" element={<Navigate to="/" replace />} />
          <Route path="reports.html" element={<Navigate to="/reports" replace />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Route>
      </Routes>
    </LanguageProvider>
  )
}

export default App
