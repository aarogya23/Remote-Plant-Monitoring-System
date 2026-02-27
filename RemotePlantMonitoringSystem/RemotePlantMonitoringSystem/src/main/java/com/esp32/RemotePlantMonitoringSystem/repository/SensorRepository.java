package com.esp32.RemotePlantMonitoringSystem.repository;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import com.esp32.RemotePlantMonitoringSystem.model.SensorData;

import java.util.List;
import java.util.Optional;

@Repository
public interface SensorRepository extends JpaRepository<SensorData, Long> {

    // Get latest record
    Optional<SensorData> findTopByOrderByTimestampDesc();

    List<SensorData> findTop20ByOrderByTimestampDesc();
}