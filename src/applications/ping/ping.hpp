#pragma once

#include "../application.hpp"
#include "core/command_sender.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <cstdint>
#include <functional>
#include <string>

class PingApp : public Application, public ICommandSender<PingApp>, public EventReceiver<PingApp> {
public:
  PingApp(const std::string &commands_multicast_address, const uint16_t commands_port, const uint8_t ttl,
          const std::string &events_multicast_address, const uint16_t events_port,
          const std::function<void(const std::string &)> log);
  ~PingApp() = default;

  // callbacks
  void on_event(const toysequencer::TextEvent &event);

  void send_command(const toysequencer::TextCommand &command, const uint64_t sender_id);

  void start() override;

  void stop() override;

  uint64_t get_instance_id() const override;

private:
  std::function<void(const std::string &)> log_;
  uint64_t pong_instance_id_;
};
