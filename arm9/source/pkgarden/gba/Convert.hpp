#pragma once
// Pokemon <-> chest envelope, and conversion between generations (PLAN.md conversion matrix).
#include <memory>
#include <string>

#include "chest/Envelope.hpp"

namespace pksm {
class PKX;
class Sav;
}  // namespace pksm

namespace pkg {

// Packs a Pokemon (box form, decrypted) into an envelope. Throws PKError for generations the
// chest does not store.
Slot toEnvelope(const pksm::PKX& pk);

// Reads an envelope. Returns null for empty slots and for formats this build cannot interpret
// (Gen 1/2 are reserved for later); `why` then says which.
std::unique_ptr<pksm::PKX> fromEnvelope(const Slot& env, std::string* why = nullptr);

struct WithdrawPlan {
    std::unique_ptr<pksm::PKX> pkm;  // ready to write to the target save; null if !ok()
    bool lossy = false;              // converted to an older generation: some data is dropped
    std::string error;               // non-empty when the Pokemon cannot go into that save
    bool ok() const { return pkm != nullptr; }
};

// Converts an envelope for the given save, checking that the target game can hold it.
WithdrawPlan planWithdraw(const Slot& env, pksm::Sav& target);

}  // namespace pkg
