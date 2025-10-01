#include "multicast_sender.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

MulticastSender::MulticastSender(const std::string &multicast_address,
                                 uint16_t port, uint8_t ttl)
    : multicast_address_(multicast_address), port_(port), ttl_(ttl)
#ifdef _WIN32
      ,
      socket_(INVALID_SOCKET)
#else
      ,
      socket_(-1)
#endif
{
#ifdef _WIN32
  // Initialize Winsock
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0) {
    throw std::runtime_error("WSAStartup failed");
  }
#endif

  setup_socket();
}

MulticastSender::~MulticastSender() {
  cleanup_socket();
#ifdef _WIN32
  WSACleanup();
#endif
}

void MulticastSender::setup_socket() {
  try {
    // Create UDP socket
#ifdef _WIN32
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
      throw std::runtime_error("Failed to create socket");
    }
#else
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ < 0) {
      throw std::runtime_error("Failed to create socket");
    }
#endif

    // Set up multicast address
    memset(&multicast_addr_, 0, sizeof(multicast_addr_));
    multicast_addr_.sin_family = AF_INET;
    multicast_addr_.sin_port = htons(port_);

    if (inet_pton(AF_INET, multicast_address_.c_str(),
                  &multicast_addr_.sin_addr) <= 0) {
      throw std::runtime_error("Invalid multicast address");
    }

    // Set TTL for multicast
    if (setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL,
                   reinterpret_cast<const char *>(&ttl_), sizeof(ttl_)) < 0) {
      throw std::runtime_error("Failed to set multicast TTL");
    }

    // Enable loopback so local subscribers on the same host receive packets
    unsigned char loop = 1;
    if (setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_LOOP,
                   reinterpret_cast<const char *>(&loop), sizeof(loop)) < 0) {
      throw std::runtime_error("Failed to enable multicast loopback");
    }

    // Allow multiple sockets to bind to the same address
    int reuse = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char *>(&reuse), sizeof(reuse)) < 0) {
      throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

  } catch (const std::exception &e) {
    cleanup_socket();
    throw;
  }
}

void MulticastSender::cleanup_socket() {
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
}

bool MulticastSender::send(const std::vector<uint8_t> &data) {
  return send(data, ttl_);
}

bool MulticastSender::send(const std::vector<uint8_t> &data, uint8_t ttl) {
  try {
    // Set TTL for this specific send if different from default
    if (ttl != ttl_) {
      if (setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL,
                     reinterpret_cast<const char *>(&ttl), sizeof(ttl)) < 0) {
        std::cerr << "Failed to set TTL for send" << std::endl;
        return false;
      }
    }

    // Send the data
    ssize_t bytes_sent = sendto(
        socket_, reinterpret_cast<const char *>(data.data()), data.size(), 0,
        reinterpret_cast<const struct sockaddr *>(&multicast_addr_),
        sizeof(multicast_addr_));

    // Restore original TTL if we changed it
    if (ttl != ttl_) {
      if (setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL,
                     reinterpret_cast<const char *>(&ttl_), sizeof(ttl_)) < 0) {
        std::cerr << "Failed to restore TTL after send" << std::endl;
      }
    }

    return bytes_sent == static_cast<ssize_t>(data.size());

  } catch (const std::exception &e) {
    std::cerr << "Failed to send multicast message: " << e.what() << std::endl;
    return false;
  }
}
