#include <DHT.h>
#include <ESP8266WiFi.h>
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

unsigned long previousMillis = 0;  // Stores last time sensor reading was published
const long interval = 10000;       // Interval at which to publish sensor readings

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  // connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  // mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void setup() {
    Serial.begin(9600);
    dht.begin();
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
    connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    temp = dht.readTemperature();
    hum = dht.readHumidity();

    // DynamicJsonDocument doc(1024);
    // doc["sensor_id"] = 1;
    // doc["temperature"] = temp;
    // doc["humidity"] = hum;

    // String jsonString;
    // serializeJson(doc, jsonString);

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
  }
}
