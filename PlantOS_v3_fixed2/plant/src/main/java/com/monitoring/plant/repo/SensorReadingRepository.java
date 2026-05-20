package com.monitoring.plant.repo;

import com.monitoring.plant.model.SensorReading;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;

public interface SensorReadingRepository extends JpaRepository<SensorReading, Long> {

  SensorReading findTopByOrderByTsDesc();

  @Query("select r from SensorReading r order by r.ts desc")
  List<SensorReading> history(Pageable pageable);

  /**
   * Get latest sensor readings ordered by timestamp descending
   * @param pageable pagination information
   * @return list of latest readings
   */
  @Query("select r from SensorReading r order by r.ts desc")
  List<SensorReading> findLatestReadings(Pageable pageable);

  /**
   * Find sensor readings by pH range
   * @param minPh minimum pH value
   * @param maxPh maximum pH value
   * @return list of readings within pH range
   */
  @Query("select r from SensorReading r where r.ph >= :minPh and r.ph <= :maxPh order by r.ts desc")
  List<SensorReading> findReadingsByPhRange(@Param("minPh") Double minPh, @Param("maxPh") Double maxPh);

  /**
   * Find sensor readings by soil moisture range
   * @param minSoil minimum soil moisture value
   * @param maxSoil maximum soil moisture value
   * @return list of readings within soil moisture range
   */
  @Query("select r from SensorReading r where r.soil >= :minSoil and r.soil <= :maxSoil order by r.ts desc")
  List<SensorReading> findReadingsBySoilRange(@Param("minSoil") Integer minSoil, @Param("maxSoil") Integer maxSoil);
}

