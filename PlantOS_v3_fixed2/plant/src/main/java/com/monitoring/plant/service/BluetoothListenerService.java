//package com.monitoring.plant.service;
//
//import com.fasterxml.jackson.databind.ObjectMapper;
//import com.fazecast.jSerialComm.SerialPort;
//import com.fazecast.jSerialComm.SerialPortDataListener;
//import com.fazecast.jSerialComm.SerialPortEvent;
//import com.monitoring.plant.model.SensorDataRequest;
//import com.monitoring.plant.model.SensorReading;
//import com.monitoring.plant.repo.SensorReadingRepository;
//import jakarta.annotation.PostConstruct;
//import jakarta.annotation.PreDestroy;
//import org.springframework.beans.factory.annotation.Autowired;
//import org.springframework.beans.factory.annotation.Value;
//import org.springframework.stereotype.Service;
//
//import java.time.Instant;
//
//@Service
//public class BluetoothListenerService {
//
//    @Value("${bluetooth.port.name:COM4}")
//    private String portName;
//
//    @Autowired
//    private SensorReadingRepository repository;
//
//    @Autowired
//    private ObjectMapper objectMapper;
//
//    private SerialPort comPort;
//    private StringBuilder buffer = new StringBuilder();
//
//    @PostConstruct
//    public void init() {
//        System.out.println("Attempting to connect to Bluetooth Serial Port: " + portName);
//        comPort = SerialPort.getCommPort(portName);
//
//        // Settings for standard Bluetooth SPP
//        comPort.setBaudRate(115200);
//        comPort.setNumDataBits(8);
//        comPort.setNumStopBits(1);
//        comPort.setParity(SerialPort.NO_PARITY);
//
//        if (comPort.openPort()) {
//            System.out.println("Successfully opened Bluetooth port: " + portName);
//        } else {
//            System.err.println("Failed to open Bluetooth port: " + portName + ". Is the ESP32 paired and connected?");
//            return;
//        }
//
//        comPort.addDataListener(new SerialPortDataListener() {
//            @Override
//            public int getListeningEvents() {
//                return SerialPort.LISTENING_EVENT_DATA_AVAILABLE;
//            }
//
//            @Override
//            public void serialEvent(SerialPortEvent event) {
//                if (event.getEventType() != SerialPort.LISTENING_EVENT_DATA_AVAILABLE)
//                    return;
//
//                byte[] newData = new byte[comPort.bytesAvailable()];
//                int numRead = comPort.readBytes(newData, newData.length);
//                String receivedText = new String(newData, 0, numRead);
//
//                buffer.append(receivedText);
//
//                // Process line by line
//                int newlineIndex;
//                while ((newlineIndex = buffer.indexOf("\n")) != -1) {
//                    String line = buffer.substring(0, newlineIndex).trim();
//                    buffer.delete(0, newlineIndex + 1);
//
//                    if (!line.isEmpty()) {
//                        processReceivedJson(line);
//                    }
//                }
//            }
//        });
//    }
//
//    private void processReceivedJson(String jsonString) {
//        if (!jsonString.startsWith("{")) {
//            // Ignore debug text from the ESP32 that isn't JSON
//            System.out.println("[BT DEBUG]: " + jsonString);
//            return;
//        }
//
//        try {
//            SensorDataRequest dto = objectMapper.readValue(jsonString, SensorDataRequest.class);
//
//            SensorReading reading = new SensorReading();
//            reading.setTs(Instant.now());
//            reading.setTemp(dto.temp);
//            reading.setHumidity(dto.humidity);
//            reading.setSoil(dto.soil);
//            reading.setPh(dto.ph);
//            reading.setLux(dto.lux);
//            reading.setBatV(dto.batV);
//            reading.setBatPct(dto.batPct);
//            reading.setHealth(dto.health);
//
//            repository.save(reading);
//            System.out.println("[BT OK] Saved reading to DB - Soil: " + dto.soil + "%, Temp: " + dto.temp + "C");
//
//        } catch (Exception e) {
//            System.err.println("[BT WARN] Failed to parse JSON from Bluetooth: " + e.getMessage());
//            System.err.println("Received string was: " + jsonString);
//        }
//    }
//
//    @PreDestroy
//    public void cleanup() {
//        if (comPort != null && comPort.isOpen()) {
//            comPort.closePort();
//            System.out.println("Closed Bluetooth port.");
//        }
//    }
//}
