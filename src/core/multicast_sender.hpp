#pragma once

#include "sender_iface.hpp"
#include <cstdint>
#include <string>
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

class MulticastSender : public ISender {
public:
  MulticastSender(const std::string &multicast_address,
                  uint16_t port, uint8_t ttl);

  ~MulticastSender();

  bool send_m(const std::vector<uint8_t> &data) override;
  bool send_m(const std::vector<uint8_t> &data, uint8_t ttl) override;

  std::string get_address() const { return multicast_address_; }
  uint16_t get_port() const { return port_; }

private:
  void setup_socket();
  void cleanup_socket();

  std::string multicast_address_;
  uint16_t port_;
  uint8_t ttl_;

#ifdef _WIN32
  SOCKET socket_;
  WSADATA wsa_data_;
#else
  int socket_;
#endif

  struct sockaddr_in multicast_addr_;
};
