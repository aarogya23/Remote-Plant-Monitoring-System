package com.monitoring.plant.controller;

import com.monitoring.plant.service.ReportService;
import com.monitoring.plant.model.SensorReading;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.Instant;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Locale;
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
    csv.append("Timestamp,Temperature (C),Humidity (%),Soil Moisture (%),pH,IR Proximity,Battery (%)").append("\n");

    for (SensorReading reading : readings) {
      csv.append(reading.getTs()).append(",")
         .append(String.format(Locale.US, "%.1f", reading.getTemp())).append(",")
         .append(String.format(Locale.US, "%.0f", reading.getHumidity())).append(",")
         .append(reading.getSoil()).append(",")
         .append(String.format(Locale.US, "%.2f", reading.getPh())).append(",")
         .append(String.format(Locale.US, "%.0f", reading.getLux())).append(",")
         .append(reading.getBatPct()).append("\n");
    }

    return ResponseEntity.ok()
        .header("Content-Disposition", "attachment; filename=plant-report.csv")
        .header("Content-Type", "text/csv; charset=UTF-8")
        .body(csv.toString());
  }

  @GetMapping("/export/pdf")
  public ResponseEntity<byte[]> exportPDF(
      @RequestParam(value = "period", defaultValue = "all") String period) {
    List<SensorReading> readings = reportService.getReadingsByPeriod(period);
    byte[] pdfBytes = buildSimplePdf(readings, period);
    return ResponseEntity.ok()
        .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=plant-report.pdf")
        .contentType(MediaType.APPLICATION_PDF)
        .body(pdfBytes);
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

  // Minimal PDF payload without extra dependencies.
  private byte[] buildSimplePdf(List<SensorReading> rows, String period) {
    StringBuilder text = new StringBuilder();
    text.append("BT /F1 12 Tf 50 790 Td (Plant Report - ").append(escapePdf(period)).append(") Tj ET\n");
    text.append("BT /F1 10 Tf 50 772 Td (Generated: ").append(escapePdf(Instant.now().toString())).append(") Tj ET\n");
    text.append("BT /F1 10 Tf 50 756 Td (Rows: ").append(rows.size()).append(") Tj ET\n");

    int y = 736;
    int shown = Math.min(rows.size(), 35);
    for (int i = 0; i < shown; i++) {
      SensorReading r = rows.get(i);
      String line = String.format(
          Locale.US,
          "%s | T=%.1fC H=%.0f%% Soil=%d%% pH=%.2f IR=%.0f Bat=%d%%",
          r.getTs(), r.getTemp(), r.getHumidity(), r.getSoil(), r.getPh(), r.getLux(), r.getBatPct()
      );
      text.append("BT /F1 8 Tf 50 ").append(y).append(" Td (").append(escapePdf(line)).append(") Tj ET\n");
      y -= 18;
      if (y < 60) break;
    }
    if (rows.size() > shown) {
      text.append("BT /F1 8 Tf 50 ").append(Math.max(y, 40)).append(" Td (Showing first ")
          .append(shown).append(" rows) Tj ET\n");
    }

    String stream = text.toString();
    byte[] streamBytes = stream.getBytes(StandardCharsets.US_ASCII);

    String obj1 = "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    String obj2 = "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
    String obj3 = "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>\nendobj\n";
    String obj4 = "4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n";
    String obj5Head = "5 0 obj\n<< /Length " + streamBytes.length + " >>\nstream\n";
    String obj5Tail = "endstream\nendobj\n";

    byte[] header = "%PDF-1.4\n".getBytes(StandardCharsets.US_ASCII);
    byte[] b1 = obj1.getBytes(StandardCharsets.US_ASCII);
    byte[] b2 = obj2.getBytes(StandardCharsets.US_ASCII);
    byte[] b3 = obj3.getBytes(StandardCharsets.US_ASCII);
    byte[] b4 = obj4.getBytes(StandardCharsets.US_ASCII);
    byte[] b5h = obj5Head.getBytes(StandardCharsets.US_ASCII);
    byte[] b5t = obj5Tail.getBytes(StandardCharsets.US_ASCII);

    int o1 = header.length;
    int o2 = o1 + b1.length;
    int o3 = o2 + b2.length;
    int o4 = o3 + b3.length;
    int o5 = o4 + b4.length;
    int xrefStart = o5 + b5h.length + streamBytes.length + b5t.length;

    String xref = "xref\n0 6\n"
        + "0000000000 65535 f \n"
        + String.format(Locale.US, "%010d 00000 n \n", o1)
        + String.format(Locale.US, "%010d 00000 n \n", o2)
        + String.format(Locale.US, "%010d 00000 n \n", o3)
        + String.format(Locale.US, "%010d 00000 n \n", o4)
        + String.format(Locale.US, "%010d 00000 n \n", o5)
        + "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n"
        + xrefStart + "\n%%EOF\n";
    byte[] bx = xref.getBytes(StandardCharsets.US_ASCII);

    byte[] out = new byte[header.length + b1.length + b2.length + b3.length + b4.length + b5h.length + streamBytes.length + b5t.length + bx.length];
    int pos = 0;
    System.arraycopy(header, 0, out, pos, header.length); pos += header.length;
    System.arraycopy(b1, 0, out, pos, b1.length); pos += b1.length;
    System.arraycopy(b2, 0, out, pos, b2.length); pos += b2.length;
    System.arraycopy(b3, 0, out, pos, b3.length); pos += b3.length;
    System.arraycopy(b4, 0, out, pos, b4.length); pos += b4.length;
    System.arraycopy(b5h, 0, out, pos, b5h.length); pos += b5h.length;
    System.arraycopy(streamBytes, 0, out, pos, streamBytes.length); pos += streamBytes.length;
    System.arraycopy(b5t, 0, out, pos, b5t.length); pos += b5t.length;
    System.arraycopy(bx, 0, out, pos, bx.length);
    return out;
  }

  private String escapePdf(String input) {
    if (input == null) return "";
    return input.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)");
  }
}
