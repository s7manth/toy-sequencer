#include "core/command_bus.hpp"
#include "core/command_sender.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <functional>
#include <string>

class PongApp : public ICommandSender, public EventReceiver {
public:
  PongApp(CommandBus &bus, std::function<void(const std::string &)> log,
          uint64_t instance_id, uint64_t ping_instance_id);

  // EventReceiver
  void on_event(const toysequencer::TextEvent &event) override;

  // ICommandSender
  void send_command(const toysequencer::TextCommand &command,
                    uint64_t sender_id) override;

private:
  CommandBus &bus_;
  std::function<void(const std::string &)> log_;
  uint64_t ping_instance_id_;
};
