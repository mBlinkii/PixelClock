#include <ArduinoJson.h>
#include <esp_random.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/sha256.h>

#include "app_state.h"

// Admin passwords are stored as salted PBKDF2-HMAC-SHA256. The browser sends
// Basic credentials with every API request, so the digest of the last verified
// Authorization header is remembered until the credentials change; only new
// headers pay for the key derivation. Failed attempts are throttled per client.

static LoginThrottle loginThrottle;
static uint8_t verifiedHeaderDigest[32];
static bool verifiedHeaderValid = false;
constexpr size_t ADMIN_SALT_BYTES = 16;
constexpr size_t ADMIN_HASH_BYTES = 32;
constexpr size_t MAX_AUTHORIZATION_HEADER_LENGTH = 256;

static String toHex(const uint8_t *data, size_t length) {
  static const char digits[] = "0123456789abcdef";
  String out;
  out.reserve(length * 2);
  for (size_t i = 0; i < length; i++) {
    out += digits[data[i] >> 4];
    out += digits[data[i] & 0x0f];
  }
  return out;
}

static bool fromHex(const String &hex, uint8_t *out, size_t length) {
  if (hex.length() != length * 2) return false;
  for (size_t i = 0; i < length * 2; i++) {
    const char c = hex[i];
    uint8_t value;
    if (c >= '0' && c <= '9') value = c - '0';
    else if (c >= 'a' && c <= 'f') value = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') value = c - 'A' + 10;
    else return false;
    out[i / 2] = (i % 2) ? (out[i / 2] | value) : static_cast<uint8_t>(value << 4);
  }
  return true;
}

static bool derivePasswordHash(const char *password, size_t length, const uint8_t *salt, uint8_t *key) {
  mbedtls_md_context_t context;
  mbedtls_md_init(&context);
  const bool ok = mbedtls_md_setup(&context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) == 0 &&
    mbedtls_pkcs5_pbkdf2_hmac(&context, reinterpret_cast<const unsigned char *>(password), length,
                              salt, ADMIN_SALT_BYTES, ADMIN_HASH_ITERATIONS, ADMIN_HASH_BYTES, key) == 0;
  mbedtls_md_free(&context);
  return ok;
}

void invalidateAdminAuthCache() {
  verifiedHeaderValid = false;
}

bool adminPasswordIsDefault() {
  return config.adminPasswordHash.isEmpty();
}

// Stores a new admin password. The factory default is kept as "no hash" so the
// UI can keep reminding the owner to change it.
bool setAdminPassword(const String &password) {
  StateLock lock;
  invalidateAdminAuthCache();
  if (password == DEFAULT_ADMIN_PASSWORD) {
    config.adminPasswordSalt = "";
    config.adminPasswordHash = "";
    return true;
  }
  uint8_t salt[ADMIN_SALT_BYTES];
  uint8_t key[ADMIN_HASH_BYTES];
  esp_fill_random(salt, sizeof(salt));
  if (!derivePasswordHash(password.c_str(), password.length(), salt, key)) return false;
  config.adminPasswordSalt = toHex(salt, sizeof(salt));
  config.adminPasswordHash = toHex(key, sizeof(key));
  return true;
}

void resetAdminCredentials() {
  StateLock lock;
  invalidateAdminAuthCache();
  config.adminUsername = DEFAULT_ADMIN_USERNAME;
  config.adminPasswordSalt = "";
  config.adminPasswordHash = "";
}

// Converts a plain password saved by firmware 0.1.17 or older.
bool migratePlainAdminPassword(const String &password) {
  if (password.length() < MIN_ADMIN_PASSWORD_LENGTH || password.length() > MAX_ADMIN_PASSWORD_LENGTH) {
    config.adminPasswordSalt = "";
    config.adminPasswordHash = "";
    return true;
  }
  return setAdminPassword(password);
}

bool adminCredentialsValid() {
  if (config.adminPasswordHash.isEmpty()) return config.adminPasswordSalt.isEmpty();
  uint8_t scratch[ADMIN_HASH_BYTES];
  return fromHex(config.adminPasswordSalt, scratch, ADMIN_SALT_BYTES) &&
    fromHex(config.adminPasswordHash, scratch, ADMIN_HASH_BYTES);
}

static bool passwordMatches(const char *password) {
  const size_t length = strlen(password);
  if (config.adminPasswordHash.isEmpty()) {
    const size_t expected = strlen(DEFAULT_ADMIN_PASSWORD);
    return length == expected && constantTimeEquals(password, DEFAULT_ADMIN_PASSWORD, expected);
  }
  uint8_t salt[ADMIN_SALT_BYTES];
  uint8_t stored[ADMIN_HASH_BYTES];
  uint8_t candidate[ADMIN_HASH_BYTES];
  if (!fromHex(config.adminPasswordSalt, salt, sizeof(salt)) ||
      !fromHex(config.adminPasswordHash, stored, sizeof(stored)) ||
      !derivePasswordHash(password, length, salt, candidate)) {
    return false;
  }
  return constantTimeEquals(candidate, stored, sizeof(stored));
}

static bool authorizationValid(const String &header) {
  if (header.isEmpty() || header.length() > MAX_AUTHORIZATION_HEADER_LENGTH) return false;
  uint8_t digest[32];
  const bool digestOk = mbedtls_sha256_ret(reinterpret_cast<const unsigned char *>(header.c_str()),
                                           header.length(), digest, 0) == 0;
  if (digestOk && verifiedHeaderValid && constantTimeEquals(digest, verifiedHeaderDigest, sizeof(digest))) return true;

  BasicCredentials credentials;
  if (!parseBasicAuthorization(header.c_str(), credentials)) return false;
  const bool valid = config.adminUsername.equals(credentials.username) && passwordMatches(credentials.password);
  memset(&credentials, 0, sizeof(credentials));
  if (valid && digestOk) {
    memcpy(verifiedHeaderDigest, digest, sizeof(digest));
    verifiedHeaderValid = true;
  }
  return valid;
}

static String authorizationHeader(AsyncWebServerRequest *request) {
  const AsyncWebHeader *header = request->getHeader("Authorization");
  return header ? header->value() : String();
}

static uint32_t clientAddress(AsyncWebServerRequest *request) {
  AsyncClient *client = request->client();
  return client ? static_cast<uint32_t>(client->remoteIP()) : 0;
}

// Side-effect free check for upload chunks.
bool isAdminAuthorized(AsyncWebServerRequest *request) {
  StateLock lock;
  return authorizationValid(authorizationHeader(request));
}

bool requireAdminAuth(AsyncWebServerRequest *request) {
  StateLock lock;
  const uint32_t client = clientAddress(request);
  const uint32_t now = millis();
  const uint32_t retryMs = loginThrottle.retryAfterMs(client, now);
  if (retryMs) {
    const uint32_t seconds = (retryMs + 999) / 1000;
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = "Zu viele Fehlversuche. Bitte später erneut versuchen.";
    doc["retryAfterSeconds"] = seconds;
    String body;
    serializeJson(doc, body);
    AsyncWebServerResponse *response = request->beginResponse(429, "application/json", body);
    response->addHeader("Retry-After", String(seconds));
    request->send(response);
    return false;
  }
  const String header = authorizationHeader(request);
  if (authorizationValid(header)) {
    loginThrottle.recordSuccess(client);
    return true;
  }
  if (!header.isEmpty()) loginThrottle.recordFailure(client, now);
  sendJsonError(request, 401, "Admin-Anmeldung erforderlich.");
  return false;
}
