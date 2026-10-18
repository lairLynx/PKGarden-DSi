#include "util/Crc32.hpp"

#include <array>

namespace pkg {
namespace {
std::array<uint32_t, 256> makeTable()
{
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        t[i] = c;
    }
    return t;
}
}  // namespace

uint32_t crc32(const uint8_t* data, size_t len, uint32_t seed)
{
    static const auto table = makeTable();
    uint32_t c = ~seed;
    for (size_t i = 0; i < len; i++) c = table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    return ~c;
}
}  // namespace pkg
