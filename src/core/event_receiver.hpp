#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

template <typename Derived> class EventReceiver {
public:
  explicit EventReceiver(uint64_t instance_id) : instance_id_(instance_id) {}
  virtual ~EventReceiver() = default;

  template <typename EventT> void on_datagram(const uint8_t *data, size_t len) {
    try {
      EventT event;
      if (!event.ParseFromArray(data, static_cast<int>(len))) {
        std::cerr << "Failed to parse event from datagram" << std::endl;
        return;
      }

      if (event.tin() != instance_id_) {
        return;
      }

      // Initialize expected sequence based on the first event we see for this
      // instance. Sequences are global, so the first targeted event may not
      // be 1.
      if (expected_seq_ == 0) {
        dispatch_event(event);
        expected_seq_ = event.seq() + 1;
        return;
      }

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
      dispatch_event(event);

    } catch (const std::exception &e) {
      std::cerr << "EventReceiver error: " << e.what() << std::endl;
    }
  }

  template <typename EventT> void on_bytes(const std::vector<uint8_t> &bytes) {
    on_datagram<EventT>(bytes.data(), bytes.size());
  }

  template <typename EventT> void receive(const EventT &ev) {
    dispatch_event(ev);
  }

protected:
  uint64_t get_instance_id() const { return instance_id_; }
  template <typename EventT> void dispatch_event(const EventT &ev) {
    static_cast<Derived *>(this)->on_event(ev);
  }

private:
  uint64_t expected_seq_ = 0;
  uint64_t instance_id_;
};
