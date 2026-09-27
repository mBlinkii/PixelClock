#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Portable helpers for HTTP Basic credentials, login throttling and device
// naming. They avoid Arduino/ESP-IDF types so tests/firmware-tests.cpp can run
// them natively; hashing and request handling live in admin_auth.cpp.

constexpr size_t MAX_ADMIN_USERNAME_LENGTH = 31;
constexpr size_t MAX_ADMIN_PASSWORD_LENGTH = 64;

inline int base64Value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

// Decodes padded standard base64. Rejects malformed input and output overflow.
inline bool decodeBase64(const char *input, size_t length, uint8_t *output, size_t capacity, size_t &written) {
  written = 0;
  if (length % 4 != 0) return false;
  for (size_t i = 0; i < length; i += 4) {
    uint32_t triple = 0;
    uint8_t padding = 0;
    for (size_t j = 0; j < 4; j++) {
      const char c = input[i + j];
      int value = 0;
      if (c == '=') {
        if (i + 4 != length || j < 2) return false;
        ++padding;
      } else {
        if (padding) return false;
        value = base64Value(c);
        if (value < 0) return false;
      }
      triple = (triple << 6) | static_cast<uint32_t>(value);
    }
    const uint8_t bytes[3] = {
      static_cast<uint8_t>(triple >> 16), static_cast<uint8_t>(triple >> 8), static_cast<uint8_t>(triple)};
    const size_t count = 3 - padding;
    if (written + count > capacity) return false;
    memcpy(output + written, bytes, count);
    written += count;
  }
  return true;
}

struct BasicCredentials {
  char username[MAX_ADMIN_USERNAME_LENGTH + 1];
  char password[MAX_ADMIN_PASSWORD_LENGTH + 1];
};

// Parses "Basic <base64(user:password)>". The scheme is case-insensitive; the
// password may contain ':' but no NUL bytes.
inline bool parseBasicAuthorization(const char *header, BasicCredentials &out) {
  if (!header) return false;
  while (*header == ' ') ++header;
  static const char scheme[] = "basic ";
  for (size_t i = 0; i < sizeof(scheme) - 1; i++) {
    char c = header[i];
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
    if (c != scheme[i]) return false;
  }
  const char *encoded = header + sizeof(scheme) - 1;
  while (*encoded == ' ') ++encoded;
  size_t length = strlen(encoded);
  while (length && encoded[length - 1] == ' ') --length;
  uint8_t decoded[sizeof(out.username) + sizeof(out.password)];
  size_t decodedLength = 0;
  if (!decodeBase64(encoded, length, decoded, sizeof(decoded), decodedLength)) return false;
  if (memchr(decoded, 0, decodedLength)) return false;
  const uint8_t *colon = static_cast<const uint8_t *>(memchr(decoded, ':', decodedLength));
  if (!colon) return false;
  const size_t userLength = static_cast<size_t>(colon - decoded);
  const size_t passwordLength = decodedLength - userLength - 1;
  if (userLength > MAX_ADMIN_USERNAME_LENGTH || passwordLength > MAX_ADMIN_PASSWORD_LENGTH) return false;
  memcpy(out.username, decoded, userLength);
  out.username[userLength] = 0;
  memcpy(out.password, colon + 1, passwordLength);
  out.password[passwordLength] = 0;
  return true;
}

inline bool constantTimeEquals(const void *left, const void *right, size_t length) {
  const uint8_t *a = static_cast<const uint8_t *>(left);
  const uint8_t *b = static_cast<const uint8_t *>(right);
  uint8_t difference = 0;
  for (size_t i = 0; i < length; i++) difference |= a[i] ^ b[i];
  return difference == 0;
}

// Failed-login backoff per client address: five free attempts, then a lock of
// 30 s that doubles with every further failure up to 5 min. Counters are
// forgotten 15 min after the last failure. Timers use wrapping millis().
class LoginThrottle {
 public:
  static constexpr uint8_t SLOTS = 8;
  static constexpr uint8_t FREE_FAILURES = 5;
  static constexpr uint32_t BASE_LOCK_MS = 30000;
  static constexpr uint32_t MAX_LOCK_MS = 300000;
  static constexpr uint32_t FORGET_MS = 900000;

  uint32_t retryAfterMs(uint32_t client, uint32_t now) const {
    const Slot *slot = find(client);
    if (!slot || slot->failures < FREE_FAILURES) return 0;
    const uint32_t elapsed = now - slot->lastFailure;
    const uint32_t lock = lockDuration(slot->failures);
    return elapsed < lock ? lock - elapsed : 0;
  }

  void recordFailure(uint32_t client, uint32_t now) {
    Slot *slot = find(client);
    if (slot && now - slot->lastFailure >= FORGET_MS) slot->failures = 0;
    if (!slot) slot = claim(client);
    if (slot->failures < 255) ++slot->failures;
    slot->lastFailure = now;
  }

  void recordSuccess(uint32_t client) {
    Slot *slot = find(client);
    if (slot) *slot = Slot();
  }

  void clear() {
    for (Slot &slot : slots) slot = Slot();
  }

 private:
  struct Slot {
    uint32_t client = 0;
    uint32_t lastFailure = 0;
    uint8_t failures = 0;
    bool used = false;
  };
  Slot slots[SLOTS];

  static uint32_t lockDuration(uint8_t failures) {
    uint32_t lock = BASE_LOCK_MS;
    for (uint8_t i = FREE_FAILURES; i < failures && lock < MAX_LOCK_MS; i++) lock *= 2;
    return lock < MAX_LOCK_MS ? lock : MAX_LOCK_MS;
  }

  const Slot *find(uint32_t client) const {
    for (const Slot &slot : slots) {
      if (slot.used && slot.client == client) return &slot;
    }
    return nullptr;
  }

  Slot *find(uint32_t client) {
    return const_cast<Slot *>(static_cast<const LoginThrottle *>(this)->find(client));
  }

  // Reuses a free slot, otherwise the one whose last failure is oldest.
  Slot *claim(uint32_t client) {
    Slot *victim = &slots[0];
    for (Slot &slot : slots) {
      if (!slot.used) { victim = &slot; break; }
      if (static_cast<int32_t>(slot.lastFailure - victim->lastFailure) < 0) victim = &slot;
    }
    *victim = Slot();
    victim->used = true;
    victim->client = client;
    return victim;
  }
};

// Last three MAC bytes as upper-case hex, shared by the router hostname, the
// setup access point name and the matrix setup prompt.
inline void formatDeviceSuffix(const uint8_t mac[6], char out[7]) {
  snprintf(out, 7, "%02X%02X%02X", mac[3], mac[4], mac[5]);
}
