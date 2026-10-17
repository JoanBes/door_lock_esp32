#pragma once

#include <PubSubClient.h>

#include "Ports.h"

class MqttService : public MessageBusPort {
 public:
  MqttService(Client& networkClient, const char* host, uint16_t port,
              const char* clientId, const char* username, const char* password,
              const char* commandTopic, const char* statusTopic)
      : mqtt_(networkClient),
        host_(host),
        port_(port),
        clientId_(clientId),
        username_(username),
        password_(password),
        commandTopic_(commandTopic),
        statusTopic_(statusTopic) {}

  void setHandler(MqttMessageHandler& handler) { handler_ = &handler; }

  void begin() {
    activeService() = this;
    mqtt_.setServer(host_, port_);
    mqtt_.setCallback(callbackThunk);
  }

  static void callbackThunk(char* topic, byte* payload, unsigned int length) {
    if (activeService() == nullptr) {
      return;
    }
    activeService()->handleMessage(topic, payload, length);
  }

  void maintainConnection() {
    if (mqtt_.connected()) {
      return;
    }
    while (!mqtt_.connected()) {
      const bool hasCredentials = username_ != nullptr && username_[0] != '\0';
      const bool connected = hasCredentials
                                 ? mqtt_.connect(clientId_, username_, password_,
                                                 statusTopic_, 1, true, "offline")
                                 : mqtt_.connect(clientId_, statusTopic_, 1, true,
                                                 "offline");
      if (connected) {
        Serial.println("MQTT CONNECT: OK");
        const bool statusPublished =
            mqtt_.publish(statusTopic_, "online", true);
        Serial.print("MQTT TX [");
        Serial.print(statusTopic_);
        Serial.print("]: ");
        Serial.println(statusPublished ? "OK" : "FAIL");

        const bool subscribed = mqtt_.subscribe(commandTopic_);
        Serial.print("MQTT SUBSCRIBE [");
        Serial.print(commandTopic_);
        Serial.print("]: ");
        Serial.println(subscribed ? "OK" : "FAIL");
      } else {
        Serial.print("MQTT CONNECT: FAIL, state=");
        Serial.println(mqtt_.state());
        delay(5000);
      }
    }
  }

  void loop() { mqtt_.loop(); }

  bool publish(const std::string& topic, const std::string& message) override {
    const bool published = mqtt_.publish(topic.c_str(), message.c_str());
    Serial.print("MQTT TX [");
    Serial.print(topic.c_str());
    Serial.print("]: ");
    Serial.println(published ? "OK" : "FAIL");
    return published;
  }

 private:
  void handleMessage(char* topic, byte* payload, unsigned int length) {
    Serial.print("MQTT RX [");
    Serial.print(topic);
    Serial.print("] length=");
    Serial.println(length);

    String message;
    for (unsigned int index = 0; index < length; ++index) {
      message += static_cast<char>(payload[index]);
    }
    if (handler_ != nullptr) {
      handler_->onMqttMessage(topic, message.c_str());
    }
  }

  static MqttService*& activeService() {
    static MqttService* service = nullptr;
    return service;
  }

  PubSubClient mqtt_;
  const char* host_;
  uint16_t port_;
  const char* clientId_;
  const char* username_;
  const char* password_;
  const char* commandTopic_;
  const char* statusTopic_;
  MqttMessageHandler* handler_ = nullptr;
};