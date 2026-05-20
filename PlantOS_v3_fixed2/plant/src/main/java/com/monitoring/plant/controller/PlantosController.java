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

import org.springframework.data.domain.Pageable;
import java.time.Instant;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.io.IOException;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

@RestController
@CrossOrigin(origins = "*")
@RequestMapping
public class PlantosController {

  private final SensorReadingRepository repository;
  private final CopyOnWriteArrayList<SseEmitter> emitters = new CopyOnWriteArrayList<>();

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

    // Broadcast the new reading to all connected SSE clients
    String json = "{\"id\":" + reading.getId() + ",\"temp\":" + reading.getTemp() + ",\"humidity\":" + reading.getHumidity() 
      + ",\"soil\":" + reading.getSoil() + ",\"ph\":" + reading.getPh() + ",\"lux\":" + reading.getLux() 
      + ",\"batV\":" + reading.getBatV() + ",\"batPct\":" + reading.getBatPct() + ",\"health\":" + reading.getHealth() + "}";
    
    for (SseEmitter emitter : emitters) {
      try {
        emitter.send(SseEmitter.event().data(json).build());
      } catch (IOException e) {
        emitters.remove(emitter);
      }
    }

    return ResponseEntity.ok().build();
  }

  @GetMapping("/api/stream")
  public SseEmitter stream() {
    SseEmitter emitter = new SseEmitter(Long.MAX_VALUE);
    emitters.add(emitter);
    
    emitter.onCompletion(() -> emitters.remove(emitter));
    emitter.onTimeout(() -> emitters.remove(emitter));
    emitter.onError((e) -> emitters.remove(emitter));
    
    return emitter;
  }

  @GetMapping("/api/latest")
  public ResponseEntity<SensorReading> latest() {
    SensorReading reading = repository.findTopByOrderByTsDesc();
    if (reading == null) return ResponseEntity.noContent().build();
    return ResponseEntity.ok(reading);
  }

  @GetMapping("/api/mock")
  public ResponseEntity<SensorReading> generateMockData() {
    SensorReading reading = new SensorReading();
    reading.setTs(Instant.now());
    reading.setTemp(20.5 + Math.random() * 10);
    reading.setHumidity(50 + Math.random() * 20);
    reading.setSoil(45 + (int)(Math.random() * 20));
    reading.setPh(6.5 + Math.random() * 1);
    reading.setLux(5000 + Math.random() * 15000);
    reading.setBatV(7.8 + Math.random() * 0.4);
    reading.setBatPct(75 + (int)(Math.random() * 25));
    reading.setHealth(80 + (int)(Math.random() * 20));

    repository.save(reading);

    // Broadcast to SSE clients
    String json = "{\"id\":" + reading.getId() + ",\"temp\":" + reading.getTemp() + ",\"humidity\":" + reading.getHumidity() 
      + ",\"soil\":" + reading.getSoil() + ",\"ph\":" + reading.getPh() + ",\"lux\":" + reading.getLux() 
      + ",\"batV\":" + reading.getBatV() + ",\"batPct\":" + reading.getBatPct() + ",\"health\":" + reading.getHealth() + "}";
    
    for (SseEmitter emitter : emitters) {
      try {
        emitter.send(SseEmitter.event().data(json).build());
      } catch (IOException e) {
        emitters.remove(emitter);
      }
    }

    return ResponseEntity.ok(reading);
  }

  @GetMapping("/api/history")
  public List<SensorReading> history(@RequestParam(defaultValue = "40") int limit) {
    int safeLimit = Math.max(1, Math.min(limit, 200));
    return repository.history((Pageable) PageRequest.of(0, safeLimit));
  }
}

