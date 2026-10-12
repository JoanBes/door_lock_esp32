#pragma once

#include <cctype>
#include <cstdint>
#include <string>

namespace door_lock {

class AccessPolicy {
 public:
  static std::string normalizeCode(const std::string& code) {
    std::string normalized;
    normalized.reserve(code.size());
    for (unsigned char character : code) {
      if (std::isalnum(character)) {
        normalized.push_back(static_cast<char>(std::toupper(character)));
      }
    }
    return normalized;
  }

  static std::string deriveAccessCode(const std::string& rfidUid) {
    // FNV-1a gives the server and ESP32 the same short code for one UID.
    const std::string normalizedUid = normalizeCode(rfidUid);
    uint32_t hash = 2166136261u;
    for (unsigned char character : normalizedUid) {
      hash ^= character;
      hash *= 16777619u;
    }

    const char digits[] = "0123456789ABCDEF";
    std::string accessCode(8, '0');
    for (int index = 7; index >= 0; --index) {
      accessCode[index] = digits[hash & 0x0F];
      hash >>= 4;
    }
    return accessCode;
  }

  static bool isValidAccessCode(const std::string& response,
                                const std::string& rfidUid) {
    return !rfidUid.empty() &&
           normalizeCode(response) == deriveAccessCode(rfidUid);
  }
};

}  // namespace door_lock