package com.monitoring.plant.service;

import com.monitoring.plant.model.SensorReading;
import com.monitoring.plant.repo.SensorReadingRepository;
import org.springframework.stereotype.Service;
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.List;
import java.util.HashMap;
import java.util.Map;

@Service
public class ReportService {

  private final SensorReadingRepository repository;

  public ReportService(SensorReadingRepository repository) {
    this.repository = repository;
  }

  /**
   * Get sensor readings for a specific period
   */
  public List<SensorReading> getReadingsByPeriod(String period) {
    Instant startTime = calculateStartTime(period);
    if (startTime == null) {
      return repository.findAll();
    }
    return repository.findByTsGreaterThanEqualOrderByTsDesc(startTime);
  }

  /**
   * Get sensor readings between two dates
   */
  public List<SensorReading> getReadingsBetween(Instant startTime, Instant endTime) {
    return repository.findByTsBetweenOrderByTsDesc(startTime, endTime);
  }

  /**
   * Calculate statistics for readings
   */
  public Map<String, Object> calculateStatistics(List<SensorReading> readings) {
    Map<String, Object> stats = new HashMap<>();
    
    if (readings == null || readings.isEmpty()) {
      return getEmptyStats();
    }

    double avgTemp = readings.stream().mapToDouble(SensorReading::getTemp).average().orElse(0);
    double avgHumidity = readings.stream().mapToDouble(SensorReading::getHumidity).average().orElse(0);
    double avgSoil = readings.stream().mapToDouble(SensorReading::getSoil).average().orElse(0);
    double avgPh = readings.stream().mapToDouble(SensorReading::getPh).average().orElse(0);
    double avgLux = readings.stream().mapToDouble(SensorReading::getLux).average().orElse(0);
    double avgBattery = readings.stream().mapToDouble(SensorReading::getBatPct).average().orElse(0);
    double avgHealth = readings.stream().mapToDouble(SensorReading::getHealth).average().orElse(0);

    double minTemp = readings.stream().mapToDouble(SensorReading::getTemp).min().orElse(0);
    double maxTemp = readings.stream().mapToDouble(SensorReading::getTemp).max().orElse(0);
    double minHumidity = readings.stream().mapToDouble(SensorReading::getHumidity).min().orElse(0);
    double maxHumidity = readings.stream().mapToDouble(SensorReading::getHumidity).max().orElse(0);
    double minSoil = readings.stream().mapToDouble(SensorReading::getSoil).min().orElse(0);
    double maxSoil = readings.stream().mapToDouble(SensorReading::getSoil).max().orElse(0);

    stats.put("count", readings.size());
    stats.put("averageTemp", round(avgTemp, 2));
    stats.put("averageHumidity", round(avgHumidity, 1));
    stats.put("averageSoil", round(avgSoil, 1));
    stats.put("averagePh", round(avgPh, 2));
    stats.put("averageLux", round(avgLux, 0));
    stats.put("averageBattery", round(avgBattery, 1));
    stats.put("averageHealth", round(avgHealth, 1));
    
    stats.put("minTemp", round(minTemp, 2));
    stats.put("maxTemp", round(maxTemp, 2));
    stats.put("minHumidity", round(minHumidity, 1));
    stats.put("maxHumidity", round(maxHumidity, 1));
    stats.put("minSoil", round(minSoil, 1));
    stats.put("maxSoil", round(maxSoil, 1));

    return stats;
  }

  /**
   * Get statistics for a specific period
   */
  public Map<String, Object> getStatisticsByPeriod(String period) {
    List<SensorReading> readings = getReadingsByPeriod(period);
    return calculateStatistics(readings);
  }

  /**
   * Get statistics between two dates
   */
  public Map<String, Object> getStatisticsBetween(Instant startTime, Instant endTime) {
    List<SensorReading> readings = getReadingsBetween(startTime, endTime);
    return calculateStatistics(readings);
  }

