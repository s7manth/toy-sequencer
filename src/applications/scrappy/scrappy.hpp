#pragma once

#include "../application.hpp"
#include "core/event_receiver.hpp"
#include "generated/messages.pb.h"
#include <fstream>
#include <string>

class ScrappyApp : public Application, public EventReceiver<ScrappyApp> {
public:
  ScrappyApp(const std::string &output_file, const std::string &multicast_address, const uint16_t port);
  ~ScrappyApp() = default;

  void on_event(const toysequencer::TextEvent &event);
  void on_event(const toysequencer::TopOfBookEvent &event);

  void start() override;

  void stop() override;

  uint64_t get_instance_id() const override;

private:
  std::ofstream output_file_;
  std::string output_filename_;
};
