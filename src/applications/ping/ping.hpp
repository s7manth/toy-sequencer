#pragma once

#include "core/command_bus.hpp"
#include "core/command_sender.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <cstdint>
#include <functional>
#include <string>

class PingApp : public ICommandSender<PingApp>, public EventReceiver<PingApp> {
public:
  PingApp(const std::string &multicast_address, uint16_t port, uint8_t ttl,
          CommandBus &bus, std::function<void(const std::string &)> log,
          uint64_t instance_id, uint64_t pong_instance_id);
  ~PingApp() = default;

  // callbacks
  void on_event(const toysequencer::TextEvent &event);

  void send_command(const toysequencer::TextCommand &command,
                    const uint64_t sender_id);

private:
  CommandBus &bus_;
  std::function<void(const std::string &)> log_;
  uint64_t pong_instance_id_;
};
