package com.monitoring.plant.controller;

import com.monitoring.plant.repo.SensorReadingRepository;
import com.monitoring.plant.model.SensorDataRequest;
import com.monitoring.plant.model.SensorReading;
import com.monitoring.plant.repo.SensorReadingRepository;
import org.springframework.data.domain.PageRequest;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.awt.print.Pageable;
import java.time.Instant;
import java.util.List;

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
    return repository.history((Pageable) PageRequest.of(0, safeLimit));
  }
}

