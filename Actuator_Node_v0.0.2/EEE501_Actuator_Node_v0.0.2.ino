#include <ESP8266WiFi.h>

extern "C" {
#include <espnow.h>
}

#define ACTUATOR_PIN 4

uint8_t actuator_1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xA1};

void initVariant() {
  WiFi.mode(WIFI_AP);
  wifi_set_macaddr(SOFTAP_IF, &actuator_1_mac[0]);
}

struct __attribute__((packed)) SENSOR_DATA {
  String sensor_name;
  float temp;
  float humidity;
  bool state;
} sensorData;

void setup() {
  pinMode(ACTUATOR_PIN, OUTPUT);
  
  Serial.begin(115200);

  if (esp_now_init() != 0) {
    Serial.println("*** ESP_Now initialization failed ***");
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {

    memcpy(&sensorData, data, sizeof(sensorData));

    Serial.println("Received data from control node");
    Serial.printf("Switch State: ");
    Serial.println(sensorData.state);

    if(sensorData.state) {
      digitalWrite(ACTUATOR_PIN, HIGH);
    } else {
      digitalWrite(ACTUATOR_PIN, LOW);
    }
  });

}

void loop() {

}
