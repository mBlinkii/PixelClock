#include "app_state.h"
#include "weather_icons.h"

uint16_t xy(uint8_t x, uint8_t y) {
  if (x >= config.width || y >= config.height) return 0;
  uint8_t physX = x;
  uint8_t physY = y;
  if (config.origin == 1 || config.origin == 3) physX = config.width - 1 - physX;
  if (config.origin == 2 || config.origin == 3) physY = config.height - 1 - physY;

  if (config.wiringMode == 2 || config.wiringMode == 3) {
    if (config.wiringMode == 3 && (physX % 2 == 1)) physY = config.height - 1 - physY;
    return physX * config.height + physY;
  }

  if (config.wiringMode == 1 && (physY % 2 == 1)) physX = config.width - 1 - physX;
  return physY * config.width + physX;
}

void px(int x, int y, CRGB color) {
  if (x < 0 || y < 0 || x >= config.width || y >= config.height) return;
  const uint16_t i = xy(x, y);
  if (i < ledCount) leds[i] = color;
}

template <uint8_t PIN>
void addFastLedsForPin() {
  if (config.colorRgb) {
    FastLED.addLeds<WS2812B, PIN, RGB>(leds, ledCount);
  } else {
    FastLED.addLeds<WS2812B, PIN, GRB>(leds, ledCount);
  }
}

void setupFastLed() {
  ledCount = min<uint16_t>(MAX_LEDS, config.width * config.height);
  switch (config.dataPin) {
    case 2: addFastLedsForPin<2>(); break;
    case 4: addFastLedsForPin<4>(); break;
    case 5: addFastLedsForPin<5>(); break;
    case 12: addFastLedsForPin<12>(); break;
    case 13: addFastLedsForPin<13>(); break;
    case 14: addFastLedsForPin<14>(); break;
    case 15: addFastLedsForPin<15>(); break;
    case 16: addFastLedsForPin<16>(); break;
    case 17: addFastLedsForPin<17>(); break;
    case 18: addFastLedsForPin<18>(); break;
    case 19: addFastLedsForPin<19>(); break;
    case 21: addFastLedsForPin<21>(); break;
    case 22: addFastLedsForPin<22>(); break;
    case 23: addFastLedsForPin<23>(); break;
    case 25: addFastLedsForPin<25>(); break;
    case 26: addFastLedsForPin<26>(); break;
    case 27: addFastLedsForPin<27>(); break;
    case 32: addFastLedsForPin<32>(); break;
    case 33: addFastLedsForPin<33>(); break;
    default:
      config.dataPin = 18;
      addFastLedsForPin<18>();
      break;
  }
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setDither(true);
  FastLED.setBrightness(config.brightness);
  FastLED.clear(true);
}

// Matrix text, icon, page, and frame rendering.
void updateBrightnessForTime() {
  struct tm timeinfo;
  uint8_t target = config.brightness;
  if (getLocalTime(&timeinfo, 10)) {
    if (isNightTime()) target = min(config.brightness, config.nightBrightness);
  }
  FastLED.setBrightness(target);
}

bool isNightTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return !weather.isDay;
  const uint8_t h = timeinfo.tm_hour;
  return config.nightStart > config.nightEnd
    ? (h >= config.nightStart || h < config.nightEnd)
    : (h >= config.nightStart && h < config.nightEnd);
}

CRGB textPixelColor(int x, int y, CRGB fallback) {
  uint8_t mode = config.colorGradientMode;
  if (forceTextGradient && mode == 0) mode = 1;
  if (mode == 0) return fallback;
  const uint8_t hue = mode == 2
    ? (uint8_t)((millis() / 55) & 0xff)
    : (uint8_t)((x * 9 + y * 4 + millis() / 90) & 0xff);
  return CHSV(hue, 220, 255);
}

