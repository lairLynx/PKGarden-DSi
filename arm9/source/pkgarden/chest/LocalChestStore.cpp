#include "chest/LocalChestStore.hpp"

#include <algorithm>
#include <cstring>
#include <string>

#include "util/Crc32.hpp"

namespace pkg {

namespace {
constexpr size_t kFileHeader = 16;
constexpr size_t kMetaSize = 36;
constexpr size_t kMetaTable = kMetaSize * kBoxCount;
constexpr size_t kFileSize = kFileHeader + kMetaTable + kBoxBlobSize * kBoxCount;  // 1,003,084

void wr16(uint8_t* p, uint16_t v)
{
    p[0] = v & 0xFF;
    p[1] = static_cast<uint8_t>(v >> 8);
}
uint16_t rd16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
void wr32(uint8_t* p, uint32_t v)
{
    for (int i = 0; i < 4; i++) p[i] = (v >> (8 * i)) & 0xFF;
}
uint32_t rd32(const uint8_t* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void checkBox(int box, int slot)
{
    if (box < 0 || box >= kBoxCount || slot < 0 || slot >= static_cast<int>(kSlotsPerBox))
        throw PKError("Box or slot out of range");
}
}  // namespace

size_t LocalChestStore::index(int box, int slot)
{
    checkBox(box, slot);
    return static_cast<size_t>(box) * kSlotsPerBox + static_cast<size_t>(slot);
}

Bytes LocalChestStore::serialize(const std::vector<Slot>& slots, const std::vector<BoxMeta>& meta)
{
    Bytes out(kFileSize, 0);
    uint8_t* metaTable = out.data() + kFileHeader;
    for (int b = 0; b < kBoxCount; b++) {
        uint8_t* m = metaTable + b * kMetaSize;
        const std::string& name = meta[b].name;
        std::memcpy(m, name.data(), std::min<size_t>(name.size(), 31));
        m[32] = meta[b].wallpaper;
    }
    std::memcpy(out.data(), "PGCF", 4);
    wr16(&out[4], 1);
    wr16(&out[6], kBoxCount);
    wr32(&out[8], crc32(metaTable, kMetaTable));
    uint8_t* boxes = metaTable + kMetaTable;
    for (int b = 0; b < kBoxCount; b++) writeBoxBlob(&slots[static_cast<size_t>(b) * kSlotsPerBox], boxes + b * kBoxBlobSize);
    return out;
}

bool LocalChestStore::parse(const Bytes& d, std::vector<Slot>& slots, std::vector<BoxMeta>& meta, std::string& error)
{
    if (d.size() != kFileSize) { error = "chest file has wrong size"; return false; }
    if (std::memcmp(d.data(), "PGCF", 4) != 0) { error = "not a PKGarden chest"; return false; }
    if (rd16(&d[4]) != 1) { error = "unsupported chest version"; return false; }
    if (rd16(&d[6]) != kBoxCount) { error = "unexpected box count"; return false; }
    const uint8_t* metaTable = d.data() + kFileHeader;
    if (rd32(&d[8]) != crc32(metaTable, kMetaTable)) { error = "chest header crc mismatch"; return false; }

    std::vector<BoxMeta> m(kBoxCount);
    for (int b = 0; b < kBoxCount; b++) {
        const uint8_t* e = metaTable + b * kMetaSize;
        m[b].name.assign(reinterpret_cast<const char*>(e), strnlen(reinterpret_cast<const char*>(e), 32));
        m[b].wallpaper = e[32];
    }
    std::vector<Slot> s(static_cast<size_t>(kBoxCount) * kSlotsPerBox);
    const uint8_t* boxes = metaTable + kMetaTable;
    for (int b = 0; b < kBoxCount; b++) {
        const uint8_t* blob = boxes + b * kBoxBlobSize;
        if (auto err = validateBoxBlob(blob, kBoxBlobSize)) {
            error = "box " + std::to_string(b + 1) + ": " + *err;
            return false;
        }
        for (size_t i = 0; i < kSlotsPerBox; i++)
            std::memcpy(s[static_cast<size_t>(b) * kSlotsPerBox + i].data(), blob + kBoxHeader + i * kSlotSize, kSlotSize);
    }
    slots = std::move(s);
    meta = std::move(m);
    return true;
}

std::unique_ptr<LocalChestStore> LocalChestStore::open(const fs::path& path)
{
    std::unique_ptr<LocalChestStore> st(new LocalChestStore(path));
    if (!path.parent_path().empty()) fs::create_directories(path.parent_path());

    if (!fs::exists(path) && fs::exists(st->backupPath())) {
        // The main file is missing but a backup exists: a crash between "remove old" and "rename new"
        // on a filesystem that cannot replace files atomically (FAT). Never start an empty chest here.
        std::string err;
        if (parse(readFile(st->backupPath()), st->slots_, st->meta_, err)) {
            st->recovered_ = true;
            return st;
        }
        throw PKError("Chest file is missing and its backup is damaged: " + err);
    }
    if (!fs::exists(path)) {
        st->slots_.assign(static_cast<size_t>(kBoxCount) * kSlotsPerBox, emptySlot());
        st->meta_.resize(kBoxCount);
        for (int b = 0; b < kBoxCount; b++) st->meta_[b].name = "Box " + std::to_string(b + 1);
        writeFileAtomic(path, serialize(st->slots_, st->meta_));
        return st;
    }
    std::string err;
    if (parse(readFile(path), st->slots_, st->meta_, err)) return st;

    if (fs::exists(st->backupPath()) && parse(readFile(st->backupPath()), st->slots_, st->meta_, err)) {
        st->recovered_ = true;
        return st;
    }
    throw PKError("Chest file is damaged: " + err);
}

const BoxMeta& LocalChestStore::meta(int box) const
{
    checkBox(box, 0);
    return meta_[static_cast<size_t>(box)];
}

const Slot& LocalChestStore::slot(int box, int s) const { return slots_[index(box, s)]; }

void LocalChestStore::setMeta(int box, const BoxMeta& m)
{
    checkBox(box, 0);
    if (txLive_) throw PKError("Finish the current transaction first");
    BoxMeta fixed = m;
    if (fixed.name.size() > 31) fixed.name.resize(31);
    auto next = meta_;
    next[static_cast<size_t>(box)] = fixed;
    Bytes data = serialize(slots_, next);
    // keep the previous file as .bak, then replace atomically
    if (fs::exists(path_)) copyFileSynced(path_, backupPath());
    writeFileAtomic(path_, data);
    meta_ = std::move(next);
}

uint32_t LocalChestStore::stageCurrent()
{
    Bytes data = serialize(slots_, meta_);
    writeFileSynced(stagedPath(), data.data(), data.size());
    return crc32(data.data(), data.size());
}

void LocalChestStore::reload()
{
    std::vector<Slot> s;
    std::vector<BoxMeta> m;
    std::string err;
    if (!parse(readFile(path_), s, m, err)) throw PKError("Chest file is damaged: " + err);
    slots_ = std::move(s);
    meta_ = std::move(m);
}

void LocalChestStore::publishStaged()
{
    if (fs::exists(path_)) copyFileSynced(path_, backupPath());
    std::error_code ec;
    fs::rename(stagedPath(), path_, ec);  // atomic replace
    if (ec) throw PKError("Cannot publish the new chest file");
}

bool LocalChestStore::commitStagedFile(uint32_t expectedCrc)
{
    if (!fs::exists(stagedPath())) return false;
    Bytes b = readFile(stagedPath());
    std::vector<Slot> s;
    std::vector<BoxMeta> m;
    std::string err;
    if (crc32(b.data(), b.size()) != expectedCrc || !parse(b, s, m, err)) return false;
    publishStaged();
    slots_ = std::move(s);
    meta_ = std::move(m);
    return true;
}

void LocalChestStore::discardStagedFile() { removeIfExists(stagedPath()); }

// ---------------------------------------------------------------------------------------------

class LocalTx final : public ChestTx {
public:
    explicit LocalTx(LocalChestStore& s) : s_(s) {}
    // Dropping an unresolved transaction touches no files: whatever was staged is handled by
    // journal recovery at the next start (exactly as after a crash).
    ~LocalTx() override
    {
        if (!done_) s_.txLive_ = false;
    }

