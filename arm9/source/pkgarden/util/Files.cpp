#include "util/Files.hpp"

#include <cstdio>
#include <system_error>

#ifdef _WIN32
#include <io.h>
#elif !defined(__NDS__)
#include <unistd.h>
#endif

namespace pkg {

// Flushes the stdio buffer and asks the OS to put the data on the medium.
static bool syncFile(FILE* f)
{
    if (std::fflush(f) != 0) return false;
#if defined(_WIN32)
    return _commit(_fileno(f)) == 0;
#elif defined(__NDS__)
    return true;  // libfat writes through on fclose
#else
    return fsync(fileno(f)) == 0;
#endif
}

Bytes readFile(const fs::path& p)
{
    FILE* f = std::fopen(p.string().c_str(), "rb");
    if (!f) throw PKError("Cannot open " + p.string());
    Bytes out;
    // Read at the exact size: growing a vector by doubling needs up to 3x the file size at once,
    // which matters on a console with a few MB of RAM.
    if (std::fseek(f, 0, SEEK_END) == 0) {
        long size = std::ftell(f);
        std::rewind(f);
        if (size > 0) out.resize(static_cast<size_t>(size));
    }
    size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    out.resize(got);
    // anything past the size we measured (a file that grew) or an unseekable file
    uint8_t buf[4096];  // small: the DSi stack is tiny
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) out.insert(out.end(), buf, buf + n);
    bool bad = std::ferror(f) != 0;
    std::fclose(f);
    if (bad) throw PKError("Read error: " + p.string());
    return out;
}

void writeFileSynced(const fs::path& p, const uint8_t* data, size_t len)
{
    FILE* f = std::fopen(p.string().c_str(), "wb");
    if (!f) throw PKError("Cannot write " + p.string());
    bool ok = len == 0 || std::fwrite(data, 1, len, f) == len;
    ok = ok && syncFile(f);
    ok = (std::fclose(f) == 0) && ok;
    if (!ok) {
        std::error_code ec;
        fs::remove(p, ec);
        throw PKError("Write failed (disk full?): " + p.string());
    }
}

void writeFileAtomic(const fs::path& p, const uint8_t* data, size_t len)
{
    fs::path tmp = p;
    tmp += ".tmp";
    writeFileSynced(tmp, data, len);
    std::error_code ec;
    fs::rename(tmp, p, ec);
    if (ec) {
        fs::remove(tmp, ec);
        throw PKError("Cannot replace " + p.string());
    }
}

bool removeIfExists(const fs::path& p)
{
    std::error_code ec;
    return fs::remove(p, ec);
}

void copyFileSynced(const fs::path& src, const fs::path& dst)
{
    Bytes b = readFile(src);
    writeFileAtomic(dst, b);
}

#ifndef PKG_NO_STD_FILESYSTEM
int64_t mtimeTicks(const fs::path& p)
{
    std::error_code ec;
    auto t = fs::last_write_time(p, ec);
    if (ec) throw PKError("Cannot stat " + p.string());
    return static_cast<int64_t>(t.time_since_epoch().count());
}
#endif

}  // namespace pkg
