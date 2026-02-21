🌱 Remote Plant Monitoring System (Arduino Mega Based)
📌 Project Overview

The Remote Plant Monitoring System is an IoT-based smart agriculture system built using the Arduino Mega microcontroller.

It monitors environmental conditions, soil nutrients, moisture levels, and fire hazards, while automatically controlling irrigation using a relay-controlled water motor.

🎯 Objectives

    Monitor temperature & humidity

    Measure soil moisture

    Detect soil nutrients (NPK)

    Monitor soil pH level

    Detect fire hazards

    Automatically control irrigation

    Display live data on LED display

🧠 Why We Used Arduino Mega

We selected Arduino Mega 2560 because:

✅ Large number of digital & analog pins

✅ Multiple serial communication ports (useful for NPK sensor)

✅ Larger memory compared to Arduino Uno

✅ Suitable for multi-sensor integration

Since our project uses multiple sensors (DHT11, Soil, NPK, pH, Fire, Relay, LED Display), Arduino Mega is ideal for handling all hardware communication efficiently.

🧰 Hardware Components Used
        Component	Purpose
        Arduino Mega 2560	Main microcontroller
        DHT11 Sensor	Temperature & Humidity
        Soil Moisture Sensor	Soil water level detection
        NPK Sensor	Nitrogen, Phosphorus, Potassium measurement
        pH Sensor	Soil acidity detection
        Fire Sensor	Fire/flame detection
        Single Channel Relay	Controls water motor
        Water Motor	Irrigation system
        LED Display (16x2 / 20x4)	Displays sensor readings

⚙️ System Working

    Sensors send data to Arduino Mega.

    Arduino processes all sensor readings.

    Values are displayed on LED display.

    If soil moisture < threshold:

    Relay turns ON

    Water motor starts

    If moisture is sufficient:

    Motor turns OFF

    If fire detected:

    Alert triggered