    const Slot& slot(int box, int slot) const override
    {
        size_t i = LocalChestStore::index(box, slot);
        auto it = overlay_.find(i);
        return it != overlay_.end() ? it->second : s_.slots_[i];
    }

    void withdraw(int box, int slot) override
    {
        guard();
        size_t i = LocalChestStore::index(box, slot);
        if (isEmpty(this->slot(box, slot))) throw PKError("That slot is empty");
        overlay_[i] = emptySlot();
    }

    void deposit(int box, int slot, const Slot& env) override
    {
        guard();
        size_t i = LocalChestStore::index(box, slot);
        if (auto err = validateSlot(env.data(), env.size())) throw PKError("Invalid Pokemon data: " + *err);
        if (isEmpty(env)) throw PKError("Nothing to deposit");
        if (!isEmpty(this->slot(box, slot))) throw PKError("That slot is not empty");
        overlay_[i] = env;
    }

    bool dirty() const override
    {
        for (auto& [i, v] : overlay_)
            if (v != s_.slots_[i]) return true;
        return false;
    }

    void prepare() override
    {
        guard();
        std::vector<Slot> next = s_.slots_;
        for (auto& [i, v] : overlay_) next[i] = v;
        Bytes data = LocalChestStore::serialize(next, s_.meta_);
        writeFileSynced(s_.stagedPath(), data.data(), data.size());
        stagedCrc_ = crc32(data.data(), data.size());
        prepared_ = true;
    }

    uint32_t stagedCrc() const override { return stagedCrc_; }

    void commit() override
    {
        guard();
        if (!prepared_) throw PKError("commit() before prepare()");
        s_.publishStaged();
        for (auto& [i, v] : overlay_) s_.slots_[i] = v;
        finish();
    }

    void cancel() override
    {
        if (done_) return;
        s_.discardStagedFile();
        finish();
    }

private:
    void guard() const
    {
        if (done_) throw PKError("Transaction already finished");
    }
    void finish()
    {
        done_ = true;
        s_.txLive_ = false;
    }

    LocalChestStore& s_;
    std::map<size_t, Slot> overlay_;
    bool prepared_ = false;
    bool done_ = false;
    uint32_t stagedCrc_ = 0;
};

std::unique_ptr<ChestTx> LocalChestStore::begin()
{
    if (txLive_) throw PKError("A transaction is already open");
    txLive_ = true;
    return std::make_unique<LocalTx>(*this);
}

uint32_t stagedChestCrc(const ChestTx& tx) { return tx.stagedCrc(); }

}  // namespace pkg
