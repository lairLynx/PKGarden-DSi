#pragma once
// Storage interface used by the UI. LocalChestStore implements it today; a server-backed
// RemoteChestStore will implement the same semantics later (see PKGarden-Spec/PLAN.md).
#include <memory>
#include <string>

#include "chest/Envelope.hpp"

namespace pkg {

struct BoxMeta {
    std::string name;  // at most 31 bytes UTF-8
    uint8_t wallpaper = 0;
};

// A set of staged changes. Nothing is visible in the store until commit().
// Protocol: withdraw()/deposit() ... prepare() -> (caller writes its save) -> commit(), or cancel().
class ChestTx {
public:
    virtual ~ChestTx() = default;
    // Slot as it will look after commit (withdrawn slots read as empty).
    virtual const Slot& slot(int box, int slot) const = 0;
    virtual void withdraw(int box, int slot) = 0;                    // throws PKError if empty
    virtual void deposit(int box, int slot, const Slot& env) = 0;    // throws PKError if not empty
    virtual bool dirty() const = 0;
    // Durably stages the result. The caller may now write its own file.
    virtual void prepare() = 0;
    // CRC-32 of what prepare() staged (recorded in the journal; 0 for stores without a staged file).
    virtual uint32_t stagedCrc() const = 0;
    virtual void commit() = 0;  // publishes (requires prepare())
    virtual void cancel() = 0;  // drops staged state; safe to call at any time
};

class ChestStore {
public:
    virtual ~ChestStore() = default;
    virtual int boxCount() const = 0;
    virtual const BoxMeta& meta(int box) const = 0;
    virtual void setMeta(int box, const BoxMeta& m) = 0;  // persisted immediately
    virtual const Slot& slot(int box, int slot) const = 0;
    virtual std::unique_ptr<ChestTx> begin() = 0;         // at most one live transaction
};

}  // namespace pkg
