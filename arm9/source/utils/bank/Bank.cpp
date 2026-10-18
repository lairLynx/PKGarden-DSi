/*
 *   Originally part of PKSM (Copyright (C) 2016-2020 Bernardo Giordano, Admiral Fish, piepie62);
 *   rewritten for PKGarden. GPLv3, see LICENSE.
 */

#include "Bank.hpp"

#include <algorithm>

#include "banks.hpp"
#include "config.hpp"
#include "gba/Convert.hpp"
#include "gui.hpp"
#include "i18n_ext.hpp"
#include "pkx/PK5.hpp"

Bank::Bank(std::unique_ptr<pkg::LocalChestStore> store) : store_(std::move(store)) {}

std::unique_ptr<pksm::PKX> Bank::pkm(int box, int slot) const {
	const pkg::Slot &env = store_->slot(box, slot);
	if(!pkg::isEmpty(env)) {
		if(auto pk = pkg::fromEnvelope(env))
			return pk;
	}
	return pksm::PKX::getPKM<pksm::Generation::FIVE>(nullptr);
}

void Bank::pkm(const pksm::PKX &pk, int box, int slot) {
	pkg::Slot env = pkg::emptySlot();
	if(pk.species() != pksm::Species::None) {
		try {
			env = pkg::toEnvelope(pk);
		} catch(const pkg::PKError &) {
			// Gen 6+ records cannot be kept in this chest; leave the slot as it was
			Gui::warn(i18n::localize(Config::getLang("lang"), "BANK_UNSUPPORTED_GEN"));
			return;
		}
	}
	pkg::Slot &cur = store_->editSlots()[static_cast<size_t>(box) * pkg::kSlotsPerBox + static_cast<size_t>(slot)];
	if(cur != env) {
		cur    = env;
		dirty_ = true;
	}
}

std::string Bank::boxName(int box) const {
	const std::string &name = store_->meta(box).name;
	// Boxes that still have their default name follow the UI language
	if(name == "Box " + std::to_string(box + 1))
		return i18n::localize(Config::getLang("lang"), "chest") + " " + std::to_string(box + 1);
	return name;
}

void Bank::boxName(std::string name, int box) {
	if(name.size() > 31)
		name.resize(31);
	pkg::BoxMeta &m = store_->editMeta()[static_cast<size_t>(box)];
	if(m.name != name) {
		m.name = std::move(name);
		dirty_ = true;
	}
}

uint32_t Bank::stage() {
	return store_->stageCurrent();
}

void Bank::publish() {
	store_->publishStaged();
	dirty_ = false;
}

void Bank::discard() {
	store_->discardStagedFile();
	store_->reload();
	dirty_ = false;
}
