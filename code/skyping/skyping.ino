#include <DHT.h>
#include <WiFi.h>
#include <Ticker.h>
#include <AsyncMqtt_Generic.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define TEMT6000 A0
#define DHTPIN D1

#define MQTT_HOST IPAddress(192, 168, 2, 46)
#define MQTT_PORT 1883

//DHT22
#define DHTTYPE    DHT22
DHT dht(DHTPIN, DHTTYPE);

float temp = 0.0;
float hum = 0.0;

#define LED D2

unsigned long previousMillis = 0;  // Stores last time sensor reading was published
const long interval = 10000;       // Interval at which to publish sensor readings

// WiFiEventHandler wifiConnectHandler;
// WiFiEventHandler wifiDisconnectHandler;
// Ticker wifiReconnectTimer;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

const char* topic_skyping = "esp/skyping";

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

// void connectToWifi() {
//   Serial.println("Connecting to Wi-Fi...");
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
// }

// void onWifiConnect(const WiFiEventStationModeGotIP& event) {
//   Serial.println("Connected to Wi-Fi.");
//   connectToMqtt();
// }

// void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
//   Serial.println("Disconnected from Wi-Fi.");
//   mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
//   wifiReconnectTimer.once(2, connectToWifi);
// }

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  digitalWrite(LED, HIGH);
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
    Serial.begin(9600);
    dht.begin();

    pinMode(LED, OUTPUT);

    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);

    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  
    connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    temp = dht.readTemperature();
    hum = dht.readHumidity();

    Serial.printf("new temperature reading: %.2f\n", temp);
    Serial.printf("new humidity reading: %.2f\n", hum);

    float volts =  analogRead(TEMT6000) * 3.3 / 1024.0; // Convert reading to VOLTS
    float VoltPercent = analogRead(TEMT6000) / 1024.0 * 100; //Reading to Percent of Voltage

    //Conversions from reading to LUX
    float amps = volts / 10000.0;  // em 10,000 Ohms
    float microamps = amps * 1000000; // Convert to Microamps
    float lux = microamps * 2.0; // Convert to Lux */
    Serial.print("LUX - ");
    Serial.print(lux);
    Serial.println(" lx");
    Serial.print(VoltPercent);
    Serial.println("%");
    Serial.print(volts);
    Serial.println(" volts");
    Serial.print(amps);
    Serial.println(" amps");
    Serial.print(microamps);
    Serial.println(" microamps");
    Serial.println(" ----------------");

    DynamicJsonDocument doc(1024);
    doc["sensor_id"] = 1;
    doc["project_id"] = 1;
    doc["project_name"] = "sky-ping";
    doc["temperature"] = temp;
    doc["humidity"] = hum;
    doc["lux"] = round(lux);
    doc["lux_volt_percent"] = VoltPercent;

    String jsonString;
    serializeJson(doc, jsonString);


    uint16_t packetIdPub1 = mqttClient.publish(topic_skyping, 1, true, jsonString.c_str());
  }

}
