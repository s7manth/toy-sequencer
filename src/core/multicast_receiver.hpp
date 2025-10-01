#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

class MulticastReceiver {
public:
  using DatagramHandler = std::function<void(const uint8_t *data, size_t len)>;

  MulticastReceiver(const std::string &multicast_address, uint16_t port);
  ~MulticastReceiver();

  void subscribe(DatagramHandler handler);

  void start();
  void stop();

  std::string get_address() const { return multicast_address_; }
  uint16_t get_port() const { return port_; }

private:
  void run_loop();

  std::string multicast_address_;
  uint16_t port_;

  std::mutex handlers_mutex_;
  std::vector<DatagramHandler> handlers_;

  std::atomic<bool> running_{false};
  std::thread worker_;

#ifdef _WIN32
  SOCKET socket_ = INVALID_SOCKET;
#else
  int socket_ = -1;
#endif
};
