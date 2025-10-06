#include "multicast_receiver.hpp"
#include <iostream>
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

#ifdef SO_REUSEPORT
  setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char *>(&reuse), sizeof(reuse));
#endif

  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_port = htons(port_);
#if defined(__APPLE__)
  // On macOS with SO_REUSEPORT, binding to the multicast group avoids duplicate delivery
  local.sin_addr.s_addr = inet_addr(multicast_address_.c_str());
#else
  local.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
  if (bind(socket_, reinterpret_cast<sockaddr *>(&local), sizeof(local)) < 0) {
    throw std::runtime_error("Failed to bind UDP socket");
  }

  ip_mreq mreq{};
  mreq.imr_multiaddr.s_addr = inet_addr(multicast_address_.c_str());
  {
    const char *ifenv = std::getenv("MCAST_IF_ADDR");
    if (ifenv && ifenv[0] != '\0') {
      mreq.imr_interface.s_addr = inet_addr(ifenv);
    } else {
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }
  }
  if (setsockopt(socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char *>(&mreq), sizeof(mreq)) < 0) {
    throw std::runtime_error("Failed to join multicast group");
  }

  std::vector<uint8_t> buffer(64 * 1024);
  // Deduplication state (function-local, receiver thread only)
  static auto last_recv_time = std::chrono::steady_clock::time_point{};
  static uint64_t last_payload_hash = 0;
  static size_t last_payload_len = 0;
  static in_addr last_src_addr{};
  static uint16_t last_src_port = 0;
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

    // Deduplicate immediate duplicates from same src:port (common on macOS with multicast)
    auto now = std::chrono::steady_clock::now();
    // Simple 64-bit FNV-1a hash over the payload
    const uint8_t *p = buffer.data();
    size_t len = static_cast<size_t>(n);
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; ++i) {
      h ^= static_cast<uint64_t>(p[i]);
      h *= 1099511628211ULL;
    }

    uint16_t src_port = ntohs(src.sin_port);
    bool same_src = (last_src_addr.s_addr == src.sin_addr.s_addr) && (last_src_port == src_port);
    bool same_payload = (last_payload_len == len) && (last_payload_hash == h);
    bool within_window =
        (last_recv_time.time_since_epoch().count() != 0) &&
        (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_recv_time) < std::chrono::milliseconds(100));

    if (same_src && same_payload && within_window) {
      continue; // drop duplicate
    }

    last_recv_time = now;
    last_payload_hash = h;
    last_payload_len = len;
    last_src_addr = src.sin_addr;
    last_src_port = src_port;

    std::cout << "recv " << n << " bytes from " << inet_ntoa(src.sin_addr) << ":" << src_port << std::endl;

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
