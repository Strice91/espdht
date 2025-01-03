#include <WiFi.h>
#include <WiFiMulti.h>

#include <WebSocketsClient.h>  // include before MQTTPubSubClient.h
#include <MQTTPubSubClient.h>
#include <DHT.h>

#include "settings.h"
#include "secret.h"

// Inspired By
// https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-sensor-arduino-ide/
// https://xylem.aegean.gr/~modestos/mo.blog/esp32-send-dht-to-mqtt-and-deepsleep/
// https://www.instructables.com/Temperature-and-Humidity-Using-ESP32-DHT22-MQTT-My/
// https://www.elektormagazine.de/articles/prototyping-eines-energiezahlers-mit-esp32

// Libraries
// https://github.com/adafruit/DHT-sensor-library
// https://github.com/hideakitai/MQTTPubSubClient
// https://github.com/Links2004/arduinoWebSockets

DHT sensor(DHTPIN, DHTTYPE);
WiFiMulti wlan;
WebSocketsClient webSocket;
MQTTPubSubClient mqtt;

float h; //humidity
float t; //temperature
bool error = false;
char* T_TOPIC;
char* H_TOPIC;
char* D_TOPIC;

void setup_wifi() {
  delay(20);
  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.enableIPv6(true);

  Serial.print("[ INFO ] Try to connect to existing network '");
  Serial.print(SSID);
  Serial.print("' ");
  wlan.addAP(SSID, PASSWORD);
  
  do {
    delay(1000);
    Serial.print(".");
  } while (wlan.run() != WL_CONNECTED);

  Serial.print("\n[  OK  ] Connected to: '");
  Serial.print(WiFi.SSID());
  Serial.print("' | IP address: '");
  Serial.print(WiFi.localIP());
  Serial.println("'");
}

void webseocket_connect() {
  // Remove previous connection
  webSocket.disconnect();
  // connect to host with MQTT over WebSocket securely
  Serial.print("[ INFO ] Connecting to broker '");
  Serial.print(MQTTSERVER);
  Serial.print("' at port '");
  Serial.print(MQTTPORT);
  Serial.println("' via websocket.");
  webSocket.beginSslWithBundle(MQTTSERVER, MQTTPORT, "/", NULL, 0, "mqtt");
  webSocket.setReconnectInterval(5000);
  //webSocket.onEvent(webSocketEvent);
  Serial.println("[  OK  ] Websocket established!");
}

void mqtt_connect() {
  Serial.print("[ INFO ] Authenticating at MQTT broker as client '");
  Serial.print(HOSTNAME);
  Serial.print("' with username '");
  Serial.print(MQTTUSER);
  Serial.println("'");
  // initialize mqtt client
  mqtt.begin(webSocket);
  // connect to mqtt broker
  while (!mqtt.connect(HOSTNAME, MQTTUSER, MQTTPASS)){
    Serial.println("[ FAIL ] Retry in 5 seconds.");
    delay(5000);
  }
  mqtt.publish(D_TOPIC, "CONNECT");
  Serial.println("[  OK  ] Connected to MQTT broker!");
}

void read_sensor() {
  error = false; // Reset Error Flag
  h = sensor.readHumidity(); // Try to read humidity
  t = sensor.readTemperature(); // Try to read temperature
  error = isnan(t) || isnan(h); // Check if NAN reading
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  for(uint8_t t = 4; t > 0; t--) {
      Serial.printf("[ INIT ] BOOT WAIT %d...\n", t);
      Serial.flush();
      delay(1000);
  }

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
  webseocket_connect();
  mqtt_connect();
  sensor.begin();

  mqtt.publish(D_TOPIC, "SETUP");
  Serial.println("[ INFO ] Setup finished. Start main loop ...");
}

void loop() {
  mqtt.update(); // should be called https://github.com/hideakitai/MQTTPubSubClient/blob/main/examples/WiFiMQTToverWebSocketSecure/WiFiMQTToverWebSocketSecure.ino#L66

  if (!webSocket.isConnected()) {
    Serial.println("[ FAIL ] Lost Websocket Connection! Reconnecting ... ");
    webseocket_connect();
  }

  if (!mqtt.isConnected()) {
    Serial.println("[ FAIL ] Lost MQTT Connection! Reconnecting ... ");
    mqtt_connect();
  }

  read_sensor();
  if (error) {
    Serial.println("[ FAIL ] Please check the DHT sensor !");
    mqtt.publish(D_TOPIC, "ERROR");
  }
  else {
    Serial.print("[ INFO ] ");
    Serial.print(T_TOPIC);
    Serial.print(": ");
    Serial.print(t);
    Serial.print(" | ");
    Serial.print(H_TOPIC);
    Serial.print(": ");
    Serial.println(h);
    mqtt.publish(T_TOPIC, String(t).c_str());
    mqtt.publish(H_TOPIC, String(h).c_str());
  }

  delay(DHTINTERVAL);
}
