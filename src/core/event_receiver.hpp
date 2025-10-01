#pragma once

#include "generated/messages.pb.h"

class IEventReceiver {
public:
  virtual ~IEventReceiver() = default;
  virtual void on_event(const toysequencer::TextEvent &event) = 0;
};
