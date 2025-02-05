#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_Accel.h>
#include <Adafruit_LSM303DLH_Mag.h>
#include <Adafruit_Simple_AHRS.h>
#include <SPI.h>
#include <SD.h>

// Create sensor instances.
Adafruit_LSM303_Accel_Unified accel(30301);
Adafruit_LSM303DLH_Mag_Unified mag(30302);

// Create simple AHRS algorithm using the above sensors.
Adafruit_Simple_AHRS ahrs(&accel, &mag);

// SD card setup
const int chipSelect = 10;
File dataFile;

void setup()
{
  Serial.begin(115200);
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
      dataFile.println("Timestamp_ms,Roll,Pitch,Heading,AccelX,AccelY,AccelZ,MagX,MagY,MagZ");
    }
    dataFile.close();
  }

  Serial.println(F("Adafruit 9 DOF Board AHRS Example\n"));

  // Initialize the sensors.
  accel.begin();
  mag.begin();
}

void loop(void)
{
  sensors_vec_t orientation;
  sensors_event_t accel_event, mag_event;
  unsigned long timestamp = millis(); // Get time in milliseconds

  // Get accelerometer and magnetometer data
  accel.getEvent(&accel_event);
  mag.getEvent(&mag_event);

  // Use the simple AHRS function to get the current orientation.
  if (ahrs.getOrientation(&orientation))
  {
    /* Print orientation data */
    Serial.print(F("Time(ms): "));
    Serial.print(timestamp);
    Serial.print(F(" | Roll: "));
    Serial.print(orientation.roll);
    Serial.print(F(" Pitch: "));
    Serial.print(orientation.pitch);
    Serial.print(F(" Heading: "));
    Serial.println(orientation.heading);
  }

  /* Print accelerometer data */
  Serial.print(F("Accelerometer (m/s²): X="));
  Serial.print(accel_event.acceleration.x);
  Serial.print(F(" Y="));
  Serial.print(accel_event.acceleration.y);
  Serial.print(F(" Z="));
  Serial.println(accel_event.acceleration.z);

  /* Print magnetometer data */
  Serial.print(F("Magnetometer (µT): X="));
  Serial.print(mag_event.magnetic.x);
  Serial.print(F(" Y="));
  Serial.print(mag_event.magnetic.y);
  Serial.print(F(" Z="));
  Serial.println(mag_event.magnetic.z);

  // Save data to SD card
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(timestamp);
    dataFile.print(",");
    dataFile.print(orientation.roll);
    dataFile.print(",");
    dataFile.print(orientation.pitch);
    dataFile.print(",");
    dataFile.print(orientation.heading);
    dataFile.print(",");
    dataFile.print(accel_event.acceleration.x);
    dataFile.print(",");
    dataFile.print(accel_event.acceleration.y);
    dataFile.print(",");
    dataFile.print(accel_event.acceleration.z);
    dataFile.print(",");
    dataFile.print(mag_event.magnetic.x);
    dataFile.print(",");
    dataFile.print(mag_event.magnetic.y);
    dataFile.print(",");
    dataFile.println(mag_event.magnetic.z);
    dataFile.close();
  } else {
    Serial.println(F("Error opening data.csv"));
  }

  delay(100);
}
