#include <ArduinoLowPower.h>
#include <Arduino_LSM6DS3.h>
#include <SPI.h>
#include <SD.h>
#include "web.hpp"
#include "sensordata.h"
#include "sensorlogging.hpp"

const int sensor_period = 10;  // ms
unsigned long last_sample_time = 0;

// SD card setup
const int chipSelect = 10;

enum FishState {
  LOW_POWER,
  COLLECTING,
  SERVING,
  ERROR,
} fish_state;

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial monitor to open
  Serial.println("Starting up");
  configure_sd_card();
  delay(2000); // give us time to upload new sketch in case the device becomes inaccessible during sleep
  switch_state(SERVING); // Start in serving mode
}

void configure_sd_card(void) {
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("SD card initialization failed!"));
  } else {
    Serial.println(F("SD card initialized."));
  }
}

void switch_state(FishState new_state) {
  Serial.print("Switching from state ");
  Serial.print(fish_state);
  Serial.print(" to state ");
  Serial.println(new_state);
  switch (fish_state) {
    case LOW_POWER:
      // nothing to do
      break;
    case COLLECTING:
      sensor_teardown();
      break;
    case SERVING:
      web_teardown();
      break;
  }
  switch (new_state) {
    case LOW_POWER:
      // nothing to do
      break;
    case COLLECTING:
      if (!sensor_setup()) {
        new_state = ERROR;
      }
      break;
    case SERVING:
      if (!web_setup()) {
        new_state = ERROR;
      }
      break;
  }
  fish_state = new_state;
}

void loop() {
  int sleep_time;
  int shake_counter;
  switch (fish_state) {
    case LOW_POWER:
      Serial.println("entering low power mode");
      shake_counter = 0;
      while (1) {
        LowPower.sleep(50);

        if (acceleration_is_high()) {
          shake_counter += 3;
        }
        if (shake_counter != 0) {
          shake_counter--;
        }
        if (shake_counter > 10) {
          while (!Serial);  // Wait for Serial monitor to open
          delay(1500);
          Serial.println("Stop shaking me!");
          switch_state(SERVING);
          break;
        }
      }
      break;
    case COLLECTING:
      sleep_time = sensor_period - (int)(millis() - last_sample_time) - 1;
      if (sleep_time) {
        LowPower.sleep(sleep_time);
      }
      while (millis() - last_sample_time < sensor_period) {}
      sensor_process();
      break;
    case SERVING:
      web_process();
      break;
    case ERROR:
      pinMode(LED_BUILTIN, OUTPUT);
      while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        LowPower.sleep(200);
        digitalWrite(LED_BUILTIN, LOW);
        LowPower.sleep(200);
      }
      break;
  }
}
