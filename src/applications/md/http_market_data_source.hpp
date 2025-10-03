#pragma once

#include "imarket_data_source.hpp"
#include <atomic>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <thread>
#include <chrono>
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
    worker_ = std::thread([this]() { this->run(); });
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
    if (worker_.joinable()) {
      worker_.join();
    }
  }

private:
  void run() {
    while (running_) {
      // Resolve
      struct addrinfo hints;
      std::memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      struct addrinfo *res = nullptr;
      if (getaddrinfo(host_.c_str(), port_.c_str(), &hints, &res) != 0) {
        if (!running_) break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
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
        if (!running_) break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }
      sock_.store(fd);
      // connected
      // std::cerr << "md: connected to " << host_ << ":" << port_ << " path=" << path_ << std::endl;

      // Send HTTP GET request
      std::ostringstream req;
      req << "GET " << path_ << " HTTP/1.1\r\n";
      req << "Host: " << host_ << ":" << port_ << "\r\n";
      req << "Accept: text/event-stream\r\n";
      req << "Connection: keep-alive\r\n";
      req << "Cache-Control: no-cache\r\n\r\n";
      const std::string request = req.str();
      (void)::send(fd, request.data(), request.size(), 0);
      // std::cerr << "md: request sent" << std::endl;

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
        // Optionally check HTTP status is 200
        if (buffer.rfind("HTTP/", 0) == 0) {
          // minimal check; if not 200, drop and reconnect
          size_t sp1 = buffer.find(' ');
          if (sp1 != std::string::npos && sp1 + 4 <= buffer.size()) {
            std::string code = buffer.substr(sp1 + 1, 3);
            if (code != "200") {
              int expected2 = fd;
              if (sock_.compare_exchange_strong(expected2, -1)) {
                ::shutdown(fd, SHUT_RDWR);
                ::close(fd);
              }
              break; // outer retry loop will reconnect
            }
          }
        }
          buffer.erase(0, pos + 4);
          headers_skipped = true;
        // std::cerr << "md: headers skipped" << std::endl;
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

          // case-insensitive prefix match for "data:"
          if (line.size() >= 5) {
            bool is_data = (line[0] == 'd' || line[0] == 'D') &&
                           (line[1] == 'a' || line[1] == 'A') &&
                           (line[2] == 't' || line[2] == 'T') &&
                           (line[3] == 'a' || line[3] == 'A') &&
                           line[4] == ':';
            if (is_data) {
            size_t p = 5;
            if (p < line.size() && line[p] == ' ')
              ++p;
            std::string json = line.substr(p);
              try {
                on_top_of_book(json);
              } catch (...) {
                // swallow to avoid terminating the worker
              }
            }
          }
        }
      }

      int expected = fd;
      if (sock_.compare_exchange_strong(expected, -1)) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
      }

      if (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
    running_.store(false);
  }

  std::string host_;
  std::string port_;
  std::string path_;
  std::atomic<bool> running_{false};
  std::atomic<int> sock_{-1};
  std::thread worker_{};
};
