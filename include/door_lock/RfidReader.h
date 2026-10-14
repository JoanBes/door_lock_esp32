#pragma once

#include <MFRC522.h>
#include <SPI.h>

#include "Ports.h"

class RfidReader : public RfidSourcePort {
 public:
  RfidReader(byte chipSelectPin, byte resetPin)
      : reader_(chipSelectPin, resetPin) {}

  void begin() {
    SPI.begin();
    reader_.PCD_Init();
  }

  bool readUid(std::string& uid) override {
    if (!reader_.PICC_IsNewCardPresent() || !reader_.PICC_ReadCardSerial()) {
      return false;
    }

    uid.clear();
    for (byte index = 0; index < reader_.uid.size; ++index) {
      if (reader_.uid.uidByte[index] < 0x10) {
        uid += "0";
      }
      uid += String(reader_.uid.uidByte[index], HEX).c_str();
    }
    for (char& character : uid) {
      character = static_cast<char>(std::toupper(character));
    }
    Serial.print("RFID read: ");
    Serial.println(uid.c_str());
    reader_.PICC_HaltA();
    reader_.PCD_StopCrypto1();
    return true;
  }

 private:
  MFRC522 reader_;
};