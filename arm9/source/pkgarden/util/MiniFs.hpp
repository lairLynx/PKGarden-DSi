#pragma once
// Tiny stand-in for the parts of std::filesystem that the chest code uses, for targets that do not
// have <filesystem> (the Nintendo DSi build). Enabled with -DPKG_NO_STD_FILESYSTEM.
//
// FAT cannot rename over an existing file, so rename() removes the destination first. That is not
// atomic: a crash in between leaves the destination missing. LocalChestStore::open() and
// chest recovery are written to cope with exactly that (see chest/Journal.hpp).
#include <sys/stat.h>

#include <cstdio>
#include <string>
#include <system_error>

namespace pkg::minifs {

class path {
public:
    path() = default;
    path(const char* s) : s_(s) {}
    path(std::string s) : s_(std::move(s)) {}

    const std::string& string() const { return s_; }
    bool empty() const { return s_.empty(); }
    path& operator+=(const char* s) { s_ += s; return *this; }
    path& operator+=(const std::string& s) { s_ += s; return *this; }
    path parent_path() const
    {
        auto i = s_.find_last_of('/');
        return i == std::string::npos ? path() : path(s_.substr(0, i));
    }
    friend path operator/(const path& a, const path& b)
    {
        if (a.s_.empty()) return b;
        if (a.s_.back() == '/') return path(a.s_ + b.s_);
        return path(a.s_ + "/" + b.s_);
    }

private:
    std::string s_;
};

inline bool exists(const path& p)
{
    struct stat st;
    return ::stat(p.string().c_str(), &st) == 0;
}

inline bool create_directories(const path& p)
{
    const std::string& s = p.string();
    for (size_t i = 1; i <= s.size(); i++) {
        if (i == s.size() || s[i] == '/') {
            std::string part = s.substr(0, i);
            // "sd:" style drive prefixes cannot be created; mkdir just fails and we move on
            if (!part.empty() && part.back() != ':') ::mkdir(part.c_str(), 0777);
        }
    }
    return exists(p);
}

inline void rename(const path& from, const path& to, std::error_code& ec)
{
    ec.clear();
    if (exists(to)) std::remove(to.string().c_str());
    if (std::rename(from.string().c_str(), to.string().c_str()) != 0) ec = std::make_error_code(std::errc::io_error);
}

inline bool remove(const path& p, std::error_code& ec)
{
    ec.clear();
    return std::remove(p.string().c_str()) == 0;
}

}  // namespace pkg::minifs
