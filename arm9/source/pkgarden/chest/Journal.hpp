#pragma once
// Crash safety for "write the .sav, then publish the chest" (PLAN.md Phase 1 save flow).
//
// The chest and the save are two files, so a crash between the two writes must never lose or
// duplicate a Pokemon. Before the save is written we record what the save will look like afterwards;
// at the next start recover() compares the save on disk with that record:
//   save matches "after"  -> the save write finished: commit the staged chest
//   anything else         -> cancel (drop the staged chest). If the user changed the save in the
//                            meantime this can duplicate a Pokemon, never delete one.
#include <optional>
#include <string>

#include "chest/LocalChestStore.hpp"

namespace pkg {

struct JournalRecord {
    std::string savePath;
    uint32_t beforeCrc = 0;
    uint32_t afterCrc = 0;
    uint64_t afterSize = 0;
    uint32_t chestCrc = 0;  // CRC of the staged chest file
};

class Journal {
public:
    explicit Journal(fs::path p) : path_(std::move(p)) {}
    bool exists() const { return fs::exists(path_); }
    void write(const JournalRecord& r) const;
    std::optional<JournalRecord> read() const;  // nullopt when absent or unreadable
    void clear() const { removeIfExists(path_); }
    const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

enum class Recovery { Nothing, Committed, Cancelled };

struct RecoveryResult {
    Recovery action = Recovery::Nothing;
    std::string message;
};

// Call once at startup, before the chest is shown.
RecoveryResult recover(const Journal& journal, LocalChestStore& chest);

}  // namespace pkg
