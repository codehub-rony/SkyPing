#include <DHT.h>
#include <WiFi.h>
#include <Ticker.h>
#include <AsyncMqtt_Generic.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "CHT8305.h"

#define uS_TO_S_FACTOR 1000000ULL // Conversion factor from microseconds to seconds
#define TIME_TO_SLEEP  600         // Time (in seconds) for ESP32-E to enter deep sleep
RTC_DATA_ATTR int bootCount = 0;    

#define TEMT6000 A0

#define BATTERY_LEVEL_PIN A3
#define ACTIVATE_SENSOR_PIN D13

// CHT8305 temperature and humidity sensor
CHT8305 CHT(0x40);  

float temp = 0.0;
float hum = 0.0;

#define MQTT_HOST IPAddress(192, 168, 2, 46)
#define MQTT_PORT 1883

unsigned long previousMillis = 0;  // Stores last time sensor reading was published
const long interval = 10000;       // Interval at which to publish sensor readings

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

const char* topic_skyping = "esp/skyping";

void print_wakeup_reason(){          
  esp_sleep_wakeup_cause_t wakeup_reason;  
  wakeup_reason = esp_sleep_get_wakeup_cause(); 

  switch(wakeup_reason) {             // Check the wake-up reason and print the corresponding message
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  connectToMqtt();
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  Serial.println("===== COLLECTING DATA ===== ");

  CHT.read();

  temp = CHT.getTemperature();
  hum = CHT.getHumidity();

  float volts = analogRead(TEMT6000) * 3.3 / 1024.0;        // Convert reading to VOLTS

  //Conversions from reading to LUX
  float amps = volts / 10000.0;      // em 10,000 Ohms
  float microamps = amps * 1000000;  // Convert to Microamps
  float lux = microamps * 2.0;       // Convert to Lux */
  Serial.println("----------------");
  Serial.print("LUX - ");
  Serial.print(lux);
  Serial.println(" lx");
  Serial.print(volts);
  Serial.println(" volts");
  Serial.println("----------------");

  DynamicJsonDocument doc(1024);
  doc["sensor_id"] = 1;
  doc["project_id"] = 1;
  doc["project_name"] = "sky-ping";
  doc["temperature"] = round(temp * 10.0) / 10.0;
  doc["humidity"] = round(hum * 10.0) / 10.0;
  doc["lux"] = round(lux);


  String jsonString;
  serializeJson(doc, jsonString);

  Serial.println("===== SENDING DATA ===== ");
  uint16_t packetIdPub1 = mqttClient.publish(topic_skyping, 1, true, jsonString.c_str());
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

  Serial.println("===== POWER OFF SENSORS ===== ");
  digitalWrite(ACTIVATE_SENSOR_PIN, LOW);
  Serial.println("===== GOING BACK TO SLEEP ===== ");
  esp_deep_sleep_start();   
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));   
    print_wakeup_reason();  
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);   

    Wire.begin();
    Wire.setClock(100000);
    delay(500);
    CHT.begin();
    delay(500);

    pinMode(ACTIVATE_SENSOR_PIN, OUTPUT);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);

    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    digitalWrite(ACTIVATE_SENSOR_PIN, HIGH);
    
    initWiFi();



    Serial.flush();

}

void loop() {

}
