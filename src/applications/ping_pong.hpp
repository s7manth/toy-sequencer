#pragma once

#include "core/command_bus.hpp"
#include "generated/messages.pb.h"
#include <atomic>
#include <functional>
#include <string>
#include <thread>

class PingApp {
public:
  PingApp(CommandBus &bus, std::function<void(const std::string &)> log);
  ~PingApp();

  void start();
  void stop();

  // Receiver callback for TextEvents
  void on_event(const toysequencer::TextEvent &event);

private:
  void run();

  CommandBus &bus_;
  std::function<void(const std::string &)> log_;
  std::thread thread_;
  std::atomic<bool> running_{false};
};

class PongApp {
public:
  explicit PongApp(std::function<void(const std::string &)> log);

  // Receiver callback for TextEvents
  void on_event(const toysequencer::TextEvent &event);

private:
  std::function<void(const std::string &)> log_;
};
