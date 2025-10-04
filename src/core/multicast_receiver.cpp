#include "multicast_receiver.hpp"
#include <stdexcept>

MulticastReceiver::MulticastReceiver(const std::string &multicast_address, uint16_t port)
    : multicast_address_(multicast_address), port_(port) {}

MulticastReceiver::~MulticastReceiver() { stop(); }

void MulticastReceiver::subscribe(DatagramHandler handler) {
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  handlers_.push_back(std::move(handler));
}

void MulticastReceiver::start() {
  if (running_.exchange(true)) {
    return;
  }
  worker_ = std::thread([this] { this->run_loop(); });
}

void MulticastReceiver::stop() {
  if (!running_.exchange(false)) {
    return;
  }
#ifdef _WIN32
  if (socket_ != INVALID_SOCKET) {
    closesocket(socket_);
    socket_ = INVALID_SOCKET;
  }
#else
  if (socket_ >= 0) {
    close(socket_);
    socket_ = -1;
  }
#endif
  if (worker_.joinable()) {
    worker_.join();
  }
}

void MulticastReceiver::run_loop() {
#ifdef _WIN32
  socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_ == INVALID_SOCKET) {
    throw std::runtime_error("Failed to create UDP socket");
  }
#else
  socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_ < 0) {
    throw std::runtime_error("Failed to create UDP socket");
  }
#endif

  int reuse = 1;
  setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));

  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_port = htons(port_);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(socket_, reinterpret_cast<sockaddr *>(&local), sizeof(local)) < 0) {
    throw std::runtime_error("Failed to bind UDP socket");
  }

  ip_mreq mreq{};
  mreq.imr_multiaddr.s_addr = inet_addr(multicast_address_.c_str());
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char *>(&mreq), sizeof(mreq)) < 0) {
    throw std::runtime_error("Failed to join multicast group");
  }

  std::vector<uint8_t> buffer(64 * 1024);
  while (running_.load()) {
    sockaddr_in src{};
#ifdef _WIN32
    int srclen = sizeof(src);
    int n = recvfrom(socket_, reinterpret_cast<char *>(buffer.data()), static_cast<int>(buffer.size()), 0,
                     reinterpret_cast<sockaddr *>(&src), &srclen);
#else
    socklen_t srclen = sizeof(src);
    ssize_t n = recvfrom(socket_, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr *>(&src), &srclen);
#endif
    if (n <= 0) {
      continue;
    }

    std::vector<DatagramHandler> copy;
    {
      std::lock_guard<std::mutex> lock(handlers_mutex_);
      copy = handlers_;
    }
    for (auto &h : copy) {
      h(buffer.data(), static_cast<size_t>(n));
    }
  }

  setsockopt(socket_, IPPROTO_IP, IP_DROP_MEMBERSHIP, reinterpret_cast<const char *>(&mreq), sizeof(mreq));
}
