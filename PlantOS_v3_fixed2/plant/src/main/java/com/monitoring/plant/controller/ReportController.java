package com.monitoring.plant.controller;

import com.monitoring.plant.service.ReportService;
import com.monitoring.plant.model.SensorReading;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Map;

@RestController
@CrossOrigin(origins = "*")
@RequestMapping("/api/report")
public class ReportController {

  private final ReportService reportService;

  public ReportController(ReportService reportService) {
    this.reportService = reportService;
  }

  /**
   * Get sensor readings for a specific period (today, week, month, all)
   * @param period the period to retrieve data for
   * @return list of sensor readings
   */
  @GetMapping("/readings")
  public ResponseEntity<List<SensorReading>> getReadingsByPeriod(
      @RequestParam(value = "period", defaultValue = "all") String period) {
    List<SensorReading> readings = reportService.getReadingsByPeriod(period);
    return ResponseEntity.ok(readings);
  }

  /**
   * Get sensor readings between two timestamps
   * @param start start timestamp (milliseconds)
   * @param end end timestamp (milliseconds)
   * @return list of sensor readings
   */
  @GetMapping("/readings/range")
  public ResponseEntity<List<SensorReading>> getReadingsByRange(
      @RequestParam("start") long start,
      @RequestParam("end") long end) {
    Instant startTime = Instant.ofEpochMilli(start);
    Instant endTime = Instant.ofEpochMilli(end);
    List<SensorReading> readings = reportService.getReadingsBetween(startTime, endTime);
    return ResponseEntity.ok(readings);
  }

  /**
   * Get statistical summary for a specific period
   * @param period the period to calculate statistics for
   * @return map containing statistical data
   */
  @GetMapping("/statistics")
  public ResponseEntity<Map<String, Object>> getStatistics(
      @RequestParam(value = "period", defaultValue = "all") String period) {
    Map<String, Object> stats = reportService.getStatisticsByPeriod(period);
    return ResponseEntity.ok(stats);
  }

  /**
   * Get statistical summary between two dates
   * @param start start timestamp (milliseconds)
   * @param end end timestamp (milliseconds)
   * @return map containing statistical data
   */
  @GetMapping("/statistics/range")
  public ResponseEntity<Map<String, Object>> getStatisticsRange(
      @RequestParam("start") long start,
      @RequestParam("end") long end) {
    Instant startTime = Instant.ofEpochMilli(start);
    Instant endTime = Instant.ofEpochMilli(end);
    Map<String, Object> stats = reportService.getStatisticsBetween(startTime, endTime);
    return ResponseEntity.ok(stats);
  }

  /**
   * Generate a complete report for a specific period
   * @param period the period to generate report for
   * @return complete report with readings and statistics
   */
  @GetMapping("/generate")
  public ResponseEntity<Map<String, Object>> generateReport(
      @RequestParam(value = "period", defaultValue = "all") String period) {
    Map<String, Object> report = reportService.generateReport(period);
    return ResponseEntity.ok(report);
  }

  /**
   * Get dashboard summary with latest reading and today's statistics
   * @return dashboard summary
   */
  @GetMapping("/summary")
  public ResponseEntity<Map<String, Object>> getDashboardSummary() {
    Map<String, Object> summary = reportService.getDashboardSummary();
    return ResponseEntity.ok(summary);
  }

  /**
   * Export report data as JSON
   * @param period the period to export
   * @return report data as JSON
   */
  @GetMapping("/export/json")
  public ResponseEntity<Map<String, Object>> exportJSON(
      @RequestParam(value = "period", defaultValue = "all") String period) {
    Map<String, Object> report = reportService.generateReport(period);
    return ResponseEntity.ok(report);
  }

  /**
   * Export report data as CSV (raw CSV string in response body)
   * @param period the period to export
   * @return CSV formatted data
   */
  @GetMapping("/export/csv")
  public ResponseEntity<String> exportCSV(
      @RequestParam(value = "period", defaultValue = "all") String period) {
    List<SensorReading> readings = reportService.getReadingsByPeriod(period);
    StringBuilder csv = new StringBuilder();
    csv.append("Timestamp,Temperature (°C),Humidity (%),Soil Moisture (%),pH,Light (lx),Battery (%)").append("\n");

    for (SensorReading reading : readings) {
      csv.append(reading.getTs()).append(",")
         .append(String.format("%.1f", reading.getTemp())).append(",")
         .append(String.format("%.0f", reading.getHumidity())).append(",")
         .append(String.format("%.0f", reading.getSoil())).append(",")
         .append(String.format("%.2f", reading.getPh())).append(",")
         .append(String.format("%.0f", reading.getLux())).append(",")
         .append(String.format("%.0f", reading.getBatPct())).append("\n");
    }

    return ResponseEntity.ok()
        .header("Content-Disposition", "attachment; filename=plant-report.csv")
        .header("Content-Type", "text/csv; charset=UTF-8")
        .body(csv.toString());
  }

  @GetMapping("/periods")
  public ResponseEntity<List<String>> getAvailablePeriods() {
    return ResponseEntity.ok(List.of(
      "today",
      "last3days",
      "week",
      "twoweeks",
      "month",
      "threemonths",
      "sixmonths",
      "year",
      "all"
    ));
  }
}
