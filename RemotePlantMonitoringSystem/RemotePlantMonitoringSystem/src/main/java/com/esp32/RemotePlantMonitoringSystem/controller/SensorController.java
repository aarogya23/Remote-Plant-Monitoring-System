package com.esp32.RemotePlantMonitoringSystem.controller;



import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

import com.esp32.RemotePlantMonitoringSystem.model.SensorData;
import com.esp32.RemotePlantMonitoringSystem.repository.SensorRepository;

import java.util.List;

@RestController
@RequestMapping("/api/sensor")
@CrossOrigin
@RequiredArgsConstructor
public class SensorController {

    private final SensorRepository repository;

    // 🔹 Save data (ESP32 will call this)
    @PostMapping
    public String saveData(@RequestBody SensorData data) {
        repository.save(data);
        return "Data Saved Successfully";
    }

    // 🔹 Get all sensor data
    @GetMapping
    public List<SensorData> getAllData() {
        return repository.findAll();
    }

    // 🔹 Get latest sensor reading
    @GetMapping("/latest")
    public SensorData getLatestData() {
        return repository.findTopByOrderByTimestampDesc()
                .orElse(null);
    }

    // 🔹 Delete all data (optional testing)
    @DeleteMapping
    public String deleteAll() {
        repository.deleteAll();
        return "All Sensor Data Deleted";
    }
}
