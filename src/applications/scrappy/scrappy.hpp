#pragma once

#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <fstream>
#include <string>

class ScrappyApp : public EventReceiver<ScrappyApp> {
public:
  ScrappyApp(const std::string &output_file);
  ~ScrappyApp() = default;

  void on_event(const toysequencer::TextEvent &event);
  void on_event(const toysequencer::TopOfBookEvent &event);

  void on_datagram(const uint8_t *data, size_t len);

private:
  std::ofstream output_file_;
  std::string output_filename_;
};
