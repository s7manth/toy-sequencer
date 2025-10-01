#pragma once
#include "message.hpp"
#include <functional>

class Receiver {
public:
  explicit Receiver(std::function<void(const Message &)> on_message);

  void on_datagram(const uint8_t *data, size_t len);

private:
  uint64_t expected_seq_ = 1;
  std::function<void(const Message &)> on_message_;
};