const char *glyphFor(char c) {
  switch (c) {
    case '0': return "111101101101111";
    case '1': return "010110010010111";
    case '2': return "111001111100111";
    case '3': return "111001111001111";
    case '4': return "101101111001001";
    case '5': return "111100111001111";
    case '6': return "111100111101111";
    case '7': return "111001001010010";
    case '8': return "111101111101111";
    case '9': return "111101111001111";
    case ':': return "000010000010000";
    case '.': return "000000000000010";
    case '-': return "000000111000000";
    case 'A': return "010101111101101";
    case 'C': return "111100100100111";
    case 'D': return "110101101101110";
    case 'F': return "111100110100100";
    case 'H': return "101101111101101";
    case 'I': return "111010010010111";
    case 'L': return "100100100100111";
    case 'M': return "101111111101101";
    case 'N': return "101111111111101";
    case 'O': return "111101101101111";
    case 'R': return "110101110101101";
    case 'S': return "111100111001111";
    case 'T': return "111010010010010";
    case 'E': return "111100110100111";
    case 'P': return "110101110100100";
    case 'W': return "101101111111101";
    default: return nullptr;
  }
}

uint8_t drawChar3x5(int x, int y, char c, CRGB color) {
  if (c == ' ') return 2;
  const char *g = glyphFor(c);
  if (!g) return 0;
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      if (g[row * 3 + col] == '1') px(x + col, y + row, textPixelColor(x + col, y + row, color));
    }
  }
  return 4;
}

uint8_t charAdvance3x5(char c) {
  if (c == ' ') return 2;
  if (c == ':' || c == '.') return 2;
  return glyphFor(c) ? 4 : 0;
}

void drawText3x5(int x, int y, const String &text, CRGB color) {
  int cursor = x;
  for (uint8_t i = 0; i < text.length(); i++) {
    cursor += drawChar3x5(cursor, y, text[i], color);
  }
}

void drawText3x5Styled(int x, int y, const String &text) {
  int cursor = x;
  for (uint8_t i = 0; i < text.length(); i++) {
    CRGB color = packedColor(config.colorText);
    if (text[i] == ':') color = packedColor(config.colorColon);
    else if (text[i] == '.') color = packedColor(config.colorPoint);
    drawChar3x5(cursor, y, text[i], color);
    cursor += charAdvance3x5(text[i]);
  }
}

