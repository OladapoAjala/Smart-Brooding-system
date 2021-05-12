#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

extern "C" {
#include <espnow.h>
#include "user_interface.h"
}

//Call prototypes for MQTT
void myPublishedCb();
void myDisconnectedCb();
void myConnectedCb();

//// create MQTT object
MQTT myMqtt("Smart_Brooder", "test.mosquitto.org", 1883);
const char* ssid     = "Kolade";
const char* password = "1441mf3h";
String heater_state = "0";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define WIFI_CHANNEL 1

uint8_t ap_mac_address[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
uint8_t sta_mac_address[] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAB };
uint8_t actuator_1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xA1};


void initVariant() {
  WiFi.mode(WIFI_AP_STA);
  wifi_set_macaddr(STATION_IF, &sta_mac_address[0]);
  wifi_set_macaddr(SOFTAP_IF, &ap_mac_address[0]);
}

// keep in sync with ESP_NOW sensor struct
struct __attribute__((packed)) SENSOR_DATA {
  String sensor_name = "N_1";
  float temp = 0;
  float humidity = 0;
  bool state = true;
} sensorData;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 'OAU_logo', 128x64px
const unsigned char OAU_logo [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x57, 0xfb, 0xb5, 0x55, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0xaa, 0xba, 0xbf, 0xf5, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x8f, 0xff, 0xff, 0xfa, 0xbc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x7f, 0xc1, 0xc0, 0xff, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x40, 0x1f, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0xf8, 0x08, 0x4a, 0x07, 0xe4, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x18, 0x44, 0x07, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0xd0, 0x60, 0x00, 0x05, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0xd0, 0x00, 0x00, 0x05, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0, 0x09, 0x4a, 0x05, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xd0, 0x11, 0x46, 0x45, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0, 0x41, 0xc0, 0x85, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0xd0, 0x05, 0xc0, 0x05, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd1, 0x00, 0xc0, 0xc5, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0xd8, 0x01, 0xcc, 0x45, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0, 0x07, 0xf8, 0x05, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0xd5, 0xe2, 0xfb, 0xdd, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0xc1, 0xcf, 0xd9, 0xc1, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0xcf, 0xfd, 0x1f, 0xfc, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x07, 0xf0, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0xff, 0xff, 0x3f, 0xff, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xfb, 0xcf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0xfa, 0xcf, 0xfb, 0xd5, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x55, 0xbc, 0x1c, 0xd7, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xf5, 0xe0, 0x07, 0xea, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0c, 0x0d, 0xab, 0xc0, 0x03, 0xeb, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x2e, 0x0f, 0x7b, 0x80, 0x01, 0xf5, 0xe8, 0x38, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x8d, 0x0f, 0xd7, 0x80, 0x00, 0xf7, 0xb8, 0x62, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x18, 0x0e, 0xf7, 0x80, 0x00, 0xfa, 0xf8, 0xb0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xe0, 0x8f, 0xaf, 0x8c, 0x18, 0xfb, 0x79, 0x0d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x10, 0x4f, 0xef, 0x8c, 0x18, 0xfd, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x7c, 0x47, 0xde, 0x8c, 0x38, 0xfd, 0xfa, 0x08, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x03, 0x27, 0xde, 0x84, 0x10, 0xbf, 0xf8, 0x7c, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x01, 0x27, 0xff, 0x00, 0x00, 0xff, 0xf4, 0xc1, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x27, 0xff, 0x02, 0x20, 0xff, 0xf4, 0x80, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0c, 0x23, 0xff, 0x83, 0xc0, 0xff, 0xf0, 0x00, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xad, 0xc3, 0xff, 0x80, 0x00, 0xff, 0xe4, 0xa8, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x88, 0x83, 0xff, 0x80, 0x21, 0xff, 0xe1, 0x25, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x4e, 0x81, 0xff, 0x85, 0x41, 0xff, 0xc1, 0x3d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x58, 0x01, 0xff, 0xc3, 0xc1, 0xff, 0xc1, 0x60, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x30, 0x40, 0xff, 0xc1, 0x81, 0xff, 0x82, 0x3a, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x82, 0xe0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x03, 0xa0, 0x7f, 0xe0, 0x03, 0xff, 0x04, 0xf4, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x15, 0x90, 0x3f, 0xd0, 0x01, 0xfe, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0a, 0x88, 0x3f, 0xe0, 0x03, 0xfc, 0x0b, 0x28, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x24, 0x1f, 0xf1, 0x0b, 0xf8, 0x14, 0xd0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x04, 0x62, 0x07, 0xc0, 0x13, 0xf0, 0x26, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x81, 0x03, 0xe4, 0x37, 0xc0, 0x41, 0x20, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x80, 0xe4, 0x37, 0x00, 0x90, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x5a, 0x60, 0x38, 0x1c, 0x01, 0x3a, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xa5, 0x10, 0x0f, 0xe0, 0x04, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x06, 0x00, 0x00, 0x13, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x70, 0xc0, 0x00, 0xc0, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x5c, 0x1c, 0x0e, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xbd, 0x00, 0x00, 0x52, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x2e, 0xca, 0x16, 0xd8, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0xfd, 0x3b, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x73, 0x29, 0xb6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x55, 0x2a, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void setup() {
  Serial.begin(115200); Serial.println();
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Connecting to WiFi..");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
  }

  delay(1000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Connected!");
  display.display();

  Serial.print("This node AP mac: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("This node STA mac: "); Serial.println(WiFi.macAddress());

  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Setting up MQTT!");
  display.display();

  //setup callbacks
  myMqtt.onConnected(myConnectedCb);
  myMqtt.onDisconnected(myDisconnectedCb);
  myMqtt.onPublished(myPublishedCb);
  myMqtt.onData(myDataCb);

  Serial.println("connect mqtt...");
  myMqtt.connect();
  myMqtt.subscribe("/agrilab/switch");

  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Connected to Broker!");
  display.display();

  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Initialize ESP NOW!");
  display.display();

  initEspNow();

  delay(2000);
  display.clearDisplay();
  display.drawBitmap(0, 0, OAU_logo, 128, 64, WHITE);
  display.display();
  delay(3000);
}

void loop() {

    if (sensorData.sensor_name == "N_1") {
      myMqtt.publish("/agrilab/nodeone/temperature", temperature);
      myMqtt.publish("/agrilab/nodeone/humidity", humidity);
    } else if (sensorData.sensor_name == "N_2") {
      myMqtt.publish("/agrilab/nodetwo/temperature", temperature);
      myMqtt.publish("/agrilab/nodetwo/humidity", humidity);
    } else if (sensorData.sensor_name == "N_3") {
      myMqtt.publish("/agrilab/nodethree/temperature", temperature);
      myMqtt.publish("/agrilab/nodethree/humidity", humidity);
    } else if (sensorData.sensor_name == "N_4") {
      myMqtt.publish("/agrilab/nodefour/temperature", temperature);
      myMqtt.publish("/agrilab/nodefour/humidity", humidity);
    } else if (sensorData.sensor_name == "N_5") {
      myMqtt.publish("/agrilab/nodefive/temperature", temperature);
      myMqtt.publish("/agrilab/nodefive/humidity", humidity);
    }
    print_temp(sensorData.temp, sensorData.humidity);
    delay(2000);
}

void initEspNow() {
  if (esp_now_init() != 0) {
    Serial.println("*** ESP_Now initialization failed ***");
    ESP.restart();
  }
  Serial.println(sizeof(sensorData));

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_add_peer(actuator_1_mac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);
  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {

    memcpy(&sensorData, data, sizeof(sensorData));

    Serial.printf("Received data from: ");
    Serial.println(sensorData.sensor_name);
    Serial.printf("Temperature = %0.3f\n"
                  "Humidity = %0.3f% \n",
                  sensorData.temp,
                  sensorData.humidity);

    {
      if (sensorData.temp < 28) {
        sensorData.state = true;
        heater_state = "1";

        uint8_t data[sizeof(sensorData)];
        memcpy(data, &sensorData, sizeof(sensorData));

        esp_now_send(actuator_1_mac, data, sizeof(sensorData));
      } else {
        sensorData.state = false;
        heater_state = "0";

        uint8_t data[sizeof(sensorData)];
        memcpy(data, &sensorData.state, sizeof(sensorData));
        esp_now_send(actuator_1_mac, data, sizeof(sensorData));
      }
      myMqtt.publish("/agrilab/switch", heater_state);
    }
  });
  esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
    Serial.println("Data sent!");
  });
}

void print_temp (float temperature, float humidity)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(sensorData.sensor_name);
  display.print(" Temp: ");
  display.setTextSize(3);
  display.setCursor(0, 30);
  display.print(temperature);
  display.setTextSize(2);
  display.cp437(true);
  display.write(167);
  display.setTextSize(3);
  display.print("C");
  display.display();

  delay(5000);
  //display humidity
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(sensorData.sensor_name);
  display.print(" Hum:");
  display.setTextSize(3);
  display.setCursor(0, 30);
  display.print(humidity);
  display.print(" %");

  display.display();
}

void myConnectedCb()
{
  Serial.println("connected to MQTT server");
}

void myDisconnectedCb()
{
  Serial.println("disconnected. try to reconnect...");
  delay(500);
  myMqtt.connect();
}

void myPublishedCb()
{
  Serial.println("published.");
}

void myDataCb(String& topic, String& data)
{
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(data);

  if (topic == "/agrilab/switch") {
    heater_state = data;
  }
}
