#include <PubSubClient.h>
#include <SPI.h>
#include <ESP8266WiFi.h>


 
const char* ssid = "";
const char* password = "";
 
char* topic = "#";    
char* server = "";  //MQTT BROKER IP
 
char message_buff[100];  
 
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);
 
void callback(char* topic, byte* payload, unsigned int length) {
  
    int i = 0;

  Serial.println("Received Topic: " + String(topic));

  
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  
  String msgString = String(message_buff);
  
  Serial.println("PayLoadString: " + msgString);
  int state = digitalRead(2);  // get the current state of GPIO1 pin
  if (msgString == "1"){    // if there is a "1" published to any topic (#) on the broker then:
    digitalWrite(2, !state);     // set pin to the opposite state 
    Serial.println("Switching LED"); 
  }
} 
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
 
void setup() {
  Serial.begin(115200);
  delay(10);
  
   
  pinMode(2, OUTPUT);   
  digitalWrite(2, 0);  
  
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 
  //  connection to broker script.
  if (client.connect("arduinoClient")) {
    client.publish("outTopic","hello world to MQTT BROKER");
    client.subscribe("CPUUTIL");   //Name of the Topic to be set in MQTT Broker Server for Receiving messages
  }
}
 
void loop() {

  client.loop();
}


