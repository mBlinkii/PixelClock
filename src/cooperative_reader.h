#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ArduinoJson's custom reader interface deliberately avoids Stream::timedRead(),
// which busy-waits on Arduino ESP32 2.x. Clock::pause() must block for a tick so
// the idle task can feed its watchdog, including with a stalled TLS connection.
template <class Client, class Clock>
class CooperativeReader {
 public:
  CooperativeReader(Client &client, uint32_t idleTimeoutMs, uint32_t totalTimeoutMs)
      : client_(client), idleTimeoutMs_(idleTimeoutMs), totalTimeoutMs_(totalTimeoutMs),
        started_(Clock::now()), lastData_(started_), lastYield_(started_) {}

  int read() {
    for (;;) {
      const uint32_t now = Clock::now();
      if (stopped_ || now - started_ >= totalTimeoutMs_) return stop();
      if (now - lastYield_ >= 16) pause();
      if (position_ < length_) return buffer_[position_++];
      if (now - lastData_ >= idleTimeoutMs_) return stop();

      const int available = client_.available();
      if (available > 0) {
        const size_t count = static_cast<size_t>(available) < sizeof(buffer_)
            ? static_cast<size_t>(available) : sizeof(buffer_);
        const int received = client_.read(buffer_, count);
        if (received > 0) {
          position_ = 0;
          length_ = static_cast<size_t>(received);
          lastData_ = Clock::now();
          continue;
        }
      }
      if (!client_.connected()) return stop();
      pause();
    }
  }

  size_t readBytes(char *output, size_t size) {
    size_t count = 0;
    for (; count < size; ++count) {
      const int value = read();
      if (value < 0) break;
      output[count] = static_cast<char>(value);
    }
    return count;
  }

  // Small streaming markers, used to locate MET's first timeseries object.
  bool find(const char *marker) {
    char window[32];
    const size_t size = strlen(marker);
    if (size == 0) return true;
    if (size > sizeof(window)) return false;
    size_t used = 0;
    for (int value; (value = read()) >= 0;) {
      if (used == size) {
        memmove(window, window + 1, size - 1);
        --used;
      }
      window[used++] = static_cast<char>(value);
      if (used == size && memcmp(window, marker, size) == 0) return true;
    }
    return false;
  }

 private:
  void pause() { Clock::pause(); lastYield_ = Clock::now(); }
  int stop() { stopped_ = true; return -1; }
  Client &client_;
  const uint32_t idleTimeoutMs_, totalTimeoutMs_;
  const uint32_t started_;
  uint32_t lastData_, lastYield_;
  uint8_t buffer_[256];
  size_t position_ = 0, length_ = 0;
  bool stopped_ = false;
};
