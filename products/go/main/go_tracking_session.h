#pragma once

#include "ag_log.h"
#include "common.h"
#include "go_storage.h"

#include <cinttypes>

/// Random 5-digit ID with bounded collision retry. Returns 0 on exhaustion.
inline uint32_t generate_tracking_session_id(StorageService &storage) {
  static constexpr const char *TAG = "TrackingSession";
  static constexpr uint8_t SESSION_ID_LENGTH = 5;
  static constexpr int SESSION_ID_MAX_RETRIES = 5;

  // Probe existing route files before opening a new session.
  for (int i = 0; i < SESSION_ID_MAX_RETRIES; ++i) {
    const uint32_t id = generate_random_number(SESSION_ID_LENGTH);
    if (!storage.route_file_exists(id)) {
      AG_LOGI(TAG, "generate_session_id: %" PRIu32 " (try %d)", id, i + 1);
      return id;
    }
    AG_LOGW(TAG, "generate_session_id: collision on %" PRIu32 " (try %d)", id, i + 1);
  }
  AG_LOGE(TAG, "generate_session_id: exhausted %d retries", SESSION_ID_MAX_RETRIES);
  return 0;
}
