#pragma once
#include "generated/messages.pb.h"
#include <functional>

class Receiver {
public:
  explicit Receiver(
      std::function<void(const toysequencer::TextEvent &)> on_event,
      uint64_t instance_id);

  void on_datagram(const uint8_t *data, size_t len);

  // Convenience for in-process bus subscription
  void on_bytes(const std::vector<uint8_t> &bytes) {
    on_datagram(bytes.data(), bytes.size());
  }

  template <typename TDeserializer>
  bool try_deserialize(TDeserializer deserializer) {
    (void)deserializer;
    return false;
  }

private:
  uint64_t expected_seq_ = 1;
  uint64_t instance_id_;
  std::function<void(const toysequencer::TextEvent &)> on_event_;
};
