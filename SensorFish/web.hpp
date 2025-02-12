#ifndef _WEB_CPP
#define _WEB_CPP

#include <WiFiNINA.h>
#include <avr/dtostrf.h>
#include "index.h"
#include "sensordata.h"
#include <SD.h>

#ifndef WIFI_SSID
  #define WIFI_SSID "test_ap"
#endif
#ifndef WIFI_PASSWORD
  #define WIFI_PASSWORD "password"
#endif

#define INDEX_FILE_ARRAY ___ui_index_html
#define INDEX_FILE_LENGTH ___ui_index_html_len

char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASSWORD;

void process_unknown(void);
void process_index(void);
void process_livedata(void);
void process_filelist(void);
void process_filedownload(void);


WiFiServer server(80);

const unsigned int header_buffer_size = 32;
struct {
  bool active;
  WiFiClient client;
  enum {
    UNKNOWN,
    INDEX,
    LIVEDATA,
    FILELIST,
    FILEDOWNLOAD,
  } type;
  union {
    struct {
      char header_buffer[header_buffer_size];
      unsigned int bytes_received;
    } unknown;
    struct {
      bool header_sent;
      unsigned int bytes_sent;
    } index;
    struct {
        File root;
    } filelist;
    struct {
      bool header_sent;
      unsigned int bytes_sent;
      char filename[header_buffer_size];
    } filedownload;
  };
} current_client;

void web_setup(void) {
  WiFi.config(IPAddress(10, 10, 10, 1));

  delay(1000);

  auto status = WL_AP_LISTENING+1;
  while (status != WL_AP_LISTENING) {
    status = WiFi.beginAP(ssid, pass);
    Serial.println("Creating ap failed");
    delay(1000);
  }
  Serial.println("Creating ap succeeded");

  delay(1000);

  server.begin();
}

void web_process(void) {
  if (!current_client.active) {
    // wait for new client
    WiFiClient client = server.available();   // check for new client
    if (client) {
      current_client.active = true;
      current_client.client = client;
      current_client.type = current_client.UNKNOWN;
      current_client.unknown.bytes_received = 0;
    }
  } else {
    switch (current_client.type) {
      case current_client.UNKNOWN:
        process_unknown();
        break;
      case current_client.INDEX:
        process_index();
        break;
      case current_client.LIVEDATA:
        process_livedata();
        break;
      case current_client.FILELIST:
        process_filelist();
        break;
      case current_client.FILEDOWNLOAD:
        process_filedownload();
        break;
    }
  }
}

void process_unknown(void) {
  /*
   * We read data from the client until we reach a newline, then we parse the result
   */
   while (1) {
    if (current_client.client.available()) {
      char c = current_client.client.read();
      current_client.unknown.header_buffer[current_client.unknown.bytes_received] = c;
      // if we reach a newline or run out of buffer space, we null-terminate and stop
      if (c == '\n' || current_client.unknown.bytes_received == header_buffer_size - 1) {
        current_client.unknown.header_buffer[current_client.unknown.bytes_received] = 0;
        break;
      }
      current_client.unknown.bytes_received += 1;
    } else {
      // no more data from the client, we return so we can read more later
      return;
    }
  }
  Serial.println("Received request");
  Serial.println(current_client.unknown.header_buffer);
  // we've filled the header buffer, we parse it to figure out what's next
  const char* buff = current_client.unknown.header_buffer;
  if (strncmp(buff, "GET ", 4) != 0) {
    Serial.print("invalid request: ");
    Serial.print(buff);
    current_client.client.println("HTTP/1.1 400 HUH?");
    current_client.client.stop();
    current_client.active = false;
  }
  buff += 4;

  if (strncmp(buff, "/ ", 2) == 0) {
    // client is asking for index page
    current_client.type = current_client.INDEX;
    current_client.index.bytes_sent = 0;
    current_client.index.header_sent = false;
    return;
  }

  if (strncmp(buff, "/live ", 6) == 0) {
    current_client.type = current_client.LIVEDATA;
    return;
  }

  if (strncmp(buff, "/files ", 7) == 0) {
    current_client.type = current_client.FILELIST;
    current_client.filelist.root = SD.open("/");
    return;
  }

  if (strncmp(buff, "/files/", 7) == 0) {
    current_client.type = current_client.FILEDOWNLOAD;
    current_client.filedownload.header_sent = false;
    current_client.filedownload.bytes_sent = 0;

    buff += 7;
    unsigned int i = 0;
    while (buff[i] != ' ' && buff[i] != 0) {
      current_client.filedownload.filename[i] = buff[i];
      i++;
    }
    current_client.filedownload.filename[i] = 0;
    Serial.print("Starting download of file ");
    Serial.println(current_client.filedownload.filename);
    return;
  }

  // nothing matched, reject the connection
    Serial.print("invalid request: ");
    Serial.println(buff);
    current_client.client.println("HTTP/1.1 400 HUH?");
    current_client.client.println();
    current_client.client.stop();
    current_client.active = false;
}

