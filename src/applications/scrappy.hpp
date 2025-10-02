#pragma once

#include "generated/messages.pb.h"
#include <fstream>
#include <string>

class ScrappyApp {
public:
  ScrappyApp(const std::string &output_file);
  ~ScrappyApp() = default;

  // Method to handle incoming events (bypasses TIN filtering)
  void on_event(const toysequencer::TextEvent &event);

  // Method to handle raw datagrams (for direct multicast subscription)
  void on_datagram(const uint8_t *data, size_t len);

private:
  std::ofstream output_file_;
  std::string output_filename_;
};
