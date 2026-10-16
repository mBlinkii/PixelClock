#include "app_state.h"

// Own appointments live in their own NVS namespace, so "reset settings" keeps
// them; the factory reset erases them with the rest of NVS.
static constexpr const char *CALENDAR_NAMESPACE = "pixel-events";
static constexpr uint8_t CALENDAR_STORAGE_VERSION = 1;

CalendarEvent calendarEvents[MAX_CALENDAR_EVENTS];
uint8_t calendarEventCount = 0;

static bool storedEventValid(const CalendarEvent &event) {
  return event.id != 0 && calendarDateValid(event.year, event.month, event.day) &&
    event.minute >= CALENDAR_ALL_DAY && event.minute < 24 * 60 &&
    memchr(event.text, '\0', sizeof(event.text)) && event.text[0];
}

void loadCalendarEvents() {
  calendarEventCount = 0;
  prefs.begin(CALENDAR_NAMESPACE, true);
  const bool known = prefs.getUChar("ver", 0) == CALENDAR_STORAGE_VERSION;
  const size_t bytes = known ? prefs.getBytesLength("list") : 0;
  if (bytes && bytes % sizeof(CalendarEvent) == 0 && bytes <= sizeof(calendarEvents)) {
    prefs.getBytes("list", calendarEvents, bytes);
    const uint8_t stored = bytes / sizeof(CalendarEvent);
    for (uint8_t i = 0; i < stored; ++i) {
      if (storedEventValid(calendarEvents[i])) calendarEvents[calendarEventCount++] = calendarEvents[i];
    }
  }
  prefs.end();
  sortCalendarEvents(calendarEvents, calendarEventCount);
}

bool saveCalendarEvents() {
  StateLock lock;
  sortCalendarEvents(calendarEvents, calendarEventCount);
  prefs.begin(CALENDAR_NAMESPACE, false);
  bool ok = prefs.putUChar("ver", CALENDAR_STORAGE_VERSION) == 1;
  if (calendarEventCount) {
    const size_t bytes = calendarEventCount * sizeof(CalendarEvent);
    ok = ok && prefs.putBytes("list", calendarEvents, bytes) == bytes;
  } else if (prefs.isKey("list")) {
    ok = ok && prefs.remove("list");
  }
  prefs.end();
  return ok;
}

uint16_t nextCalendarEventId() {
  uint16_t id = 0;
  for (uint8_t i = 0; i < calendarEventCount; ++i) id = max(id, calendarEvents[i].id);
  return id == UINT16_MAX ? 1 : id + 1;
}

int findCalendarEvent(uint16_t id) {
  for (uint8_t i = 0; i < calendarEventCount; ++i) {
    if (calendarEvents[i].id == id) return i;
  }
  return -1;
}

// Fills `out` with today's events, all-day ones first, then by time.
uint8_t calendarEventsToday(const CalendarEvent **out, uint8_t capacity) {
  struct tm timeinfo;
  if (!calendarEventCount || !getLocalTime(&timeinfo, 10)) return 0;
  const uint16_t year = timeinfo.tm_year + 1900;
  uint8_t count = 0;
  for (uint8_t i = 0; i < calendarEventCount && count < capacity; ++i) {
    const CalendarEvent &event = calendarEvents[i];
    if (!calendarEventOn(event, year, timeinfo.tm_mon + 1, timeinfo.tm_mday)) continue;
    uint8_t j = count++;
    while (j > 0 && out[j - 1]->minute > event.minute) {
      out[j] = out[j - 1];
      --j;
    }
    out[j] = &event;
  }
  return count;
}

bool hasCalendarEventsToday() {
  const CalendarEvent *today[1];
  return calendarEventsToday(today, 1) > 0;
}
