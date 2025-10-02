#include "scrappy.hpp"
#include <iostream>

ScrappyApp::ScrappyApp(const std::string &output_file)
    : output_filename_(output_file) {
  output_file_.open(output_file, std::ios::out | std::ios::app);
  if (!output_file_.is_open()) {
    std::cerr << "Failed to open output file: " << output_file << std::endl;
  }
}

void ScrappyApp::on_event(const toysequencer::TextEvent &event) {
  if (output_file_.is_open()) {
    // Format: Sequence Number|SID|TIN|Payload
    output_file_ << "#=" << event.seq() << "|" << "SID=" << event.sid() << "|"
                 << "TIN=" << event.tin() << "|" << "TEXT=" << event.text()
                 << std::endl;
    output_file_.flush(); // Ensure immediate write to file
  }
}

void ScrappyApp::on_datagram(const uint8_t *data, size_t len) {
  try {
    toysequencer::TextEvent event;
    if (!event.ParseFromArray(data, static_cast<int>(len))) {
      std::cerr << "Failed to parse TextEvent from datagram" << std::endl;
      return;
    }

    on_event(event);

  } catch (const std::exception &e) {
    std::cerr << "Scrappy error: " << e.what() << std::endl;
  }
}