uint8_t textWidth3x5(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += charAdvance3x5(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawRightAlignedStyled(int x, int y, int width, const String &text) {
  const int start = x + width - textWidth3x5(text);
  drawText3x5Styled(start, y, text);
}

void drawRightAlignedText(int x, int y, int width, const String &text, CRGB color) {
  const int start = x + width - textWidth3x5(text);
  int cursor = start;
  for (uint8_t i = 0; i < text.length(); i++) {
    drawChar3x5(cursor, y, text[i], color);
    cursor += charAdvance3x5(text[i]);
  }
}

void drawCenteredText3x5(int y, const String &text, CRGB color) {
  const int start = max(0, (int(config.width) - int(textWidth3x5(text))) / 2);
  drawText3x5(start, y, text, color);
}

void drawCenteredStyled3x5(int y, const String &text) {
  const int start = max(0, (int(config.width) - int(textWidth3x5(text))) / 2);
  drawText3x5Styled(start, y, text);
}

const char *bigGlyphFor(char c) {
  switch (c) {
    case '0': return "011101000110001100011000101110";
    case '1': return "001000110000100001000010001110";
    case '2': return "011101000100010001001000011111";
    case '3': return "111100000100110000010000111110";
    case '4': return "000100011001010111110001000010";
    case '5': return "111111000011110000010000111110";
    case '6': return "011101000011110100011000101110";
    case '7': return "111110000100010001000100001000";
    case '8': return "011101000101110100011000101110";
    case '9': return "011101000101111000010000101110";
    case '-': return "000000000000000111110000000000";
    case 'C': return "011101000110000100001000101110";
    case 'F': return "111111000011110100001000010000";
    default: return nullptr;
  }
}

uint8_t bigCharAdvance(char c) {
  return bigGlyphFor(c) ? 6 : 0;
}

uint8_t bigTextWidth(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += bigCharAdvance(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawBigChar(int x, int y, char c, CRGB color) {
  const char *g = bigGlyphFor(c);
  if (!g) return;
  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 5; col++) {
      if (g[row * 5 + col] == '1') px(x + col, y + row, textPixelColor(x + col, y + row, color));
    }
  }
}

void drawBigTextCentered(int y, const String &text) {
  int cursor = max(0, (int(config.width) - int(bigTextWidth(text))) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    CRGB color = packedColor(config.colorText);
    if (text[i] == ':') color = packedColor(config.colorColon);
    drawBigChar(cursor, y, text[i], color);
    cursor += bigCharAdvance(text[i]);
  }
}

void drawBigClockCentered(const tm &timeinfo) {
  char hours[3];
  char minutes[3];
  uint8_t hour = timeinfo.tm_hour;
  if (config.hourFormat == 12) {
    hour %= 12;
    if (hour == 0) hour = 12;
  }
  snprintf(hours, sizeof(hours), "%02d", hour);
  snprintf(minutes, sizeof(minutes), "%02d", timeinfo.tm_min);
  const CRGB textColor = packedColor(config.colorText);
  const CRGB colonColor = packedColor(config.colorColon);
  drawBigChar(1, 1, hours[0], textColor);
  drawBigChar(7, 1, hours[1], textColor);
  px(15, 3, textPixelColor(15, 3, colonColor));
  px(16, 3, textPixelColor(16, 3, colonColor));
  px(15, 5, textPixelColor(15, 5, colonColor));
  px(16, 5, textPixelColor(16, 5, colonColor));
  drawBigChar(20, 1, minutes[0], textColor);
  drawBigChar(26, 1, minutes[1], textColor);
}

const char *tallNarrowGlyphFor(char c) {
  switch (c) {
    case '0': return "111101101101101111";
    case '1': return "010110010010010111";
    case '2': return "111001001111100111";
    case '3': return "111001001111001111";
    case '4': return "101101101111001001";
    case '5': return "111100100111001111";
    case '6': return "111100100111101111";
    case '7': return "111001010010010010";
    case '8': return "111101101111101111";
    case '9': return "111101101111001111";
    default: return nullptr;
  }
}

uint8_t tallNarrowAdvance(char c) {
  if (c == '.') return 2;
  return tallNarrowGlyphFor(c) ? 4 : 0;
}

uint8_t tallNarrowWidth(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += tallNarrowAdvance(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawTallNarrowChar(int x, int y, char c, CRGB color) {
  const char *g = tallNarrowGlyphFor(c);
  if (!g) return;
  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      if (g[row * 3 + col] == '1') px(x + col, y + row, textPixelColor(x + col, y + row, color));
    }
  }
}

void drawLargeDateCentered(int y) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return;
  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02d.%02d.%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1, (timeinfo.tm_year + 1900) % 100);
  const String text(buffer);
  int cursor = max(0, (int(config.width) - 28) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    if (text[i] == '.') {
      px(cursor, y + 5, textPixelColor(cursor, y + 5, packedColor(config.colorPoint)));
    } else {
      drawTallNarrowChar(cursor, y, text[i], packedColor(config.colorText));
    }
    cursor += (i == 5) ? 3 : tallNarrowAdvance(text[i]);
  }
}

uint8_t compactDateAdvance(char c) {
  if (c == '.') return 2;
  if (c == '-') return 2;
  if (c == ' ') return 1;
  return glyphFor(c) ? 3 : 0;
}

uint8_t compactTextWidth3x5(const String &text) {
  uint8_t width = 0;
  for (uint8_t i = 0; i < text.length(); i++) width += compactDateAdvance(text[i]);
  return width > 0 ? width - 1 : 0;
}

