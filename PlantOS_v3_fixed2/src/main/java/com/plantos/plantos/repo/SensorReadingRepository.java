package com.plantos.plantos.repo;

import com.plantos.plantos.model.SensorReading;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;

import java.util.List;

public interface SensorReadingRepository extends JpaRepository<SensorReading, Long> {

  SensorReading findTopByOrderByTsDesc();

  @Query("select r from SensorReading r order by r.ts desc")
  List<SensorReading> history(Pageable pageable);
}

