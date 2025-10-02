#include "core/command_bus.hpp"
#include "core/command_sender.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <functional>
#include <string>

class PongApp : public ICommandSender, public IEventReceiver {
public:
  PongApp(CommandBus &bus, std::function<void(const std::string &)> log);

  // IEventReceiver
  void on_event(const toysequencer::TextEvent &event) override;

  // ICommandSender
  void send_command(const toysequencer::TextCommand &command) override;

private:
  CommandBus &bus_;
  std::function<void(const std::string &)> log_;
};
