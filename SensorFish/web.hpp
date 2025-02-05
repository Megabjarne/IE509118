#ifndef _WEB_CPP
#define _WEB_CPP

#include <WiFiNINA.h>
#include <incbin.h>

#ifndef WIFI_SSID
  #define WIFI_SSID "test_ap"
#endif
#ifndef WIFI_PASSWORD
  #define WIFI_PASSWORD "password"
#endif

INCTXT(IndexText, "../ui/index.html");

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
      unsigned int current_file_index;
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
    current_client.filelist.current_file_index = 0;
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
  int n = current_client.client.write(gIndexTextData + current_client.index.bytes_sent, min(gIndexTextSize - current_client.index.bytes_sent, 64));
  current_client.index.bytes_sent += n;

  // send final newline and close connection if the document body is done being sent
  if (current_client.index.bytes_sent == gIndexTextSize) {
    current_client.client.println();
    current_client.client.flush();
    current_client.client.stop();
    current_client.active = false;
    return;
  }
};

void process_livedata(void) {
  current_client.client.println("HTTP/1.1 200 OK");
  current_client.client.println("Content-type:text/text");
  current_client.client.println();

  current_client.client.println("id: \"Sensorfish\"");
  current_client.client.println("temperature: 20.3");
  current_client.client.println("accelerometer: [1.2, 2.3, 3.4]");
  current_client.client.println("magnetometer: [1.2, 2.3, 3.4]");
  current_client.client.println("gyroscope: [1.2, 2.3, 3.4]");
  delay(10);
  current_client.client.flush();
  current_client.client.stop();
  current_client.active = false;
};

void process_filelist(void) {
  current_client.client.println("HTTP/1.1 200 OK");
  current_client.client.println("Content-type:text/text");
  current_client.client.println();

  current_client.client.println("\"data1.csv\" 1337 \"2025-02-04\"");
  current_client.client.println("\"data2.csv\" 47 \"2025-02-05\"");
  delay(10);
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
  delay(10);

  current_client.client.flush();
  current_client.client.stop();
  current_client.active = false;
};

/*
      Serial.println("new client");           // print a message out the serial port

      delayMicroseconds(10);                // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
      
      delay(10);

      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println();

      // the content of the HTTP response follows the header:
      unsigned int bytes_written = 0;
      while (bytes_written < ___webui_index_html_len) {
        int n = client.write(((char*)___webui_index_html) + bytes_written, min(___webui_index_html_len - bytes_written, 64));
        bytes_written += n;
        delay(1);
      }

      // The HTTP response ends with another blank line:
      client.println();
      // close the connection:
      client.flush();
      client.stop();
      Serial.println("client disconnected");
    }
    delay(1);
  }
}*/

#endif
