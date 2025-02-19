
// Define the analog pin for the TMP36 sensor's Vout pin
#define sensorPin A0

// SD card setup
const int chipSelect = 10;
File dataFile;

// Timer for reading the TMP36 sensor every 1000 ms
unsigned long lastTempSample = 0;

// Circular buffer for averaging the last 10 temperature readings
float tempReadings[10] = {0};
int tempIndex = 0;
int tempCount = 0; // how many readings have been stored (max 10)

bool sensor_setup(void) {
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("SD card initialization failed!"));
    return false;
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
    return false;
  }

  // Optionally print sensor sample rates for reference
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");

  return true;
}

void sensor_teardown(void) {

}

void sensor_process(void) {
    sensor_readings.timestamp = millis();

    // Read the TMP36 temperature sensor every 1000 ms
    if (millis() - lastTempSample >= 1000) {
      int reading = analogRead(sensorPin);
      // Convert the analog reading to voltage (adjust 3.3 if your board uses a different Vref)
      float voltage = reading * (3.3 / 1024.0);
      // Convert voltage to temperature in Celsius (TMP36 specifics: 500 mV offset and 10 mV/°C scale factor, plus offset correction)
      float temperatureC = (voltage - 0.5) * 100 + 9;
      
      // Store new temperature reading in the circular buffer
      tempReadings[tempIndex] = temperatureC;
      tempIndex = (tempIndex + 1) % 10;
      if (tempCount < 10) {
        tempCount++;
      }
      
      // Compute the average of the last readings
      float sum = 0;
      for (int i = 0; i < tempCount; i++) {
        sum += tempReadings[i];
      }
      float avgTemperature = sum / tempCount;
      sensor_readings.temperature = avgTemperature;
      
      lastTempSample = millis();
    }
    
    // Alternatively, if you prefer to use the IMU's temperature sensor, you could use:
    /*
    if (IMU.temperatureAvailable()) {
      IMU.readTemperature(sensor_readings.temperature);
    }
    */

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
