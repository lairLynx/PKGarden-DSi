#include "chest/Envelope.hpp"

#include <cstring>

#include "util/Crc32.hpp"
#include "util/Files.hpp"

namespace pkg {

static uint16_t rd16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
static void wr16(uint8_t* p, uint16_t v)
{
    p[0] = v & 0xFF;
    p[1] = static_cast<uint8_t>(v >> 8);
}
static void wr32(uint8_t* p, uint32_t v)
{
    for (int i = 0; i < 4; i++) p[i] = (v >> (8 * i)) & 0xFF;
}

Slot emptySlot() { return Slot{}; }
bool isEmpty(const Slot& s) { return s[0] == 0; }
Format format(const Slot& s) { return static_cast<Format>(s[0]); }
uint8_t flags(const Slot& s) { return s[1]; }
uint16_t payloadLen(const Slot& s) { return rd16(&s[2]); }
const uint8_t* payload(const Slot& s) { return &s[16]; }

std::optional<std::string> validateSlot(const uint8_t* s, size_t len)
{
    if (len != kSlotSize) return "envelope must be 336 bytes";
    uint8_t fmt = s[0];
    unsigned payload = rd16(s + 2), extra = rd16(s + 4);
    if (fmt > 5) return "unknown format";
    int expected = fmt == 0 ? 0 : fmt == 3 ? 80 : (fmt == 4 || fmt == 5) ? 136 : -1;
    if (expected >= 0 && payload != static_cast<unsigned>(expected)) return "bad payload length";
    if ((fmt == 1 || fmt == 2) && payload == 0) return "bad payload length";
    if (fmt == 0 && extra != 0) return "empty slot with extra data";
    if (16 + payload + extra > kSlotSize) return "lengths exceed envelope";
    return std::nullopt;
}

Slot makeSlot(Format f, const uint8_t* data, size_t len, uint8_t fl)
{
    if (len > kSlotSize - 16) throw PKError("payload too large");
    Slot s{};
    s[0] = static_cast<uint8_t>(f);
    s[1] = fl;
    wr16(&s[2], static_cast<uint16_t>(len));
    if (auto err = validateSlot(s.data(), s.size())) throw PKError("Invalid envelope: " + *err);
    if (len) {
        if (!data) throw PKError("Missing payload");
        std::memcpy(&s[16], data, len);
    }
    return s;
}

void writeBoxBlob(const Slot* slots, uint8_t* out)
{
    std::memset(out, 0, kBoxHeader);
    std::memcpy(out, "PGBX", 4);
    wr16(out + 4, 1);
    wr16(out + 6, kSlotSize);
    wr16(out + 8, kSlotsPerBox);
    for (size_t i = 0; i < kSlotsPerBox; i++)
        std::memcpy(out + kBoxHeader + i * kSlotSize, slots[i].data(), kSlotSize);
    wr32(out + 12, crc32(out + kBoxHeader, kSlotSize * kSlotsPerBox));
}

std::optional<std::string> validateBoxBlob(const uint8_t* d, size_t len)
{
    if (len != kBoxBlobSize) return "box blob has wrong size";
    if (std::memcmp(d, "PGBX", 4) != 0) return "bad magic";
    if (rd16(d + 4) != 1) return "unsupported format_version";
    if (rd16(d + 6) != kSlotSize || rd16(d + 8) != kSlotsPerBox) return "bad slot geometry";
    if (rd32(d + 12) != crc32(d + kBoxHeader, kSlotSize * kSlotsPerBox)) return "crc mismatch";
    for (size_t i = 0; i < kSlotsPerBox; i++)
        if (auto e = validateSlot(d + kBoxHeader + i * kSlotSize, kSlotSize))
            return "slot " + std::to_string(i) + ": " + *e;
    return std::nullopt;
}

}  // namespace pkg
