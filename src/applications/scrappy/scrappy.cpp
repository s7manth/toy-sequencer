#include "scrappy.hpp"
#include "utils/instanceid_utils.hpp"
#include <iostream>

ScrappyApp::ScrappyApp(const std::string &output_file, const std::string &multicast_address, const uint16_t port)
    : EventReceiver<ScrappyApp>(0, multicast_address, port), output_filename_(output_file) {
  output_file_.open(output_file, std::ios::out | std::ios::app);
  if (!output_file_.is_open()) {
    std::cerr << "Failed to open output file: " << output_file << std::endl;
  }
}

void ScrappyApp::on_event(const toysequencer::TextEvent &event) {
  if (output_file_.is_open()) {
    // format: Sequence Number|SID|TIN|Payload
    output_file_ << "#=" << event.seq() << "|" << "SID=" << event.sid() << "|"
                 << "TIN=" << event.tin() << "|" << "TEXT=" << event.text() << std::endl;
    output_file_.flush(); // ensure immediate write to file
  }
}

void ScrappyApp::on_event(const toysequencer::TopOfBookEvent &event) {
  std::cout << "Scrappy: on_event(TopOfBookEvent) seq=" << event.seq() << " sid=" << event.sid()
            << " tin=" << event.tin() << " symbol=" << event.symbol() << std::endl;
  if (output_file_.is_open()) {
    output_file_ << "#=" << event.seq() << "|" << "SID=" << event.sid() << "|"
                 << "TIN=" << event.tin() << "|" << "SYMBOL=" << event.symbol() << "|"
                 << "BID_PRICE=" << event.bid_price() << "|"
                 << "BID_SIZE=" << event.bid_size() << "|"
                 << "ASK_PRICE=" << event.ask_price() << "|"
                 << "ASK_SIZE=" << event.ask_size() << std::endl;
    output_file_.flush(); // ensure immediate write to file
  }
}

void ScrappyApp::start() { EventReceiver<ScrappyApp>::start(); }

void ScrappyApp::stop() { EventReceiver<ScrappyApp>::stop(); }

uint64_t ScrappyApp::get_instance_id() const { return InstanceIdUtils::get_instance_id("SCRAPPY"); }
