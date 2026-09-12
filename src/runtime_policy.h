#pragma once
#include <stdint.h>

inline bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

inline bool weatherFetchDue(uint32_t now, uint32_t lastFetch, uint32_t lastAttempt,
                            uint32_t interval, uint32_t retryDelay, bool failed) {
  // Failed scheduled refreshes respect backoff even with an old good sample.
  if (failed) return now - lastAttempt >= retryDelay;
  return lastFetch == 0 ? lastAttempt == 0 : now - lastFetch >= interval;
}
