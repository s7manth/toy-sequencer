#pragma once

#include "core/multicast_receiver.hpp"
#include <cstdint>
#include <iostream>

template <typename Derived> class CommandReceiver : public MulticastReceiver {
public:
  explicit CommandReceiver(const std::string &multicast_address, uint16_t port)
      : MulticastReceiver(multicast_address, port) {}

  virtual ~CommandReceiver() = default;

  void start() { MulticastReceiver::start(); }

  void stop() { MulticastReceiver::stop(); }

  template <typename CommandT> void subscribe() {
    MulticastReceiver::subscribe(std::function<void(const uint8_t *data, size_t len)>(
        [this](const uint8_t *data, size_t len) { on_datagram<CommandT>(data, len); }));
  }

  template <typename CommandT> void on_datagram(const uint8_t *data, size_t len) {
    try {
      CommandT command;
      if (!command.ParseFromArray(data, static_cast<int>(len))) {
        std::cerr << "Failed to parse command from datagram" << std::endl;
        return;
      }

      std::cout << "CommandReceiver: received command " << command.DebugString() << std::endl;
      dispatch_command(command);
    } catch (const std::exception &e) {
      std::cerr << "CommandReceiver error: " << e.what() << std::endl;
    }
  }

protected:
  template <typename CommandT> void dispatch_command(const CommandT &cmd) {
    static_cast<Derived *>(this)->on_command(cmd);
  }
};
