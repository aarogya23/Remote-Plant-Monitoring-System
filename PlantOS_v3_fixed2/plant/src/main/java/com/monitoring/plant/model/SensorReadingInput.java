package com.monitoring.plant.model;

/**
 * Input type for GraphQL mutations
 * Used when adding new sensor readings via GraphQL API
 */
public class SensorReadingInput {

    private double temp;
    private double humidity;
    private int soil;
    private double ph;
    private double lux;
    private double batV;
    private int batPct;
    private int health;

    // Constructors
    public SensorReadingInput() {
    }

    public SensorReadingInput(double temp, double humidity, int soil, double ph,
                              double lux, double batV, int batPct, int health) {
        this.temp = temp;
        this.humidity = humidity;
        this.soil = soil;
        this.ph = ph;
        this.lux = lux;
        this.batV = batV;
        this.batPct = batPct;
        this.health = health;
    }

    // Getters and Setters
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

    @Override
    public String toString() {
        return "SensorReadingInput{" +
                "temp=" + temp +
                ", humidity=" + humidity +
                ", soil=" + soil +
                ", ph=" + ph +
                ", lux=" + lux +
                ", batV=" + batV +
                ", batPct=" + batPct +
                ", health=" + health +
                '}';
    }
}
