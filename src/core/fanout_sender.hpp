#pragma once

#include "sender_iface.hpp"
#include <memory>
#include <vector>

// Sends each datagram to all underlying senders
class FanoutSender : public ISender {
public:
  FanoutSender() = default;

  void add_sender(std::shared_ptr<ISender> sender) {
    senders_.push_back(std::move(sender));
  }

  bool send(const std::vector<uint8_t> &data) override {
    bool all_ok = true;
    for (auto &s : senders_) {
      all_ok = s->send(data) && all_ok;
    }
    return all_ok;
  }

  bool send(const std::vector<uint8_t> &data, uint8_t ttl) override {
    bool all_ok = true;
    for (auto &s : senders_) {
      all_ok = s->send(data, ttl) && all_ok;
    }
    return all_ok;
  }

private:
  std::vector<std::shared_ptr<ISender>> senders_;
};