void drawCompactTextCentered3x5(int y, const String &text, CRGB color) {
  int cursor = max(0, (int(config.width) - int(compactTextWidth3x5(text))) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    if (text[i] == '.') {
      px(cursor, y + 4, textPixelColor(cursor, y + 4, packedColor(config.colorPoint)));
    } else if (text[i] != ' ') {
      drawChar3x5(cursor, y, text[i], color);
    }
    cursor += compactDateAdvance(text[i]);
  }
}

void drawCompactDateCentered(int y) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return;
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  const String text(buffer);
  int cursor = max(0, (int(config.width) - int(compactTextWidth3x5(text))) / 2);
  for (uint8_t i = 0; i < text.length(); i++) {
    if (text[i] == '.') {
      px(cursor, y + 4, textPixelColor(cursor, y + 4, packedColor(config.colorPoint)));
    } else {
      drawChar3x5(cursor, y, text[i], packedColor(config.colorText));
    }
    cursor += compactDateAdvance(text[i]);
  }
}

void drawDateValue(int x, int y) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return;
  char day[3];
  char month[3];
  snprintf(day, sizeof(day), "%02d", timeinfo.tm_mday);
  snprintf(month, sizeof(month), "%02d", timeinfo.tm_mon + 1);
  drawText3x5(x, y, day, packedColor(config.colorText));
  px(x + 8, y + 4, textPixelColor(x + 8, y + 4, packedColor(config.colorPoint)));
  drawText3x5(x + 10, y, month, packedColor(config.colorText));
}

void drawFixedClock(int x, int y, const tm &timeinfo, bool showColon) {
  char hours[3];
  char minutes[3];
  uint8_t hour = timeinfo.tm_hour;
  if (config.hourFormat == 12) {
    hour %= 12;
    if (hour == 0) hour = 12;
  }
  snprintf(hours, sizeof(hours), "%02d", hour);
  snprintf(minutes, sizeof(minutes), "%02d", timeinfo.tm_min);
  drawText3x5(x, y, hours, packedColor(config.colorText));
  if (showColon) drawChar3x5(x + 8, y, ':', packedColor(config.colorColon));
  drawText3x5(x + 11, y, minutes, packedColor(config.colorText));
}

void drawIndicator(uint8_t active) {
  const uint8_t y = config.height - 1;
  int start = VALUE_SLOT_X + ((VALUE_SLOT_W - 13) / 2);
  if (config.displayMode == 1) start = LEFT_VALUE_X + ((LEFT_VALUE_W - 13) / 2);
  else if (config.displayMode == 2) start = (config.width - 13) / 2;
  for (uint8_t i = 0; i < 3; i++) {
    CRGB color = i == active ? packedColor(config.colorPoint) : CRGB(18, 18, 18);
    px(start + i * 5, y, color);
    px(start + i * 5 + 1, y, color);
    px(start + i * 5 + 2, y, color);
  }
}

void drawSun(int x, int y) {
  CRGB yellow(255, 190, 20);
  CRGB orange(255, 90, 0);
  px(x + 4, y, orange); px(x + 4, y + 7, orange);
  px(x, y + 3, orange); px(x + 8, y + 3, orange);
  px(x + 2, y + 1, orange); px(x + 6, y + 1, orange);
  px(x + 2, y + 6, orange); px(x + 6, y + 6, orange);
  for (uint8_t yy = 2; yy <= 5; yy++) {
    for (uint8_t xx = 3; xx <= 5; xx++) px(x + xx, y + yy, yellow);
  }
}

void drawCloud(int x, int y, CRGB color = CRGB(120, 130, 145)) {
  px(x + 3, y + 2, color); px(x + 4, y + 2, color);
  px(x + 2, y + 3, color); px(x + 3, y + 3, color); px(x + 4, y + 3, color); px(x + 5, y + 3, color);
  for (uint8_t xx = 1; xx <= 7; xx++) px(x + xx, y + 4, color);
  for (uint8_t xx = 2; xx <= 6; xx++) px(x + xx, y + 5, color);
}