void process_index(void) {
  // send header if it hasn't already been sent
  if (!current_client.index.header_sent) {
    current_client.client.println("HTTP/1.1 200 OK");
    current_client.client.println("Content-type:text/html");
    current_client.client.println();
    current_client.index.header_sent = true;
    return;
  }

  // write the body of the index file
  int n = current_client.client.write(INDEX_FILE_ARRAY + current_client.index.bytes_sent, min(INDEX_FILE_LENGTH - current_client.index.bytes_sent, 64));
  current_client.index.bytes_sent += n;

  // send final newline and close connection if the document body is done being sent
  if (current_client.index.bytes_sent == INDEX_FILE_LENGTH) {
    current_client.client.println();
    current_client.client.flush();
    current_client.client.stop();
    current_client.active = false;
    return;
  }
};

void process_livedata(void) {
  char buffer[12];
  current_client.client.println("HTTP/1.1 200 OK");
  current_client.client.println("Content-type:text/text");
  current_client.client.println();

  current_client.client.println("id: \"Sensorfish_1\"");
  
  current_client.client.print("temperature: ");
  dtostrf(sensor_readings.temperature, 0, 3, buffer);
  current_client.client.println(buffer);
  
  current_client.client.print("accelerometer: [");
  dtostrf(sensor_readings.accelerometer.x, 0, 3, buffer);
  current_client.client.print(buffer);
  current_client.client.print(", ");
  dtostrf(sensor_readings.accelerometer.y, 0, 3, buffer);
  current_client.client.print(buffer);
  current_client.client.print(", ");
  dtostrf(sensor_readings.accelerometer.z, 0, 3, buffer);
  current_client.client.print(buffer);
  current_client.client.println("]");
  
  current_client.client.println("magnetometer: [0.000, 0.000, 0.000]");
  
  current_client.client.print("gyroscope: [");
  dtostrf(sensor_readings.gyroscope.x, 0, 3, buffer);
  current_client.client.print(buffer);
  current_client.client.print(", ");
  dtostrf(sensor_readings.gyroscope.y, 0, 3, buffer);
  current_client.client.print(buffer);
  current_client.client.print(", ");
  dtostrf(sensor_readings.gyroscope.z, 0, 3, buffer);
  current_client.client.print(buffer);
  current_client.client.println("]");
  
  delay(1);
  current_client.client.flush();
  current_client.client.stop();
  current_client.active = false;
};

void process_filelist(void) {
  current_client.client.println("HTTP/1.1 200 OK");
  current_client.client.println("Content-type:text/text");
  current_client.client.println();

  //current_client.client.println("\"data1.csv\" 1337 \"2025-02-04\"");
  File next_file = current_client.filelist.root.openNextFile();
  if (next_file) {
    if (!next_file.isDirectory() && next_file.name().endsWith(".csv")) {
        current_client.client.print(next_file.name());
        current_client.client.print(" ");
        current_client.client.print(next_file.size());
        current_client.client.println(" \"unknown date\"");
    }
  } else {
    current_client.filelist.root.close();
    delay(1);
    current_client.client.flush();
    current_client.client.stop();
    current_client.active = false;
};
void process_filedownload(void) {
  current_client.client.println("HTTP/1.1 200 OK");
  current_client.client.println("Content-type:text/csv");
  current_client.client.println("Content-Length: 14");
  current_client.client.println("Content-Disposition: attachment; filename=data1.csv");
  current_client.client.println();

  current_client.client.println("ok, ja");
  current_client.client.println("1, 2");
  delay(1);

  current_client.client.flush();
  current_client.client.stop();
  current_client.active = false;
};

#endif
