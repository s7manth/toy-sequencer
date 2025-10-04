#pragma once

#include "core/multicast_sender.hpp"
#include <cstdint>

template <typename Derived> class IEventSender : public MulticastSender {
public:
  IEventSender(const std::string &multicast_address, uint16_t port, uint8_t ttl)
      : MulticastSender(multicast_address, port, ttl) {}

  virtual ~IEventSender() = default;

  template <typename EventT> void send(const EventT &event) { static_cast<Derived *>(this)->send_event(event); }
};