void drawRain(int x, int y) {
  drawCloud(x, y, CRGB(105, 115, 130));
  CRGB blue(40, 120, 255);
  px(x + 2, y + 6, blue); px(x + 4, y + 7, blue); px(x + 6, y + 6, blue);
}

void drawSnow(int x, int y) {
  drawCloud(x, y, CRGB(130, 150, 170));
  CRGB ice(160, 220, 255);
  px(x + 2, y + 6, ice); px(x + 4, y + 6, ice); px(x + 6, y + 6, ice);
  px(x + 3, y + 7, ice); px(x + 5, y + 7, ice);
}

void drawThunder(int x, int y) {
  drawCloud(x, y, CRGB(90, 90, 110));
  CRGB bolt(255, 220, 20);
  px(x + 5, y + 5, bolt); px(x + 4, y + 6, bolt); px(x + 5, y + 6, bolt); px(x + 3, y + 7, bolt);
}

void drawFog(int x, int y) {
  CRGB grey(110, 120, 125);
  drawCloud(x, y, grey);
  for (uint8_t xx = 1; xx <= 8; xx += 2) px(x + xx, y + 7, grey);
}

void drawWeatherIcon(int x, int y) {
  const int c = weather.weatherCode;
  const bool night = isNightTime();
  const uint32_t *icon = icon_unknown;
  if (c == 0) icon = night ? icon_clear_night : icon_clear_day;
  else if (c == 1 || c == 2) icon = night ? icon_partly_night : icon_partly_day;
  else if (c == 3) icon = icon_cloud;
  else if (c == 45 || c == 48) icon = icon_fog;
  else if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) icon = icon_rain;
  else if ((c >= 71 && c <= 77) || (c >= 85 && c <= 86)) icon = icon_snow;
  else if (c >= 95) icon = icon_thunder;
  drawBitmapIcon(x, y, icon);
}

void drawTemperatureIcon(int x, int y) {
  drawBitmapIcon(x, y, icon_temperature);
}

void drawBitmapIcon(int x, int y, const uint32_t *icon) {
  for (uint8_t yy = 0; yy < WEATHER_ICON_H; yy++) {
    for (uint8_t xx = 0; xx < WEATHER_ICON_W; xx++) {
      const uint32_t color = pgm_read_dword(&icon[yy * WEATHER_ICON_W + xx]);
      if (color == 0) continue;
      px(x + xx, y + yy, CRGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));
    }
  }
}

void drawClockIcon(int x, int y, uint8_t frame) {
  CRGB body = packedColor(config.colorPoint);
  CRGB hand = packedColor(config.colorColon);
  px(x + 3, y, body); px(x + 4, y, body);
  px(x + 1, y + 1, body); px(x + 6, y + 1, body);
  px(x, y + 3, body); px(x + 7, y + 3, body);
  px(x, y + 4, body); px(x + 7, y + 4, body);
  px(x + 1, y + 6, body); px(x + 6, y + 6, body);
  px(x + 3, y + 7, body); px(x + 4, y + 7, body);
  px(x + 4, y + 4, hand);
  px(x + 4, y + 3, hand);
  if (frame % 4 == 0) px(x + 5, y + 4, hand);
  else if (frame % 4 == 1) px(x + 4, y + 5, hand);
  else if (frame % 4 == 2) px(x + 3, y + 4, hand);
  else px(x + 4, y + 2, hand);
}

void drawBootGreeting() {
  const uint8_t frame = ((millis() - bootStarted) / 350) % 4;
  forceTextGradient = true;
  drawClockIcon(0, 0, frame);
  drawText3x5(11, 1, "HELLO", packedColor(config.colorText));
  forceTextGradient = false;
}

void drawSetupPrompt() {
  drawClockIcon(0, 0, (millis() / 450) % 4);
  const bool showAp = ((millis() / 1600) % 2) == 1;
  drawText3x5(12, 1, showAp ? "AP" : "WIFI", packedColor(config.colorText));
}

