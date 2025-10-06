#pragma once

#include "core/multicast_receiver.hpp"
#include "generated/messages.pb.h"
#include <cstdint>
#include <iostream>

template <typename Derived> class CommandReceiver : public MulticastReceiver {
public:
  explicit CommandReceiver(const std::string &multicast_address, uint16_t port)
      : MulticastReceiver(multicast_address, port) {}

  virtual ~CommandReceiver() = default;

  void start() { MulticastReceiver::start(); }

  void stop() { MulticastReceiver::stop(); }

  template <typename CommandT> void subscribe(toysequencer::MessageType msg_type) {
    MulticastReceiver::subscribe(
        std::function<void(const uint8_t *data, size_t len)>([this, msg_type](const uint8_t *data, size_t len) {
          // Peek at the first 4 bytes (protobuf varint for enum is usually 1 byte, but proto3 fields are fixed order)
          // In our .proto, msg_type is field 1, which is a varint (enum), so it's the first field.
          // We can parse the first byte(s) to extract the field 1 value.
          // Protobuf wire format: <field_number << 3 | wire_type>, then value.
          // For field 1 (msg_type), tag = 1 << 3 | 0 = 8 (0x08), then value (varint)
          if (len >= 2 && data[0] == 0x08) { // 0x08 is the tag for field 1, varint
            // read varint (should be 1 byte for our enum values)
            uint8_t msg_type_val = data[1];
            if (msg_type_val == static_cast<uint8_t>(msg_type)) {
              on_datagram<CommandT>(data, len);
            }
          }
        }));
  }

  template <typename CommandT> void on_datagram(const uint8_t *data, size_t len) {
    try {
      CommandT command;
      if (!command.ParseFromArray(data, static_cast<int>(len))) {
        std::cerr << "Failed to parse command from datagram" << std::endl;
        return;
      }

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