  /**
   * Generate a detailed report for a period
   */
  public Map<String, Object> generateReport(String period) {
    List<SensorReading> readings = getReadingsByPeriod(period);
    Map<String, Object> stats = calculateStatistics(readings);
    
    Map<String, Object> report = new HashMap<>();
    report.put("period", period);
    report.put("generatedAt", Instant.now());
    report.put("statistics", stats);
    report.put("readings", readings);
    
    return report;
  }

  /**
   * Get data summary for dashboard
   */
  public Map<String, Object> getDashboardSummary() {
    List<SensorReading> todayReadings = getReadingsByPeriod("today");
    Map<String, Object> summary = new HashMap<>();
    
    if (!todayReadings.isEmpty()) {
      SensorReading latest = todayReadings.get(0);
      summary.put("lastReading", latest);
      summary.put("todayStats", calculateStatistics(todayReadings));
    }
    
    return summary;
  }

  /**
   * Calculate health score based on current readings
   */
  public int calculateHealthScore(SensorReading reading) {
    int score = 100;
    
    // Temperature check (15-35°C optimal)
    if (reading.getTemp() < 15 || reading.getTemp() > 35) score -= 20;
    else if (reading.getTemp() < 18 || reading.getTemp() > 32) score -= 10;
    
    // Humidity check (40-70% optimal)
    if (reading.getHumidity() < 30 || reading.getHumidity() > 80) score -= 20;
    else if (reading.getHumidity() < 40 || reading.getHumidity() > 70) score -= 10;
    
    // Soil moisture check (35-65% optimal)
    if (reading.getSoil() < 30 || reading.getSoil() > 70) score -= 20;
    else if (reading.getSoil() < 35 || reading.getSoil() > 65) score -= 10;
    
    // pH check (5.5-7.5 optimal)
    if (reading.getPh() < 5.0 || reading.getPh() > 8.0) score -= 20;
    else if (reading.getPh() < 5.5 || reading.getPh() > 7.5) score -= 10;
    
    // Battery check
    if (reading.getBatPct() < 20) score -= 15;
    else if (reading.getBatPct() < 50) score -= 5;
    
    return Math.max(0, Math.min(100, score));
  }

  // Helper methods
  private Instant calculateStartTime(String period) {
    LocalDateTime now = LocalDateTime.now();
    LocalDateTime startDateTime = null;
    
    switch (period.toLowerCase()) {
      case "today":
        startDateTime = now.withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "last3days":
        startDateTime = now.minusDays(3).withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "week":
      case "weekly":
        startDateTime = now.minusDays(7).withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "twoweeks":
        startDateTime = now.minusDays(14).withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "month":
      case "monthly":
        startDateTime = now.minusDays(30).withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "threemonths":
        startDateTime = now.minusDays(90).withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "sixmonths":
        startDateTime = now.minusDays(180).withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "year":
      case "yearly":
        startDateTime = now.minusDays(365).withHour(0).withMinute(0).withSecond(0).withNano(0);
        break;
      case "all":
        return null; // Return all data
      default:
        return null;
    }
    
    return startDateTime != null ? startDateTime.atZone(ZoneId.systemDefault()).toInstant() : null;
  }

  private double round(double value, int places) {
    if (places < 0) throw new IllegalArgumentException();
    long factor = (long) Math.pow(10, places);
    return (double) Math.round(value * factor) / factor;
  }

  private Map<String, Object> getEmptyStats() {
    Map<String, Object> stats = new HashMap<>();
    stats.put("count", 0);
    stats.put("averageTemp", 0);
    stats.put("averageHumidity", 0);
    stats.put("averageSoil", 0);
    stats.put("averagePh", 0);
    stats.put("averageLux", 0);
    stats.put("averageBattery", 0);
    stats.put("averageHealth", 0);
    stats.put("minTemp", 0);
    stats.put("maxTemp", 0);
    stats.put("minHumidity", 0);
    stats.put("maxHumidity", 0);
    stats.put("minSoil", 0);
    stats.put("maxSoil", 0);
    return stats;
  }
}
