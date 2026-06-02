package com.plantos.plantos.controller;

import com.plantos.plantos.model.SensorDataRequest;
import com.plantos.plantos.model.SensorReading;
import com.plantos.plantos.repo.SensorReadingRepository;
import org.springframework.data.domain.PageRequest;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

@RestController
@CrossOrigin(origins = "*")
@RequestMapping
public class PlantosController {

  private final SensorReadingRepository repository;

  public PlantosController(SensorReadingRepository repository) {
    this.repository = repository;
  }

  @PostMapping("/api/data")
  public ResponseEntity<Void> receive(@RequestBody SensorDataRequest req) {
    SensorReading reading = new SensorReading();
    reading.setTs(Instant.now());
    reading.setTemp(req.temp);
    reading.setHumidity(req.humidity);
    reading.setSoil(req.soil);
    reading.setPh(req.ph);
    reading.setLux(req.lux);
    reading.setBatV(req.batV);
    reading.setBatPct(req.batPct);
    reading.setHealth(req.health);

    repository.save(reading);
    return ResponseEntity.ok().build();
  }

  @GetMapping("/api/latest")
  public ResponseEntity<SensorReading> latest() {
    SensorReading reading = repository.findTopByOrderByTsDesc();
    if (reading == null) return ResponseEntity.noContent().build();
    return ResponseEntity.ok(reading);
  }

  @GetMapping("/api/history")
  public List<SensorReading> history(@RequestParam(defaultValue = "40") int limit) {
    int safeLimit = Math.max(1, Math.min(limit, 200));
    return repository.history(PageRequest.of(0, safeLimit));
  }

  @GetMapping("/api/report/readings")
  public List<SensorReading> reportReadings(@RequestParam(defaultValue = "all") String period) {
    return getReadingsByPeriod(period);
  }

  @GetMapping("/api/report/export/json")
  public Map<String, Object> exportJson(@RequestParam(defaultValue = "all") String period) {
    List<SensorReading> rows = getReadingsByPeriod(period);
    return Map.of(
        "period", period,
        "count", rows.size(),
        "readings", rows
    );
  }

  @GetMapping("/api/report/export/csv")
  public ResponseEntity<String> exportCsv(@RequestParam(defaultValue = "all") String period) {
    List<SensorReading> rows = getReadingsByPeriod(period);
    StringBuilder csv = new StringBuilder();
    csv.append("Timestamp,Temperature (C),Humidity (%),Soil Moisture (%),pH,Light (lx),Battery Voltage (V),Battery (%),Health").append("\n");

    for (SensorReading r : rows) {
      csv.append(r.getTs()).append(",")
          .append(String.format("%.1f", r.getTemp())).append(",")
          .append(String.format("%.1f", r.getHumidity())).append(",")
          .append(r.getSoil()).append(",")
          .append(String.format("%.2f", r.getPh())).append(",")
          .append(String.format("%.0f", r.getLux())).append(",")
          .append(String.format("%.2f", r.getBatV())).append(",")
          .append(r.getBatPct()).append(",")
          .append(r.getHealth())
          .append("\n");
    }

    return ResponseEntity.ok()
        .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=plant-report.csv")
        .contentType(MediaType.parseMediaType("text/csv; charset=UTF-8"))
        .body(csv.toString());
  }

  @GetMapping("/api/report/export/pdf")
  public ResponseEntity<byte[]> exportPdf(@RequestParam(defaultValue = "all") String period) {
    List<SensorReading> rows = getReadingsByPeriod(period);
    byte[] pdfBytes = buildSimplePdf(rows, period);
    return ResponseEntity.ok()
        .header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=plant-report.pdf")
        .contentType(MediaType.APPLICATION_PDF)
        .body(pdfBytes);
  }

  private List<SensorReading> getReadingsByPeriod(String period) {
    List<SensorReading> all = repository.history(PageRequest.of(0, 10000));
    Instant since = periodToSince(period);
    if (since == null) return all;

    List<SensorReading> filtered = new ArrayList<>();
    for (SensorReading r : all) {
      if (r.getTs() != null && !r.getTs().isBefore(since)) {
        filtered.add(r);
      }
    }
    return filtered;
  }

  private Instant periodToSince(String period) {
    String p = period == null ? "all" : period.trim().toLowerCase();
    Instant now = Instant.now();
    switch (p) {
      case "today":
        return now.minus(1, ChronoUnit.DAYS);
      case "last3days":
        return now.minus(3, ChronoUnit.DAYS);
      case "week":
        return now.minus(7, ChronoUnit.DAYS);
      case "twoweeks":
        return now.minus(14, ChronoUnit.DAYS);
      case "month":
      case "monthly":
        return now.minus(30, ChronoUnit.DAYS);
      case "threemonths":
        return now.minus(90, ChronoUnit.DAYS);
      case "sixmonths":
        return now.minus(180, ChronoUnit.DAYS);
      case "year":
      case "yearly":
        return now.minus(365, ChronoUnit.DAYS);
      case "all":
      default:
        return null;
    }
  }

  // Minimal PDF generator (no extra dependency needed).
  private byte[] buildSimplePdf(List<SensorReading> rows, String period) {
    StringBuilder text = new StringBuilder();
    text.append("BT /F1 12 Tf 50 790 Td (PlantOS Report - ").append(escapePdf(period)).append(") Tj ET\n");
    text.append("BT /F1 10 Tf 50 772 Td (Generated: ").append(escapePdf(Instant.now().toString())).append(") Tj ET\n");
    text.append("BT /F1 10 Tf 50 756 Td (Rows: ").append(rows.size()).append(") Tj ET\n");

    int y = 736;
    int shown = Math.min(rows.size(), 35);
    for (int i = 0; i < shown; i++) {
      SensorReading r = rows.get(i);
      String line = String.format(
          "%s | T=%.1fC H=%.1f%% Soil=%d%% pH=%.2f Lux=%.0f Bat=%.2fV/%d%% Health=%d",
          r.getTs(), r.getTemp(), r.getHumidity(), r.getSoil(), r.getPh(), r.getLux(), r.getBatV(), r.getBatPct(), r.getHealth()
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
        + String.format("%010d 00000 n \n", o1)
        + String.format("%010d 00000 n \n", o2)
        + String.format("%010d 00000 n \n", o3)
        + String.format("%010d 00000 n \n", o4)
        + String.format("%010d 00000 n \n", o5)
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

