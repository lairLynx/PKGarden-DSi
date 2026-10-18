#pragma once
#include <cstddef>
#include <cstdint>

namespace pkg {
// IEEE 802.3 CRC-32 (same as System.IO.Hashing.Crc32 on the server and zlib).
uint32_t crc32(const uint8_t* data, size_t len, uint32_t seed = 0);
}  // namespace pkg