String weekdayShort(const tm &t) {
  static const char *days[] = {"SO", "MO", "DI", "MI", "DO", "FR", "SA"};
  return days[t.tm_wday];
}

float displayTemperature(float celsius) {
  if (isnan(celsius)) return NAN;
  return config.temperatureUnit == 1 ? (celsius * 9.0f / 5.0f) + 32.0f : celsius;
}

String temperatureUnitText() {
  return config.temperatureUnit == 1 ? "F" : "C";
}

String currentTemperatureText() {
  if (isnan(weather.temperature)) return "--" + temperatureUnitText();
  return String((int)round(displayTemperature(weather.temperature))) + temperatureUnitText();
}

String temperatureOverviewText() {
  String text = currentTemperatureText();
  if (!isnan(weather.temperatureMin) && !isnan(weather.temperatureMax)) {
    text += " ";
    text += String((int)round(displayTemperature(weather.temperatureMin)));
    text += "-";
    text += String((int)round(displayTemperature(weather.temperatureMax)));
  }
  return text;
}

void drawClockPage() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) {
    drawText3x5(2, 1, "----", CRGB::Red);
    return;
  }
  if (config.displayMode == 1) {
    drawFixedClock(LEFT_VALUE_X, 1, timeinfo, true);
    drawWeatherIcon(RIGHT_ICON_X, 0);
  } else if (config.displayMode == 2) {
    drawBigClockCentered(timeinfo);
  } else {
    drawWeatherIcon(LEFT_SLOT_X, 0);
    drawFixedClock(VALUE_SLOT_X, 1, timeinfo, true);
  }
}

void drawDatePage() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) {
    drawText3x5(3, 1, "NO TIME", CRGB::Red);
    return;
  }
  if (config.displayMode == 1) {
    drawDateValue(LEFT_VALUE_X, 1);
    drawRightAlignedText(RIGHT_ICON_X, 1, 9, weekdayShort(timeinfo), packedColor(config.colorWeekday));
  } else if (config.displayMode == 2) {
    drawLargeDateCentered(1);
  } else {
    drawText3x5(LEFT_SLOT_X, 1, weekdayShort(timeinfo), packedColor(config.colorWeekday));
    drawDateValue(VALUE_SLOT_X, 1);
  }
}

void drawWeatherPage() {
  const String text = currentTemperatureText();
  if (config.displayMode == 1) {
    drawText3x5(LEFT_VALUE_X, 1, text, packedColor(config.colorText));
    drawTemperatureIcon(RIGHT_ICON_X, 0);
  } else if (config.displayMode == 2) {
    drawBigTextCentered(1, text);
  } else {
    drawTemperatureIcon(LEFT_SLOT_X, 0);
    drawRightAlignedText(VALUE_SLOT_X, 1, VALUE_SLOT_W, text, packedColor(config.colorText));
  }
}

void drawTestPattern() {
  for (uint8_t y = 0; y < config.height; y++) {
    for (uint8_t x = 0; x < config.width; x++) {
      px(x, y, CHSV((x * 255) / max<uint8_t>(1, config.width - 1), 220, 160));
    }
  }
}

void renderDisplay() {
  updateBrightnessForTime();
  fill_solid(leds, ledCount, CRGB::Black);
  if (millis() - bootStarted < BOOT_GREETING_MS) {
    drawBootGreeting();
  } else if (setupMode && config.ssid.isEmpty()) {
    drawSetupPrompt();
  } else if (displayTest && millis() < displayTestUntil) {
    drawTestPattern();
  } else {
    displayTest = false;
    const uint8_t page = config.autoPage ? currentPage : config.selectedPage;
    if (page == 0) drawClockPage();
    else if (page == 1) drawDatePage();
    else drawWeatherPage();
    if (config.displayMode != 2) drawIndicator(page);
  }
  FastLED.show();
}
