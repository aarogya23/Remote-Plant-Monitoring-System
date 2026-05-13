package com.monitoring.plant.repo;

import com.monitoring.plant.model.SensorReading;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;


import java.awt.print.Pageable;
import java.util.List;

public interface SensorReadingRepository extends JpaRepository<SensorReading, Long> {

  SensorReading findTopByOrderByTsDesc();

  @Query("select r from SensorReading r order by r.ts desc")
  List<SensorReading> history(Pageable pageable);
}

