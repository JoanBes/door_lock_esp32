#include <unity.h>

#include <string>

#include "door_lock/AccessControllerLogic.h"
#include "door_lock/AccessPolicy.h"

namespace {
constexpr char RFID_TOPIC[] = "doorlock/rfid";
constexpr char COMMAND_TOPIC[] = "doorlock/command";
constexpr char RELEASED_TOPIC[] = "doorlock/released";

class FakeReader : public RfidSourcePort {
 public:
  bool readUid(std::string& uid) override {
    if (nextUid.empty()) {
      return false;
    }
    uid = nextUid;
    nextUid.clear();
    return true;
  }
  std::string nextUid;
};

class FakeDoor : public DoorOutputPort {
 public:
  void open() override { openCount++; }
  void close() override { closeCount++; }
  int openCount = 0;
  int closeCount = 0;
};

class FakeBus : public MessageBusPort {
 public:
  bool publish(const std::string& topic, const std::string& message) override {
    lastTopic = topic;
    lastMessage = message;
    publishCount++;
    return true;
  }
  int publishCount = 0;
  std::string lastTopic;
  std::string lastMessage;
};
}

void test_normalize_code_removes_separators_and_uppercases() {
  TEST_ASSERT_EQUAL_STRING(
      "A1B2C3",
      door_lock::AccessPolicy::normalizeCode("a1:b2-c3").c_str());
}

void test_derives_stable_code_from_rfid_uid() {
  TEST_ASSERT_EQUAL_STRING(
      door_lock::AccessPolicy::deriveAccessCode("a1:b2-c3").c_str(),
      door_lock::AccessPolicy::deriveAccessCode("A1B2C3").c_str());
  TEST_ASSERT_TRUE(door_lock::AccessPolicy::deriveAccessCode("A1B2C3") !=
                   door_lock::AccessPolicy::deriveAccessCode("D4E5F6"));
}

void test_controller_publishes_rfid_and_unlocks_only_with_derived_code() {
  FakeReader reader;
  FakeDoor door;
  FakeBus bus;
  AccessControllerLogic controller(reader, door, bus, RFID_TOPIC,
                                   COMMAND_TOPIC, RELEASED_TOPIC);
  reader.nextUid = "A1B2C3";
  controller.update();
  TEST_ASSERT_EQUAL(1, bus.publishCount);
  TEST_ASSERT_EQUAL_STRING(RFID_TOPIC, bus.lastTopic.c_str());
  TEST_ASSERT_EQUAL_STRING("A1B2C3", bus.lastMessage.c_str());

    controller.onMqttMessage(COMMAND_TOPIC, "WRONG");
    TEST_ASSERT_EQUAL(0, door.openCount);
    controller.onMqttMessage(
      COMMAND_TOPIC, door_lock::AccessPolicy::deriveAccessCode("A1B2C3"));
    TEST_ASSERT_EQUAL(1, door.openCount);
}

  void test_controller_closes_with_same_rfid_and_notifies_server() {
  FakeReader reader;
  FakeDoor door;
  FakeBus bus;
  AccessControllerLogic controller(reader, door, bus, RFID_TOPIC,
                     COMMAND_TOPIC, RELEASED_TOPIC);
    reader.nextUid = "A1B2C3";
  controller.update();
    controller.onMqttMessage(
      COMMAND_TOPIC, door_lock::AccessPolicy::deriveAccessCode("A1B2C3"));
    reader.nextUid = "A1B2C3";
    controller.update();
    TEST_ASSERT_EQUAL(1, door.closeCount);
    TEST_ASSERT_EQUAL_STRING(RELEASED_TOPIC, bus.lastTopic.c_str());
    TEST_ASSERT_EQUAL_STRING(
      door_lock::AccessPolicy::deriveAccessCode("A1B2C3").c_str(),
      bus.lastMessage.c_str());
  }

  void test_controller_keeps_open_for_other_rfid() {
    FakeReader reader;
    FakeDoor door;
    FakeBus bus;
    AccessControllerLogic controller(reader, door, bus, RFID_TOPIC,
                     COMMAND_TOPIC, RELEASED_TOPIC);
    reader.nextUid = "A1B2C3";
    controller.update();
    controller.onMqttMessage(
      COMMAND_TOPIC, door_lock::AccessPolicy::deriveAccessCode("A1B2C3"));
    reader.nextUid = "D4E5F6";
    controller.update();
    TEST_ASSERT_EQUAL(0, door.closeCount);
    TEST_ASSERT_EQUAL(1, door.openCount);
}

void test_repeated_command_does_not_open_door_again() {
  FakeReader reader;
  FakeDoor door;
  FakeBus bus;
  AccessControllerLogic controller(reader, door, bus, RFID_TOPIC,
                                   COMMAND_TOPIC, RELEASED_TOPIC);
  reader.nextUid = "A1B2C3";
  controller.update();
  const std::string code =
      door_lock::AccessPolicy::deriveAccessCode("A1B2C3");
  controller.onMqttMessage(COMMAND_TOPIC, code);
  controller.onMqttMessage(COMMAND_TOPIC, code);
  TEST_ASSERT_EQUAL(1, door.openCount);
  TEST_ASSERT_EQUAL(0, door.closeCount);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normalize_code_removes_separators_and_uppercases);
  RUN_TEST(test_derives_stable_code_from_rfid_uid);
  RUN_TEST(test_controller_publishes_rfid_and_unlocks_only_with_derived_code);
  RUN_TEST(test_controller_closes_with_same_rfid_and_notifies_server);
  RUN_TEST(test_controller_keeps_open_for_other_rfid);
  RUN_TEST(test_repeated_command_does_not_open_door_again);
  return UNITY_END();
}