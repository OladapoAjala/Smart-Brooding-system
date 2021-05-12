#include <ESP8266WiFi.h>
#include "DHT.h"
extern "C" {
#include <espnow.h>
}

#define WIFI_CHANNEL 1
#define DHTPIN 13
#define DHTTYPE DHT11
#define POWER_PIN 12

uint8_t control_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

struct __attribute__((packed)) SENSOR_DATA {
  String name = "N_1";
  float temp;
  float humidity;
} sensorData;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  pinMode(POWER_PIN, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("*** ESP_Now initialization failed***");
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(control_mac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);
  esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
    if (sendStatus) {
      Serial.printf("Data sent to %d\n", mac);
    }
  });
  dht.begin();
}

void loop() {
  read_sensor();
  ESP.deepSleep(10e6);
}

void read_sensor(void) {
  digitalWrite(POWER_PIN, HIGH);
  delay(2000);
  sensorData.temp = dht.readTemperature();
  sensorData.humidity = dht.readHumidity();

  if (isnan(sensorData.temp) || isnan(sensorData.humidity)) {
    ESP.restart();
  }
  
  uint8_t data[sizeof(sensorData)];
  memcpy(data, &sensorData, sizeof(sensorData));
  esp_now_send(control_mac, data, sizeof(sensorData));
  digitalWrite(POWER_PIN, LOW);
}
