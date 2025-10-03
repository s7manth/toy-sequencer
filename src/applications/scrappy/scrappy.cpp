#include "scrappy.hpp"
#include <iostream>

ScrappyApp::ScrappyApp(const std::string &output_file)
    : EventReceiver<ScrappyApp>(0), output_filename_(output_file) {
  output_file_.open(output_file, std::ios::out | std::ios::app);
  if (!output_file_.is_open()) {
    std::cerr << "Failed to open output file: " << output_file << std::endl;
  }
}

void ScrappyApp::on_event(const toysequencer::TextEvent &event) {
  if (output_file_.is_open()) {
    // format: Sequence Number|SID|TIN|Payload
    output_file_ << "#=" << event.seq() << "|" << "SID=" << event.sid() << "|"
                 << "TIN=" << event.tin() << "|" << "TEXT=" << event.text()
                 << std::endl;
    output_file_.flush(); // ensure immediate write to file
  }
}

void ScrappyApp::on_event(const toysequencer::TopOfBookEvent &event) {
  std::cout << "Scrappy: on_event(TopOfBookEvent)" << std::endl;
  if (output_file_.is_open()) {
    output_file_ << "#=" << event.seq() << "|" << "SID=" << event.sid() << "|"
                 << "TIN=" << event.tin() << "|" << "SYMBOL=" << event.symbol()
                 << "|" << "BID_PRICE=" << event.bid_price() << "|"
                 << "BID_SIZE=" << event.bid_size() << "|"
                 << "ASK_PRICE=" << event.ask_price() << "|"
                 << "ASK_SIZE=" << event.ask_size() << std::endl;
    output_file_.flush(); // ensure immediate write to file
  }
}

void ScrappyApp::on_datagram(const uint8_t *data, size_t len) {
  try {
    // Try parse as TopOfBookEvent first (more fields)
    {
      toysequencer::TopOfBookEvent tob_ev;
      if (tob_ev.ParseFromArray(data, static_cast<int>(len))) {
        on_event(tob_ev);
        return;
      }
    }

    // Fallback: try parse as TextEvent
    {
      toysequencer::TextEvent text_ev;
      if (text_ev.ParseFromArray(data, static_cast<int>(len))) {
        on_event(text_ev);
        return;
      }
    }

    std::cerr << "Scrappy: unknown datagram payload format" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Scrappy error: " << e.what() << std::endl;
  }
}
