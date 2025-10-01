#pragma once

#include <cstdint>
#include <vector>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#elif defined(__linux__)
#include <endian.h>
#elif defined(_WIN32)
#include <winsock2.h>
#define htobe64(x) htonll(x)
#define be64toh(x) ntohll(x)
#else
#include <arpa/inet.h>
#endif

struct MsgHeader {
  uint64_t seq;
  uint64_t timestamp;
  uint64_t type;
  uint64_t payload_size;
};

struct Message {
  MsgHeader header;
  std::vector<uint8_t> payload;

  std::vector<uint8_t> serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(MsgHeader) + payload.size());

    MsgHeader network_header;
    network_header.seq = htobe64(header.seq);
    network_header.timestamp = htobe64(header.timestamp);
    network_header.type = htobe64(header.type);
    network_header.payload_size = htobe64(header.payload_size);

    const uint8_t *header_bytes =
        reinterpret_cast<const uint8_t *>(&network_header);
    buffer.insert(buffer.end(), header_bytes, header_bytes + sizeof(MsgHeader));
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    return buffer;
  }

  static Message deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < sizeof(MsgHeader)) {
      throw std::runtime_error("Data too small to contain header");
    }

    Message msg;

    const MsgHeader *network_header =
        reinterpret_cast<const MsgHeader *>(data.data());
    msg.header.seq = be64toh(network_header->seq);
    msg.header.timestamp = be64toh(network_header->timestamp);
    msg.header.type = be64toh(network_header->type);
    msg.header.payload_size = be64toh(network_header->payload_size);

    size_t payload_start = sizeof(MsgHeader);
    if (data.size() < payload_start + msg.header.payload_size) {
      throw std::runtime_error("Data too small for declared payload size");
    }

    msg.payload.assign(data.begin() + payload_start,
                       data.begin() + payload_start + msg.header.payload_size);

    return msg;
  }
};
