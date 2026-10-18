#pragma once
#include <map>
#include <vector>

#include "chest/ChestStore.hpp"
#include "util/Files.hpp"

namespace pkg {

// Single-file chest (chest.pgc, SPEC.md section 2b).
//   chest.pgc       current chest
//   chest.pgc.new   staged by ChestTx::prepare()
//   chest.pgc.bak   previous chest, refreshed on every commit
class LocalChestStore final : public ChestStore {
public:
    // Creates an empty chest file when `path` does not exist. If the file is corrupt but the .bak is
    // valid, the .bak is used and recoveredFromBackup() returns true.
    static std::unique_ptr<LocalChestStore> open(const fs::path& path);

    int boxCount() const override { return kBoxCount; }
    const BoxMeta& meta(int box) const override;
    void setMeta(int box, const BoxMeta& m) override;
    const Slot& slot(int box, int slot) const override;
    std::unique_ptr<ChestTx> begin() override;

    bool recoveredFromBackup() const { return recovered_; }
    const fs::path& path() const { return path_; }
    fs::path stagedPath() const { return withSuffix(".new"); }
    fs::path backupPath() const { return withSuffix(".bak"); }

    // Used by journal recovery: publish an already-staged .new file (after checking its CRC), or drop it.
    bool commitStagedFile(uint32_t expectedCrc);
    void discardStagedFile();

    // Whole-image editing (used by the DSi app, which rearranges the chest freely before saving):
    // edit through these references, then stageCurrent() -> (write the save) -> publishStaged().
    // reload() throws the edits away.
    std::vector<Slot>& editSlots() { return slots_; }
    std::vector<BoxMeta>& editMeta() { return meta_; }
    uint32_t stageCurrent();  // writes chest.pgc.new from the current state; returns its CRC-32
    void publishStaged();     // chest.pgc.new -> chest.pgc (previous file kept as .bak)
    void reload();

    static Bytes serialize(const std::vector<Slot>& slots, const std::vector<BoxMeta>& meta);
    static bool parse(const Bytes& data, std::vector<Slot>& slots, std::vector<BoxMeta>& meta, std::string& error);

private:
    friend class LocalTx;
    explicit LocalChestStore(fs::path p) : path_(std::move(p)) {}
    fs::path withSuffix(const char* s) const
    {
        fs::path p = path_;
        p += s;
        return p;
    }
    static size_t index(int box, int slot);

    fs::path path_;
    std::vector<Slot> slots_;
    std::vector<BoxMeta> meta_;
    bool recovered_ = false;
    bool txLive_ = false;
};

// CRC-32 of the staged chest file written by prepare(); recorded in the journal.
uint32_t stagedChestCrc(const ChestTx& tx);

}  // namespace pkg
