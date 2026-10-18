#include "gba/Convert.hpp"

#include <string>
#include <vector>

#include "PK3.hpp"
#include "PK4.hpp"
#include "PK5.hpp"
#include "PKX.hpp"
#include "sav/Sav.hpp"
#include "util/Files.hpp"

namespace pkg {

Slot toEnvelope(const pksm::PKX& pk)
{
    // Work on a copy in box form so party-only bytes are never stored.
    std::unique_ptr<pksm::PKX> c = pk.clone();
    Format f;
    size_t len;
    switch (pk.generation()) {
        case pksm::Generation::THREE: f = Format::PK3; len = pksm::PK3::BOX_LENGTH; break;
        case pksm::Generation::FOUR: f = Format::PK4; len = pksm::PK4::BOX_LENGTH; break;
        case pksm::Generation::FIVE: f = Format::PK5; len = pksm::PK5::BOX_LENGTH; break;
        default: throw PKError("This Pokemon's generation cannot be stored in the chest");
    }
    // PKSM-Core keeps records decrypted in memory; the envelope stores that canonical form. The
    // checksum must be valid: the core uses it to tell decrypted from encrypted records.
    c->refreshChecksum();
    return makeSlot(f, c->rawData(), len);
}

std::unique_ptr<pksm::PKX> fromEnvelope(const Slot& env, std::string* why)
{
    auto fail = [&](const char* msg) -> std::unique_ptr<pksm::PKX> {
        if (why) *why = msg;
        return nullptr;
    };
    if (isEmpty(env)) return fail("empty");
    if (validateSlot(env.data(), env.size())) return fail("damaged entry");
    // The core may decrypt in place, so hand it a private copy.
    std::vector<uint8_t> buf(payload(env), payload(env) + payloadLen(env));
    switch (format(env)) {
        case Format::PK3: return pksm::PKX::getPKM<pksm::Generation::THREE>(buf.data(), false);
        case Format::PK4: return pksm::PKX::getPKM<pksm::Generation::FOUR>(buf.data(), false);
        case Format::PK5: return pksm::PKX::getPKM<pksm::Generation::FIVE>(buf.data(), false);
        case Format::PK1:
        case Format::PK2: return fail("Gen 1/2 Pokemon are not supported yet");
        default: return fail("unknown format");
    }
}

static const char* reasonText(pksm::Sav::BadTransferReason r)
{
    switch (r) {
        case pksm::Sav::BadTransferReason::MOVE: return "knows a move this game does not have";
        case pksm::Sav::BadTransferReason::SPECIES: return "this species does not exist in this game";
        case pksm::Sav::BadTransferReason::FORM: return "this form does not exist in this game";
        case pksm::Sav::BadTransferReason::ABILITY: return "has an ability this game does not have";
        case pksm::Sav::BadTransferReason::ITEM: return "holds an item this game does not have";
        case pksm::Sav::BadTransferReason::BALL: return "its Poke Ball does not exist in this game";
        default: return "";
    }
}

WithdrawPlan planWithdraw(const Slot& env, pksm::Sav& target)
{
    WithdrawPlan plan;
    std::string why;
    auto pk = fromEnvelope(env, &why);
    if (!pk) {
        plan.error = why == "empty" ? "Nothing to withdraw" : why;
        return plan;
    }
    auto reason = target.invalidTransferReason(*pk);
    if (reason != pksm::Sav::BadTransferReason::OKAY) {
        plan.error = std::string("Cannot move to this game: ") + reasonText(reason);
        return plan;
    }
    const bool lossy = pk->generation() > target.generation();
    auto converted = target.transfer(*pk);
    if (!converted) {
        plan.error = "This Pokemon cannot be converted for this game";
        return plan;
    }
    plan.pkm = std::move(converted);
    plan.lossy = lossy;
    return plan;
}

}  // namespace pkg
