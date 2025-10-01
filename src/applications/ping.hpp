#pragma once

#include "core/command_bus.hpp"
#include "generated/messages.pb.h"
#include <functional>
#include <string>

class PingApp {
public:
  PingApp(CommandBus &bus, std::function<void(const std::string &)> log);
  ~PingApp() = default;

  // Executed by caller-managed thread/context
  void run();

  void on_event(const toysequencer::TextEvent &event);

private:
  CommandBus &bus_;
  std::function<void(const std::string &)> log_;
};
