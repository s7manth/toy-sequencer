#pragma once

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

class MulticastSender {
public:
  MulticastSender(const std::string &multicast_address = "239.255.0.1",
                  uint16_t port = 30001, uint8_t ttl = 1);

  ~MulticastSender();

  // Send a message to the multicast group
  bool send(const std::vector<uint8_t> &data);

  // Send a message with specific TTL
  bool send(const std::vector<uint8_t> &data, uint8_t ttl);

  // Get multicast address and port
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
