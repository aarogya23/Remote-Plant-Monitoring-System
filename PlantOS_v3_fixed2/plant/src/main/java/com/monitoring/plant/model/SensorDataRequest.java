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
}

