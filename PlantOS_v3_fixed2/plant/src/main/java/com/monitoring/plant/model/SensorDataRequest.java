package com.monitoring.plant.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

@JsonIgnoreProperties(ignoreUnknown = true)
public class SensorDataRequest {
  public double temp;
  public double humidity;
  public int soil;
  public double ph;
  public double lux;
  public double batV;
  public int batPct;
  public int health;
  // Light/dark time accumulated on the device since power-on
  public Long lightSec;
  public Long darkSec;
  public Integer lightPct;
  public Long uptimeSec;
}

