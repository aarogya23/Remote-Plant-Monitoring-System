package com.monitoring.plant.model;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

import java.time.Instant;

@Entity
@Table(name = "sensor_readings")
public class SensorReading {

  @Id
  @GeneratedValue(strategy = GenerationType.IDENTITY)
  private Long id;

  @Column(nullable = false)
  private Instant ts;

  private double temp;
  private double humidity;
  private int soil;
  private double ph;
  private double lux;
  private double batV;
  private int batPct;
  private int health;

  public Long getId() {
    return id;
  }

  public void setId(Long id) {
    this.id = id;
  }

  public Instant getTs() {
    return ts;
  }

  public void setTs(Instant ts) {
    this.ts = ts;
  }

  public double getTemp() {
    return temp;
  }

  public void setTemp(double temp) {
    this.temp = temp;
  }

  public double getHumidity() {
    return humidity;
  }

  public void setHumidity(double humidity) {
    this.humidity = humidity;
  }

  public int getSoil() {
    return soil;
  }

  public void setSoil(int soil) {
    this.soil = soil;
  }

  public double getPh() {
    return ph;
  }

  public void setPh(double ph) {
    this.ph = ph;
  }

  public double getLux() {
    return lux;
  }

  public void setLux(double lux) {
    this.lux = lux;
  }

  public double getBatV() {
    return batV;
  }

  public void setBatV(double batV) {
    this.batV = batV;
  }

  public int getBatPct() {
    return batPct;
  }

  public void setBatPct(int batPct) {
    this.batPct = batPct;
  }

  public int getHealth() {
    return health;
  }

  public void setHealth(int health) {
    this.health = health;
  }
}

