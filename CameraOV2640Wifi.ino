#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Wire.h>
#include <SPI.h>

#include <ArduCAM.h>
#include "memorysaver.h"

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#define OV2640_CHIPID_HIGH   0x0A
#define OV2640_CHIPID_LOW   0x0B


const int CS = 16;
uint8_t vid, pid;

int wifiType = 1; // 0:Station  1:AP
const char* ssid = ""; // Put your SSID here
const char* password = ""; // Put your PASSWORD here

ESP8266WebServer server(80);
ArduCAM myCAM(OV2640, CS);
void setup() {
  pinMode(CS, OUTPUT);
  Wire.begin(4, 5);

  Serial.begin(115200);
  Serial.println("ArduCAM Start!");

  SPI.begin();
  SPI.setFrequency(4000000); //4MHz

  if (spiWorks()) {
    Serial.println("Can do stuff?");
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26) || (pid != 0x42)) {
      Serial.println("Can't find OV2640 module!");
      // while (1);
    } else {
      Serial.println("OV2640 detected.");
    }
    myCAM.set_format(JPEG);
    myCAM.InitCAM();
    myCAM.OV2640_set_JPEG_size(OV2640_640x480);
    myCAM.clear_fifo_flag();
    myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);

    if (wifiType == 0) {
      // Connect to WiFi network
      Serial.println();
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(ssid);

      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("WiFi connected");
      Serial.println("");
      Serial.println(WiFi.localIP());
    } else if (wifiType == 1) {
      Serial.println();
      Serial.println();
      Serial.print("Share AP: ");
      Serial.println(ssid);

      WiFi.mode(WIFI_AP);
      WiFi.softAP(ssid, password);
      Serial.println("");
      Serial.println(WiFi.softAPIP());
    }

    // Start the server
    server.on("/capture", HTTP_GET, serverCapture);
    server.on("/stream", HTTP_GET, serverStream);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("Server started");
  }

  else {
    Serial.println("Cannot proceed");
  }
}


bool spiWorks() {
  //ArduCAM myCAM(OV2640, CS);
  int testValue = 0x55;
  myCAM.write_reg(ARDUCHIP_TEST1, testValue);
  uint8_t temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp == testValue) {
    return true;
  }
  Serial.println("SPI1 interface Error!");
  Serial.print("Expected: ");
  Serial.println(testValue, 2);
  Serial.print("Got: ");
  Serial.println(temp, 2);
  return false;
}
void start_capture() {
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

void camCapture(ArduCAM myCAM) {
  WiFiClient client = server.client();

  size_t len = myCAM.read_fifo_length();
  if (len >= 393216) {
    Serial.println("Over size.");
    return;
  } else if (len == 0 ) {
    Serial.println("Size is 0.");
    return;
  }

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  SPI.transfer(0xFF);

  if (!client.connected()) return;
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: image/jpeg\r\n";
  response += "Content-Length: " + String(len) + "\r\n\r\n";
  server.sendContent(response);

  static const size_t bufferSize = 1024;
  static uint8_t buffer[bufferSize] = {0xFF};

  while (len) {
    size_t will_copy = (len < bufferSize) ? len : bufferSize;
    SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
    if (!client.connected()) break;
    client.write(&buffer[0], will_copy);
    len -= will_copy;
  }

  myCAM.CS_HIGH();
}

void serverCapture() {
  start_capture();
  Serial.println("CAM Capturing");

  int total_time = 0;

  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  Serial.print("capture total_time used (in miliseconds):");
  Serial.println(total_time, DEC);

  total_time = 0;

  Serial.println("CAM Capture Done!");
  total_time = millis();
  camCapture(myCAM);
  total_time = millis() - total_time;
  Serial.print("send total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  Serial.println("CAM send Done!");
  if (server.hasArg("ql")) {
    int ql = server.arg("ql").toInt();
    myCAM.OV2640_set_JPEG_size(ql);
    Serial.println("QL change to: " + server.arg("ql"));
  }
}

void serverStream() {
  WiFiClient client = server.client();

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (1) {
    start_capture();

    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

    size_t len = myCAM.read_fifo_length();
    if (len >= 393216) {
      Serial.println("Over size.");
      continue;
    } else if (len == 0 ) {
      Serial.println("Size is 0.");
      continue;
    }

    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    SPI.transfer(0xFF);

    if (!client.connected()) break;
    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);

    static const size_t bufferSize = 1024;
    static uint8_t buffer[bufferSize] = {0xFF};

    while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!client.connected()) break;
      client.write(&buffer[0], will_copy);
      len -= will_copy;
    }
    myCAM.CS_HIGH();

    if (!client.connected()) break;
  }
}

void handleNotFound() {
  
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);

  
}

void loop() {
  server.handleClient();
}
