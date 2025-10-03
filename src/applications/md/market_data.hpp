#pragma once

#include "imarket_data_source.hpp"
#include <atomic>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// HTTP Server-Sent Events (SSE) market data source. Connects to host:port/path
// and expects lines like: "data: {json}" terminated by blank line between
// events.
class HttpSseMarketDataSource : public IMarketDataSource {
public:
  HttpSseMarketDataSource(std::string host, std::string port, std::string path)
      : host_(std::move(host)), port_(std::move(port)), path_(std::move(path)) {}

  void start() override {
    if (running_.exchange(true))
      return;
    worker_ = std::thread([this] { this->run(); });
  }

  void stop() override {
    if (!running_.exchange(false))
      return;
    // Closing socket will unblock recv
    int fd = sock_.exchange(-1);
    if (fd != -1) {
      ::shutdown(fd, SHUT_RDWR);
      ::close(fd);
    }
    if (worker_.joinable())
      worker_.join();
  }

private:
  void run() {
    // Resolve
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res = nullptr;
    if (getaddrinfo(host_.c_str(), port_.c_str(), &hints, &res) != 0) {
      return;
    }
    int fd = -1;
    for (auto *p = res; p != nullptr; p = p->ai_next) {
      fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (fd == -1)
        continue;
      if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
        break;
      }
      ::close(fd);
      fd = -1;
    }
    freeaddrinfo(res);
    if (fd == -1) {
      return;
    }
    sock_.store(fd);

    // Send HTTP GET request
    std::ostringstream req;
    req << "GET " << path_ << " HTTP/1.1\r\n";
    req << "Host: " << host_ << ":" << port_ << "\r\n";
    req << "Accept: text/event-stream\r\n";
    req << "Connection: keep-alive\r\n";
    req << "Cache-Control: no-cache\r\n\r\n";
    const std::string request = req.str();
    (void)::send(fd, request.data(), request.size(), 0);

    // Read loop
    std::string buffer;
    buffer.reserve(8192);
    std::vector<char> tmp(4096);
    bool headers_skipped = false;
    while (running_) {
      ssize_t n = ::recv(fd, tmp.data(), tmp.size(), 0);
      if (n <= 0)
        break;
      buffer.append(tmp.data(), static_cast<size_t>(n));

      if (!headers_skipped) {
        size_t pos = buffer.find("\r\n\r\n");
        if (pos == std::string::npos) {
          continue;
        }
        buffer.erase(0, pos + 4);
        headers_skipped = true;
      }

      // Process by lines
      size_t line_start = 0;
      for (;;) {
        size_t nl = buffer.find('\n', line_start);
        if (nl == std::string::npos) {
          // keep partial line
          buffer.erase(0, line_start);
          break;
        }
        std::string line = buffer.substr(line_start, nl - line_start);
        if (!line.empty() && line.back() == '\r')
          line.pop_back();
        line_start = nl + 1;

        if (line.rfind("data:", 0) == 0) {
          size_t p = 5;
          if (p < line.size() && line[p] == ' ')
            ++p;
          std::string json = line.substr(p);
          on_top_of_book(json);
        }
      }
    }

    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
  }

  std::string host_;
  std::string port_;
  std::string path_;
  std::thread worker_;
  std::atomic<bool> running_{false};
  std::atomic<int> sock_{-1};
};
