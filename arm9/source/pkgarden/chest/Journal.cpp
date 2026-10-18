#include "chest/Journal.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <map>
#include <sstream>

#include "util/Crc32.hpp"

namespace pkg {

void Journal::write(const JournalRecord& r) const
{
    char buf[512];
    std::snprintf(buf, sizeof buf, "v=1\nbefore=%08x\nafter=%08x\nsize=%llu\nchest=%08x\n", r.beforeCrc, r.afterCrc,
                  static_cast<unsigned long long>(r.afterSize), r.chestCrc);
    std::string body = buf;
    body += "save=" + r.savePath + "\n";
    body += "end=1\n";
    writeFileAtomic(path_, reinterpret_cast<const uint8_t*>(body.data()), body.size());
}

std::optional<JournalRecord> Journal::read() const
{
    if (!exists()) return std::nullopt;
    Bytes b;
    try { b = readFile(path_); } catch (const PKError&) { return std::nullopt; }
    std::istringstream in(std::string(b.begin(), b.end()));
    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto eq = line.find('=');
        if (eq != std::string::npos) kv[line.substr(0, eq)] = line.substr(eq + 1);
    }
    if (kv["v"] != "1" || kv["end"] != "1" || kv["save"].empty()) return std::nullopt;
    JournalRecord r;
    r.savePath = kv["save"];
    r.beforeCrc = static_cast<uint32_t>(std::strtoul(kv["before"].c_str(), nullptr, 16));
    r.afterCrc = static_cast<uint32_t>(std::strtoul(kv["after"].c_str(), nullptr, 16));
    r.afterSize = std::strtoull(kv["size"].c_str(), nullptr, 10);
    r.chestCrc = static_cast<uint32_t>(std::strtoul(kv["chest"].c_str(), nullptr, 16));
    return r;
}

RecoveryResult recover(const Journal& journal, LocalChestStore& chest)
{
    RecoveryResult res;
    auto rec = journal.read();
    if (!rec) {
        // No (valid) journal: any staged chest was never tied to a save write, so it is stale.
        bool hadJournalFile = journal.exists();
        bool hadStaged = fs::exists(chest.stagedPath());
        chest.discardStagedFile();
        journal.clear();
        if (hadJournalFile || hadStaged) {
            res.action = Recovery::Cancelled;
            res.message = "An unfinished save was discarded.";
        }
        return res;
    }

    bool saveIsAfter = false;
    try {
        Bytes save = readFile(rec->savePath);
        saveIsAfter = save.size() == rec->afterSize && crc32(save.data(), save.size()) == rec->afterCrc;
    } catch (const PKError&) {
    }

    // Crash after the chest was already published but before the journal was cleared.
    bool chestAlreadyPublished = false;
    if (!fs::exists(chest.stagedPath())) {
        try {
            Bytes cur = readFile(chest.path());
            chestAlreadyPublished = crc32(cur.data(), cur.size()) == rec->chestCrc;
        } catch (const PKError&) {
        }
    }

    if (saveIsAfter && (chestAlreadyPublished || chest.commitStagedFile(rec->chestCrc))) {
        res.action = Recovery::Committed;
        res.message = "Finished an interrupted save.";
    } else {
        chest.discardStagedFile();
        res.action = Recovery::Cancelled;
        res.message = saveIsAfter ? "Interrupted save could not be completed; chest left unchanged."
                                  : "Interrupted save was not written; chest left unchanged.";
    }
    journal.clear();
    return res;
}

}  // namespace pkg
