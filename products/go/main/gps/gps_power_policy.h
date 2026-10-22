#pragma once

#include "go_types.h"
#include <cstdint>

inline constexpr uint32_t GPS_IDLE_SLEEP_MS = 60000;
// Longest U4 duration, not an undocumented infinite-sleep sentinel.
inline constexpr uint32_t GPS_ALWAYS_OFF_SLEEP_MS = UINT32_MAX;
// Provisional until reset-to-GPS-ready time is measured on the bench.
inline constexpr uint32_t GPS_RESET_THRESHOLD_MS = 10000;
inline constexpr uint32_t GPS_RESET_CONFIRM_MS = 1000;

inline uint32_t gps_inactive_sleep_ms(GpsMode gps_mode, OperatingMode mode, uint32_t next_wake_ms) {
  if (gps_mode == GpsMode::AlwaysOff) {
    return GPS_ALWAYS_OFF_SLEEP_MS;
  }
  return mode == OperatingMode::Offline && next_wake_ms != 0 ? next_wake_ms : GPS_IDLE_SLEEP_MS;
}
