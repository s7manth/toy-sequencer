#include "inproc_multicast.hpp"

bool InprocMulticastSender::send(const std::vector<uint8_t> &data) {
  std::vector<Subscriber> copy;
  {
    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    copy = subscribers_;
  }
  for (auto &s : copy) {
    s(data);
  }
  return true;
}

bool InprocMulticastSender::send(const std::vector<uint8_t> &data, uint8_t) {
  return send(data);
}

void InprocMulticastSender::register_subscriber(Subscriber subscriber) {
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  subscribers_.push_back(std::move(subscriber));
}
