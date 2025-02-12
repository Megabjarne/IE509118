#include <Arduino_LSM6DS3.h>
#include <SPI.h>
#include <SD.h>
#include "web.hpp"

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
    last_sample = millis();
    float temperature = 0.0, gx = 0.0, gy = 0.0, gz = 0.0;
    float ax = 0.0, ay = 0.0, az = 0.0;
    unsigned long timestamp = millis();

    if (IMU.temperatureAvailable()) {
      IMU.readTemperature(temperature);
    }
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gx, gy, gz);
    }
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(ax, ay, az);
    }

    // Print sensor readings to Serial
    Serial.print("Time(ms): ");
    Serial.print(timestamp);
    Serial.print(" | Temp (C): ");
    Serial.print(temperature);
    Serial.print(" | Gyro (dps): X: ");
    Serial.print(gx);
    Serial.print(" Y: ");
    Serial.print(gy);
    Serial.print(" Z: ");
    Serial.print(gz);
    Serial.print(" | Accel (g): X: ");
    Serial.print(ax);
    Serial.print(" Y: ");
    Serial.print(ay);
    Serial.print(" Z: ");
    Serial.println(az);

    // Save data to SD card
    dataFile = SD.open("data.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.print(timestamp);
      dataFile.print(",");
      dataFile.print(temperature);
      dataFile.print(",");
      dataFile.print(gx);
      dataFile.print(",");
      dataFile.print(gy);
      dataFile.print(",");
      dataFile.print(gz);
      dataFile.print(",");
      dataFile.print(ax);
      dataFile.print(",");
      dataFile.print(ay);
      dataFile.print(",");
      dataFile.println(az);
      dataFile.close();
    } else {
      Serial.println(F("Error opening data.csv"));
    }
  }

  // Process web server tasks
  web_process();
}
