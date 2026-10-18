#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef PKG_NO_STD_FILESYSTEM
#include "util/MiniFs.hpp"
#else
#include <filesystem>
#endif

namespace pkg {

using Bytes = std::vector<uint8_t>;
#ifdef PKG_NO_STD_FILESYSTEM
namespace fs = minifs;
#else
namespace fs = std::filesystem;
#endif

// Every user-facing failure is a PKError; its message is shown in the UI.
struct PKError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

Bytes readFile(const fs::path& p);  // throws PKError
// Writes and flushes to disk. Removes the file again if the write fails.
void writeFileSynced(const fs::path& p, const uint8_t* data, size_t len);
// Writes p.tmp, then replaces p. Never leaves a partially written p behind.
void writeFileAtomic(const fs::path& p, const uint8_t* data, size_t len);
inline void writeFileAtomic(const fs::path& p, const Bytes& b) { writeFileAtomic(p, b.data(), b.size()); }
bool removeIfExists(const fs::path& p);
// Copies src over dst atomically.
void copyFileSynced(const fs::path& src, const fs::path& dst);
// Last-modified time as an opaque, comparable tick count.
#ifndef PKG_NO_STD_FILESYSTEM
int64_t mtimeTicks(const fs::path& p);
#endif

}  // namespace pkg
