#pragma once

#include "generated/messages.pb.h"

class ICommandSender {
public:
  virtual ~ICommandSender() = default;
  virtual void send_command(const toysequencer::TextCommand &command,
                            uint64_t sender_id) = 0;
};
