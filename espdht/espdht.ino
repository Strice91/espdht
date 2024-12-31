#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#include "settings.h"
#include "secret.h"

// Inspired By
// https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-sensor-arduino-ide/
// https://xylem.aegean.gr/~modestos/mo.blog/esp32-send-dht-to-mqtt-and-deepsleep/
// https://www.instructables.com/Temperature-and-Humidity-Using-ESP32-DHT22-MQTT-My/

DHT sensor(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

float h; //humidity
float t; //temperature
bool error = false;
char* T_TOPIC;
char* H_TOPIC;
char* D_TOPIC;


void setup() {
  Serial.begin(115200);
  Serial.println(F("DHT22 test!"));

  size_t topic_len = strlen(HOSTNAME) + strlen("/temperature") + 1;
  T_TOPIC = new char[topic_len];
  snprintf(T_TOPIC, topic_len, "%s/temperature", HOSTNAME);

  topic_len = strlen(HOSTNAME) + strlen("/humidity") + 1; 
  H_TOPIC = new char[topic_len];
  snprintf(H_TOPIC, topic_len, "%s/humidity", HOSTNAME);

  topic_len = strlen(HOSTNAME) + strlen("/debug") + 1; 
  D_TOPIC = new char[topic_len];
  snprintf(D_TOPIC, topic_len, "%s/debug", HOSTNAME);

  setup_wifi();
  sensor.begin();


  
  client.setServer(MQTTSERVER, MQTTPORT);
  if (!client.connected()) {
    //mqtt_connect();
  }
}

void mqtt_connect() {
  while (!client.connected()){
    Serial.print("Connecting to MQTT broker ");
    Serial.println(MQTTSERVER);
    bool result = client.connect(HOSTNAME, MQTTUSER, MQTTPASSWORD);
    if(result){
      Serial.println("OK");
    }
    else {
      Serial.print("[Error] Not connected: ");
      Serial.print(client.state());
      Serial.println(" Wait 5 seconds before retry.");
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(20);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("\n\nTry to connect to existing network");
  
  uint8_t timeout = 20;
  do {
    delay(500);
    Serial.print(".");
    timeout--;
  } while (timeout && WiFi.status() != WL_CONNECTED);

  Serial.print("\nIP address: ");
  Serial.println(WiFi.localIP());
}

void read_sensor() {
  error = false; // Reset Error Flag
  h = sensor.readHumidity(); // Try to read humidity
  t = sensor.readTemperature(); // Try to read temperature
  error = isnan(t) || isnan(h); // Check if NAN reading
}

void loop() {
  delay(2000);
  Serial.println(T_TOPIC);
  Serial.println(H_TOPIC);
  //Serial.println(DEBUGTOPIC);
  read_sensor();
  if (error) {
    Serial.println("[ERROR] Please check the DHT sensor !");
  }
  else {
    Serial.print("Temperature : ");
    Serial.print(t);
    Serial.print(" | Humidity : ");
    Serial.println(h);
  }
}
