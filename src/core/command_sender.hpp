#pragma once

#include <cstdint>

template <typename Derived> class ICommandSender {
public:
  virtual ~ICommandSender() = default;

  template <typename CommandT>
  void send(const CommandT &cmd, const uint64_t sender_id) {
    static_cast<Derived *>(this)->send_command(cmd, sender_id);
  }
};
