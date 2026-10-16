#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Own appointments entered in the web UI, without iCal. Kept free of Arduino
// types so tests/firmware-tests.cpp can check the rules natively.
constexpr uint8_t MAX_CALENDAR_EVENTS = 24;
// UTF-8 bytes; the web UI limits the text to 32 characters.
constexpr uint8_t CALENDAR_TEXT_MAX = 64;
constexpr int16_t CALENDAR_ALL_DAY = -1;

struct CalendarEvent {
  uint16_t id;
  uint16_t year;
  uint8_t month;  // 1-12
  uint8_t day;    // 1-31
  int16_t minute; // minutes after midnight, CALENDAR_ALL_DAY for the whole day
  uint8_t yearly; // repeats every year on the same day, e.g. birthdays
  char text[CALENDAR_TEXT_MAX + 1];
};

inline bool calendarLeapYear(uint16_t year) {
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

inline uint8_t calendarDaysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  return month == 2 && calendarLeapYear(year) ? 29 : days[month - 1];
}

inline bool calendarDateValid(uint16_t year, uint8_t month, uint8_t day) {
  return year >= 1900 && year <= 2199 && day >= 1 && day <= calendarDaysInMonth(year, month);
}

static inline bool calendarDigits(const char *text, size_t count, int &value) {
  value = 0;
  for (size_t i = 0; i < count; ++i) {
    if (text[i] < '0' || text[i] > '9') return false;
    value = value * 10 + (text[i] - '0');
  }
  return true;
}

// "YYYY-MM-DD" as sent by <input type="date">.
inline bool parseCalendarDate(const char *text, uint16_t &year, uint8_t &month, uint8_t &day) {
  int y, m, d;
  if (!text || strlen(text) != 10 || text[4] != '-' || text[7] != '-') return false;
  if (!calendarDigits(text, 4, y) || !calendarDigits(text + 5, 2, m) || !calendarDigits(text + 8, 2, d)) return false;
  if (!calendarDateValid(y, m, d)) return false;
  year = y;
  month = m;
  day = d;
  return true;
}

// "HH:MM" (seconds are ignored); an empty text means all day.
inline bool parseCalendarTime(const char *text, int16_t &minute) {
  if (!text || !*text) {
    minute = CALENDAR_ALL_DAY;
    return true;
  }
  const size_t length = strlen(text);
  int h, m, s;
  if ((length != 5 && length != 8) || text[2] != ':') return false;
  if (!calendarDigits(text, 2, h) || !calendarDigits(text + 3, 2, m) || h > 23 || m > 59) return false;
  if (length == 8 && (text[5] != ':' || !calendarDigits(text + 6, 2, s))) return false;
  minute = h * 60 + m;
  return true;
}

// A yearly event from 29 February shows on 28 February in other years.
inline bool calendarEventOn(const CalendarEvent &event, uint16_t year, uint8_t month, uint8_t day) {
  if (!event.yearly) return event.year == year && event.month == month && event.day == day;
  if (event.month == 2 && event.day == 29 && !calendarLeapYear(year)) return month == 2 && day == 28;
  return event.month == month && event.day == day && year >= event.year;
}

// Order for storage and the web list: date, then all-day before timed events.
inline bool calendarEventBefore(const CalendarEvent &a, const CalendarEvent &b) {
  if (a.year != b.year) return a.year < b.year;
  if (a.month != b.month) return a.month < b.month;
  if (a.day != b.day) return a.day < b.day;
  if (a.minute != b.minute) return a.minute < b.minute;
  return a.id < b.id;
}

inline void sortCalendarEvents(CalendarEvent *events, uint8_t count) {
  for (uint8_t i = 1; i < count; ++i) {
    CalendarEvent current = events[i];
    uint8_t j = i;
    while (j > 0 && calendarEventBefore(current, events[j - 1])) {
      events[j] = events[j - 1];
      --j;
    }
    events[j] = current;
  }
}

// Trims spaces, drops control characters and cuts to CALENDAR_TEXT_MAX bytes
// without splitting a UTF-8 sequence. Returns false when nothing is left.
inline bool sanitizeCalendarText(const char *input, char *out) {
  size_t length = 0;
  const unsigned char *p = reinterpret_cast<const unsigned char *>(input ? input : "");
  while (*p == ' ' || *p == '\t') ++p;
  while (*p) {
    size_t sequence = 1;
    if (*p >= 0xF0) sequence = 4;
    else if (*p >= 0xE0) sequence = 3;
    else if (*p >= 0xC0) sequence = 2;
    if (*p < 0x20 || *p == 0x7F) {
      ++p;
      continue;
    }
    size_t available = 0;
    while (available < sequence && p[available]) ++available;
    if (available < sequence || length + sequence > CALENDAR_TEXT_MAX) break;
    memcpy(out + length, p, sequence);
    length += sequence;
    p += sequence;
  }
  while (length && out[length - 1] == ' ') --length;
  out[length] = '\0';
  return length > 0;
}

// Display codes for umlauts; display.cpp draws them as A/O/U with two dots.
constexpr char CALENDAR_GLYPH_AE = '\x01';
constexpr char CALENDAR_GLYPH_OE = '\x02';
constexpr char CALENDAR_GLYPH_UE = '\x03';

// Converts UTF-8 text to the upper-case characters of the 3x5 matrix font.
// Accented Latin letters lose their accent, unknown characters are skipped.
inline size_t calendarDisplayText(const char *input, char *out, size_t outSize) {
  if (!outSize) return 0;
  size_t length = 0;
  const auto put = [&](char c) {
    if (length + 1 < outSize) out[length++] = c;
  };
  const unsigned char *p = reinterpret_cast<const unsigned char *>(input ? input : "");
  while (*p) {
    const unsigned char c = *p;
    if (c < 0x80) {
      if (c >= 'a' && c <= 'z') put(static_cast<char>(c - 32));
      else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || strchr(" .:-!?/+,'()", c)) put(static_cast<char>(c));
      ++p;
      continue;
    }
    if (c == 0xC3 && p[1]) {
      // Second byte of U+00C0-U+00FF; upper case (0x80-0x9E) folds onto lower case.
      const unsigned char n = p[1] | 0x20;
      if (p[1] == 0x9F) { put('S'); put('S'); }
      else if (n == 0xA4) put(CALENDAR_GLYPH_AE);
      else if (n == 0xB6) put(CALENDAR_GLYPH_OE);
      else if (n == 0xBC) put(CALENDAR_GLYPH_UE);
      else if (n >= 0xA0 && n <= 0xA5) put('A');
      else if (n == 0xA7) put('C');
      else if (n >= 0xA8 && n <= 0xAB) put('E');
      else if (n >= 0xAC && n <= 0xAF) put('I');
      else if (n == 0xB1) put('N');
      else if ((n >= 0xB2 && n <= 0xB5) || n == 0xB8) put('O');
      else if (n >= 0xB9 && n <= 0xBB) put('U');
      else if (n == 0xBD || n == 0xBF) put('Y');
      p += 2;
      continue;
    }
    // Skip any other multi-byte sequence (emoji, other scripts).
    ++p;
    while ((*p & 0xC0) == 0x80) ++p;
  }
  out[length] = '\0';
  return length;
}
