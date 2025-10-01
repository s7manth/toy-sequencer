#pragma once

#include "sender_iface.hpp"
#include <functional>
#include <mutex>
#include <vector>

// Simple in-process multicast bus for tests/demos. It implements ISender and
// fans out the sent datagram to all registered subscribers in the same process.
class InprocMulticastSender : public ISender {
public:
  using Subscriber = std::function<void(const std::vector<uint8_t> &)>;

  InprocMulticastSender() = default;

  // ISender
  bool send(const std::vector<uint8_t> &data) override;
  bool send(const std::vector<uint8_t> &data, uint8_t /*ttl*/) override;

  // Register a local subscriber to receive sent datagrams.
  void register_subscriber(Subscriber subscriber);

private:
  std::mutex subscribers_mutex_;
  std::vector<Subscriber> subscribers_;
};
