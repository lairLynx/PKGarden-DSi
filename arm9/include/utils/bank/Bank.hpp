/*
 *   This file was part of PKSM (Copyright (C) 2016-2020 Bernardo Giordano, Admiral Fish, piepie62)
 *   and was rewritten for PKGarden: the chest is now a single PKGarden chest file (chest.pgc,
 *   see PKGarden-Spec/SPEC.md) instead of a pkmn-chest .bnk bank.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#ifndef BANK_HPP
#define BANK_HPP

#include <memory>
#include <string>

#include "chest/LocalChestStore.hpp"
#include "pkx/PKX.hpp"

// The chest as the UI sees it: 99 boxes of 30 Pokemon (Gen 3, 4 and 5 records), edited freely in
// memory. Nothing reaches the SD card until the save flow in loader.cpp (commitAll) runs, which
// writes the chest and the .sav together so that a Pokemon is never lost or duplicated.
class Bank
{
public:
    explicit Bank(std::unique_ptr<pkg::LocalChestStore> store);

    // Returns a blank (species None) Pokemon for empty slots.
    std::unique_ptr<pksm::PKX> pkm(int box, int slot) const;
    // Storing a Pokemon with species None empties the slot. Only Gen 3/4/5 records can be stored.
    void pkm(const pksm::PKX& pkm, int box, int slot);

    std::string boxName(int box) const;
    void boxName(std::string name, int box);
    int boxes() const { return pkg::kBoxCount; }

    bool hasChanged() const { return dirty_; }

    // Two-phase save, driven by commitAll():
    uint32_t stage();   // writes chest.pgc.new, returns its CRC-32
    void publish();     // chest.pgc.new becomes chest.pgc; the previous chest is kept as .bak
    void discard();     // forget all unsaved changes (reloads from disk)

    pkg::LocalChestStore& store() { return *store_; }

private:
    std::unique_ptr<pkg::LocalChestStore> store_;
    bool dirty_ = false;
};

#endif
