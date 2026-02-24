#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "CHT8305.h"

#define uS_TO_S_FACTOR 1000000ULL // Conversion factor from microseconds to seconds
#define TIME_TO_SLEEP  900        // Time (in seconds) for ESP32-E to enter deep sleep
RTC_DATA_ATTR int bootCount = 0;

#define TEMT6000 A0

#define BATTERY_LEVEL_PIN A3
#define ACTIVATE_SENSOR_PIN D13

// CHT8305 temperature and humidity sensor
CHT8305 CHT(0x40);

const char* mqtt_server = "192.168.178.121";

unsigned long previousMillis = 0;  // Stores last time sensor reading was published
const long interval = 10000;       // Interval at which to publish sensor readings

WiFiClient espClient;
PubSubClient client(espClient);


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

//Helper functon for notifications using on board led
void blinkLed(uint8_t times, uint16_t onMs = 80, uint16_t offMs = 80) {
    for (uint8_t i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(onMs);
        digitalWrite(LED_BUILTIN, LOW);
        delay(offMs);
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
  digitalWrite(LED_BUILTIN, LOW);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    if (client.connect("ESP32-skyping")) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(ACTIVATE_SENSOR_PIN, OUTPUT);

    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Powering ON sensors");

    digitalWrite(ACTIVATE_SENSOR_PIN, HIGH);

    delay(1000);

    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    print_wakeup_reason();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    Serial.println("Setting up I2C for CHT sensor");
    Wire.begin();
    Wire.setClock(100000);
    delay(500);
    CHT.begin();
    delay(500);

    initWiFi();


    client.setServer(mqtt_server, MQTT_PORT);

}

void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  delay(2000);

  Serial.println("Reading sensors");

  CHT.begin();
  delay(200);
  if (!CHT.read()) {
      Serial.println("Sensor read failed");
  }

  float  temp = CHT.getTemperature();
  float  hum = CHT.getHumidity();

  Serial.println("----------------");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");
  Serial.println("----------------");

  DynamicJsonDocument doc(256);
  doc["sensor_id"] = 1;
  doc["temperature"] = round(temp * 10.0) / 10.0;
  doc["humidity"] = round(hum * 10.0) / 10.0;

  char payload[256];
  serializeJson(doc, payload);

  Serial.println("Publishing MQTT message:");
  Serial.println(payload);

  client.publish(topic_skyping, payload);

  Serial.println("Powering OFF sensors");
  digitalWrite(ACTIVATE_SENSOR_PIN, LOW);

  Serial.flush();

  esp_deep_sleep_start();

}
