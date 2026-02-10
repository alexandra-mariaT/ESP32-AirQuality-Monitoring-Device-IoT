#include <WiFi.h>
#include <PubSubClient.h> 
#include <ArduinoJson.h>  
#include <Wire.h> 
#include <Adafruit_BME280.h>
#include "MHZ19.h" 

// Network configuration
const char* ssid = "wifi_ssid";     
const char* password = "password";
const char* mqtt_server = "broker.hivemq.com"; 
const char* mqtt_topic = "esp32/sensor_data"; 

#define RX_PIN 4 
#define TX_PIN 5 

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BME280 bme;
MHZ19 myMHZ19; 
StaticJsonDocument<200> doc; 

void reconnect() {
  while (!client.connected()) 
  {
    Serial.print("Connecting to mqtt...");

    // clientId is unique
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) 
      Serial.println("Connected!");
    else 
    {
      Serial.println("Retrying connection in 5 seconds...");
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(2000);
    Serial.println("Connection was not successfull...");
  }

  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}


void setup() {
  Serial.begin(115200); 
  Wire.begin(21, 22); 
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  if (!bme.begin(0x76)) 
  {
    Serial.println("BME280 Error!");
    while (1);
  }

  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); 
  myMHZ19.begin(Serial2);
  myMHZ19.autoCalibration(false); 
  
  Serial.println("System initialized successfully!");
}

void loop() {
  if (!client.connected())
    reconnect();
  client.loop();

  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  int co2 = myMHZ19.getCO2();
  
  if (co2 < 400 || co2 > 5000) { 
     Serial.println("CO2 value out of range! Reading discarded.");
     co2 = 0; 
  }

  doc["temp"] = temp;
  doc["hum"] = hum;
  doc["co2"] = co2;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer); 
  
  client.publish(mqtt_topic, jsonBuffer);
  
  Serial.print("Published: ");
  Serial.println(jsonBuffer); 
  delay(5000); 
}