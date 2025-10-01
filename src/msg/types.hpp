#pragma once

#include <cstdint>

// Stable wire type identifiers. Must match values in messages.proto.
namespace msgtypes {
static constexpr uint32_t MESSAGE_TYPE_UNKNOWN = 0;
// Event types
static constexpr uint32_t MESSAGE_TYPE_EVENT_TEXT = 1;
static constexpr uint32_t MESSAGE_TYPE_EVENT_HEARTBEAT = 2;
// Command types
static constexpr uint32_t MESSAGE_TYPE_COMMAND_TEXT = 100;
static constexpr uint32_t MESSAGE_TYPE_COMMAND_START = 101;
static constexpr uint32_t MESSAGE_TYPE_COMMAND_STOP = 102;

inline bool is_event(uint32_t type) {
  switch (type) {
  case MESSAGE_TYPE_EVENT_TEXT:
  case MESSAGE_TYPE_EVENT_HEARTBEAT:
    return true;
  default:
    return false;
  }
}

inline bool is_command(uint32_t type) {
  switch (type) {
  case MESSAGE_TYPE_COMMAND_TEXT:
  case MESSAGE_TYPE_COMMAND_START:
  case MESSAGE_TYPE_COMMAND_STOP:
    return true;
  default:
    return false;
  }
}

inline uint32_t command_to_event(uint32_t command_type) {
  switch (command_type) {
  case MESSAGE_TYPE_COMMAND_TEXT:
    return MESSAGE_TYPE_EVENT_TEXT;
  default:
    return MESSAGE_TYPE_UNKNOWN;
  }
}
} // namespace msgtypes
