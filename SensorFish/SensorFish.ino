#include <Arduino_LSM6DS3.h>
#include <SPI.h>
#include <SD.h>
#include "web.hpp"
#include "sensordata.h"

// SD card setup
const int chipSelect = 10;
File dataFile;

unsigned long last_sample = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial monitor to open

  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("SD card initialization failed!"));
    while (1);
  }
  Serial.println(F("SD card initialized."));

  // Create/open file and write headers if new
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    if (dataFile.size() == 0) {
      // Updated headers: removed magnetometer and orientation data
      dataFile.println("Timestamp_ms,Temperature,GyroX,GyroY,GyroZ,AccelX,AccelY,AccelZ");
    }
    dataFile.close();
  }

  Serial.println(F("LSM6DS3 Sensor Example\n"));

  // Initialize the LSM6DS3 sensor
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Optionally print sensor sample rates for reference
  Serial.print("Temperature sensor sample rate = ");
  Serial.print(IMU.temperatureSampleRate());
  Serial.println(" Hz");

  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");

  // Start web server (assuming web_setup() is defined in web.hpp)
  web_setup();
}

void loop() {
  // Sample every 100ms
  if (millis() - last_sample > 100) {
    sensor_readings.timestamp = millis();
    last_sample = sensor_readings.timestamp;

    if (IMU.temperatureAvailable()) {
      IMU.readTemperature(sensor_readings.temperature);
    }
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(
        sensor_readings.gyroscope.x,
        sensor_readings.gyroscope.y,
        sensor_readings.gyroscope.z
      );
    }
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(
        sensor_readings.accelerometer.x,
        sensor_readings.accelerometer.y,
        sensor_readings.accelerometer.z
      );
    }

    // Print sensor readings to Serial
    Serial.print("Time(ms): ");
    Serial.print(sensor_readings.timestamp);
    Serial.print(" | Temp (C): ");
    Serial.print(sensor_readings.temperature);
    Serial.print(" | sensor_readings.gyroscope.yro (dps): X: ");
    Serial.print(sensor_readings.gyroscope.x);
    Serial.print(" Y: ");
    Serial.print(sensor_readings.gyroscope.y);
    Serial.print(" Z: ");
    Serial.print(sensor_readings.gyroscope.z);
    Serial.print(" | Accel (g): X: ");
    Serial.print(sensor_readings.accelerometer.x);
    Serial.print(" Y: ");
    Serial.print(sensor_readings.accelerometer.y);
    Serial.print(" Z: ");
    Serial.println(sensor_readings.accelerometer.z);

    // Save data to SD card
    dataFile = SD.open("data.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.print(sensor_readings.timestamp);
      dataFile.print(",");
      dataFile.print(sensor_readings.temperature);
      dataFile.print(",");
      dataFile.print(sensor_readings.gyroscope.x);
      dataFile.print(",");
      dataFile.print(sensor_readings.gyroscope.y);
      dataFile.print(",");
      dataFile.print(sensor_readings.gyroscope.z);
      dataFile.print(",");
      dataFile.print(sensor_readings.accelerometer.x);
      dataFile.print(",");
      dataFile.print(sensor_readings.accelerometer.y);
      dataFile.print(",");
      dataFile.println(sensor_readings.accelerometer.z);
      dataFile.close();
    } else {
      Serial.println(F("Error opening data.csv"));
    }
  }

  // Process web server tasks
  web_process();
}
