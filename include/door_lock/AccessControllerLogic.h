#pragma once

#include <string>
#include <utility>

#include "AccessPolicy.h"
#include "Ports.h"

class AccessControllerLogic : public MqttMessageHandler {
 public:
  AccessControllerLogic(RfidSourcePort& reader, DoorOutputPort& door,
                        MessageBusPort& bus, std::string rfidTopic,
                        std::string commandTopic, std::string releasedTopic)
      : reader_(reader),
        door_(door),
        bus_(bus),
        rfidTopic_(std::move(rfidTopic)),
        commandTopic_(std::move(commandTopic)),
        releasedTopic_(std::move(releasedTopic)) {}

  void update() {
    std::string uid;
    if (!reader_.readUid(uid) || uid.empty()) {
      return;
    }

    const std::string normalizedUid = door_lock::AccessPolicy::normalizeCode(uid);
    if (!openUid_.empty()) {
      if (normalizedUid == openUid_) {
        door_.close();
        bus_.publish(releasedTopic_,
                     door_lock::AccessPolicy::deriveAccessCode(openUid_));
        openUid_.clear();
      }
      return;
    }

    pendingUid_ = normalizedUid;
    // The server uses this UID to calculate the response code.
    bus_.publish(rfidTopic_, pendingUid_);
  }

  void onMqttMessage(const std::string& topic,
                     const std::string& message) override {
    if (topic != commandTopic_ || pendingUid_.empty()) {
      return;
    }
    if (door_lock::AccessPolicy::isValidAccessCode(message, pendingUid_)) {
      door_.open();
      openUid_ = pendingUid_;
      pendingUid_.clear();
    }
  }

 private:
  RfidSourcePort& reader_;
  DoorOutputPort& door_;
  MessageBusPort& bus_;
  std::string rfidTopic_;
  std::string commandTopic_;
  std::string releasedTopic_;
  std::string pendingUid_;
  std::string openUid_;
};