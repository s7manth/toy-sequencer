#pragma once

#include "core/multicast_receiver.hpp"
#include <cstdint>
#include <iostream>
#include <vector>

template <typename Derived> class EventReceiver: public MulticastReceiver {
public:
  explicit EventReceiver(
    uint64_t instance_id, 
    const std::string &multicast_address, 
    uint16_t port) : 
    instance_id_(instance_id), 
    MulticastReceiver(multicast_address, port) {}

  virtual ~EventReceiver() = default;

  void start() {
    MulticastReceiver::start();
  }

  void stop() {
    MulticastReceiver::stop();
  }

  template <typename EventT>
  void subscribe() {
    MulticastReceiver::subscribe(std::function<void(const uint8_t *data, size_t len)>([this](const uint8_t *data, size_t len) {
      on_datagram<EventT>(data, len);
    }));
  }

  template <typename EventT> void on_datagram(const uint8_t *data, size_t len) {
    try {
      EventT event;
      if (!event.ParseFromArray(data, static_cast<int>(len))) {
        std::cerr << "Failed to parse event from datagram" << std::endl;
        return;
      }

      std::cout << "EventReceiver: on_datagram seq=" << event.seq() << ", expected_seq=" << expected_seq_ << std::endl;

      if (event.seq() < expected_seq_) {
        return;
      }

      if (event.seq() == expected_seq_) {
        dispatch_event(event);
        expected_seq_++;
        return;
      }

      std::cout << "Out of order event received. Expected: " << expected_seq_
                << ", Got: " << event.seq() << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "EventReceiver error: " << e.what() << std::endl;
    }
  }

protected:
  uint64_t get_instance_id() const { return instance_id_; }
  template <typename EventT> void dispatch_event(const EventT &ev) {
    static_cast<Derived *>(this)->on_event(ev);
  }

private:
  uint64_t expected_seq_ = 1;
  uint64_t instance_id_;
};
