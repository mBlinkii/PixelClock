#pragma once
#include <cstring>

inline int normalizeWeatherApiCode(int code) {
  switch (code) {
    case 1000: return 0;
    case 1003: return 2;
    case 1006: case 1009: return 3;
    case 1030: case 1135: case 1147: return 45;
    case 1150: case 1153: return 51;
    case 1072: case 1168: case 1171: return 57;
    case 1063: case 1180: case 1183: case 1186: case 1189: case 1192: case 1195: return 61;
    case 1069: case 1198: case 1201: case 1204: case 1207: case 1249: case 1252: return 67;
    case 1066: case 1114: case 1117: case 1210: case 1213: case 1216: case 1219: case 1222: case 1225: return 71;
    case 1237: case 1261: case 1264: return 77;
    case 1240: case 1243: case 1246: return 80;
    case 1255: case 1258: return 85;
    case 1087: case 1273: case 1276: case 1279: case 1282: return 95;
    default: return -1;
  }
}

inline int normalizeMetSymbol(const char *symbol) {
  if (!symbol || !*symbol) return -1;
  if (strstr(symbol, "thunder")) return 95;
  if (strstr(symbol, "sleet")) return 67;
  if (strstr(symbol, "snow")) return strstr(symbol, "showers") ? 85 : 71;
  if (strstr(symbol, "rain")) return strstr(symbol, "showers") ? 80 : 61;
  if (strstr(symbol, "fog")) return 45;
  if (strncmp(symbol, "clearsky", 8) == 0) return 0;
  if (strncmp(symbol, "fair", 4) == 0) return 1;
  if (strncmp(symbol, "partlycloudy", 12) == 0) return 2;
  if (strcmp(symbol, "cloudy") == 0) return 3;
  return -1;
}

inline int normalizeOpenWeatherCode(int code) {
  if (code >= 200 && code < 300) return 95;
  if (code >= 300 && code < 400) return 51;
  if (code >= 500 && code < 600) return 61;
  if (code >= 600 && code < 700) return 71;
  if (code >= 700 && code < 800) return 45;
  if (code == 800) return 0;
  if (code == 801) return 1;
  if (code == 802) return 2;
  if (code == 803 || code == 804) return 3;
  return -1;
}

inline int normalizeBrightSkyIcon(const char *icon) {
  if (icon == nullptr || strlen(icon) == 0) return -1;
  if (strcmp(icon, "clear-day") == 0 || strcmp(icon, "clear-night") == 0) return 0;
  if (strcmp(icon, "partly-cloudy-day") == 0 || strcmp(icon, "partly-cloudy-night") == 0) return 2;
  if (strcmp(icon, "cloudy") == 0) return 3;
  if (strcmp(icon, "fog") == 0) return 45;
  if (strcmp(icon, "drizzle") == 0) return 51;
  if (strcmp(icon, "rain") == 0) return 61;
  if (strcmp(icon, "sleet") == 0) return 67;
  if (strcmp(icon, "snow") == 0) return 71;
  if (strcmp(icon, "hail") == 0) return 77;
  if (strcmp(icon, "thunderstorm") == 0) return 95;
  return -1;
}


