#include<string.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h> 

const char* ssid = "";
const char* password = "";


#define TRIGGER 0
#define ECHO    2 


char server[] = "52.204.73.21";
char topic[] = "iot-2/evt/display/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = "token";
char clientId[] = "thatislinux";

WiFiClient wifiClient;
PubSubClient client(server, 1883, NULL, wifiClient);

// NodeMCU Pin D1 > TRIGGER | Pin D2 > ECHO   PINOUT for ESP-12E

void setup() {
  
          Serial.begin(115200);
          pinMode(TRIGGER, OUTPUT);
          pinMode(ECHO, INPUT);
          pinMode(BUILTIN_LED, OUTPUT);
          Serial.println();

    
          Serial.print("Connecting to ");
          
          WiFi.begin(ssid, password);
          
          while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            } 
          Serial.println("");
          Serial.print("WiFi connected, IP address: "); Serial.println(WiFi.localIP());
 }

void loop() {
  if (!client.connected()) {
     Serial.print("Reconnecting client to ");
     Serial.println(server);
     Serial.println(clientId);
     while (!client.connect(clientId, authMethod, token)) {
       Serial.print(".");
       delay(500);
     }
     Serial.println();
   }
  
  long duration, distance;
  digitalWrite(TRIGGER, LOW);  
  delayMicroseconds(2); 
  
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10); 
  
  digitalWrite(TRIGGER, LOW);
  duration = pulseIn(ECHO, HIGH);
  distance = (duration/2) / 29.1;
  
  Serial.print("Distance(cm): ");
  Serial.print(distance);
  Serial.println();

  String payload = "{\"distance\":";
  payload += distance;
  payload += "}";
  

  Serial.print("Sending payload: ");
  Serial.println(payload);
  if (client.publish(topic, (char*) payload.c_str())) {
     Serial.println("Publish ok");
     } 
   else
     {
     Serial.println("Publish failed");
     }
  delay(2000);
}
