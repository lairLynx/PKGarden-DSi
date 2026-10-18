#pragma once
// Slot envelope and box blob, see PKGarden-Spec/SPEC.md sections 1-2.
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace pkg {

constexpr size_t kSlotSize = 336;
constexpr size_t kSlotsPerBox = 30;
constexpr size_t kBoxHeader = 16;
constexpr size_t kBoxBlobSize = kBoxHeader + kSlotSize * kSlotsPerBox;  // 10096
constexpr int kBoxCount = 99;

enum class Format : uint8_t { Empty = 0, PK1 = 1, PK2 = 2, PK3 = 3, PK4 = 4, PK5 = 5 };

using Slot = std::array<uint8_t, kSlotSize>;

Slot emptySlot();
bool isEmpty(const Slot& s);
Format format(const Slot& s);
uint8_t flags(const Slot& s);
uint16_t payloadLen(const Slot& s);
const uint8_t* payload(const Slot& s);

// Builds an envelope. Throws PKError on a bad format/length combination.
Slot makeSlot(Format f, const uint8_t* payload, size_t payloadLen, uint8_t flags = 0);

// Returns an error description, or nullopt when valid.
std::optional<std::string> validateSlot(const uint8_t* data, size_t len);

// Box blob (header + 30 envelopes).
void writeBoxBlob(const Slot* slots /*30*/, uint8_t* out /*kBoxBlobSize*/);
std::optional<std::string> validateBoxBlob(const uint8_t* data, size_t len);

}  // namespace pkg
