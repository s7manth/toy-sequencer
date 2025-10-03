#pragma once

#include "core/multicast_sender.hpp"
#include <cstdint>

template <typename Derived> class ICommandSender: public MulticastSender {
public:
  ICommandSender(const std::string &multicast_address, uint16_t port, uint8_t ttl)
      : MulticastSender(multicast_address, port, ttl) {}

  virtual ~ICommandSender() = default;

  template <typename CommandT>
  void send(const CommandT &cmd, const uint64_t sender_id) {
    static_cast<Derived *>(this)->send_command(cmd, sender_id);
  }
};
