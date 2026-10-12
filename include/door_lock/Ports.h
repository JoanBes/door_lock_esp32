#pragma once

#include <string>

class RfidSourcePort {
 public:
  virtual ~RfidSourcePort() = default;
  virtual bool readUid(std::string& uid) = 0;
};

class DoorOutputPort {
 public:
  virtual ~DoorOutputPort() = default;
  virtual void open() = 0;
  virtual void close() = 0;
};

class MessageBusPort {
 public:
  virtual ~MessageBusPort() = default;
  virtual bool publish(const std::string& topic, const std::string& message) = 0;
};

class MqttMessageHandler {
 public:
  virtual ~MqttMessageHandler() = default;
  virtual void onMqttMessage(const std::string& topic,
                             const std::string& message) = 0;
};