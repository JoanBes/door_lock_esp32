#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "door_lock/AccessControllerLogic.h"
#include "door_lock/DoorActuator.h"
#include "door_lock/MqttService.h"
#include "door_lock/RfidReader.h"
#include "door_lock/config.h"

namespace {
constexpr char DEVICE_ID[] = "ESP32_Door_Lock_01";
constexpr char RFID_TOPIC[] = "doorlockJBL/01/rfid";
constexpr char COMMAND_TOPIC[] = "doorlockJBL/01/command";
constexpr char RELEASED_TOPIC[] = "doorlockJBL/01/released";
constexpr char STATUS_TOPIC[] = "doorlockJBL/01/status";

constexpr byte RFID_SS_PIN = 5;
constexpr byte RFID_RST_PIN = 4;
constexpr byte SERVO_PIN = 16;
constexpr byte LED_PIN = 2;
constexpr int LOCKED_ANGLE = 0;
constexpr int UNLOCKED_ANGLE = 90;

WiFiClientSecure wifiClient;
RfidReader rfidReader(RFID_SS_PIN, RFID_RST_PIN);
DoorActuator doorActuator(SERVO_PIN, LED_PIN, LOCKED_ANGLE, UNLOCKED_ANGLE);
MqttService mqttService(wifiClient, MQTT_HOST, MQTT_PORT, DEVICE_ID, MQTT_USER,
                        MQTT_PASSWORD, COMMAND_TOPIC, STATUS_TOPIC);
AccessControllerLogic doorLock(rfidReader, doorActuator, mqttService,
                               RFID_TOPIC, COMMAND_TOPIC, RELEASED_TOPIC);
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  const unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
    delay(500);
    Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" connected, IP=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print(" FAILED, status=");
    Serial.println(WiFi.status());
  }
}

void setup() {
  Serial.begin(9600);
  delay(10000);
  connectToWifi();
  wifiClient.setInsecure();
  mqttService.setHandler(doorLock);
  mqttService.begin();
  rfidReader.begin();
  doorActuator.begin();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
    delay(1000);
    return;
  }
  mqttService.maintainConnection();
  mqttService.loop();
  doorLock.update();
}