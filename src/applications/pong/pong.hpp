#pragma once

#include "core/command_sender.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <cstdint>
#include <functional>
#include <string>

class PongApp : public ICommandSender<PongApp>, public EventReceiver<PongApp> {
public:
  PongApp(const std::string &multicast_address, const uint16_t port, uint8_t ttl,
          const std::string &events_multicast_address, const uint16_t events_port,
          const std::function<void(const std::string &)> log, const uint64_t instance_id,
          const uint64_t ping_instance_id);

  // callbacks
  void on_event(const toysequencer::TextEvent &event);

  void send_command(const toysequencer::TextCommand &command, const uint64_t sender_id);

  void start() { EventReceiver<PongApp>::start(); }

  void stop() { EventReceiver<PongApp>::stop(); }

private:
  std::function<void(const std::string &)> log_;
  uint64_t ping_instance_id_;
};
