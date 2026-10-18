#ifndef LOADER_HPP
#define LOADER_HPP

#include <memory>

#include "Sav.hpp"

extern std::shared_ptr<pksm::Sav> save;

bool loadSave(std::string savePath);

// Backs up the save file and overwrites it with the current state. Returns false if writing failed.
bool saveChanges(std::string savePath);

// Writes the save file and the chest together so that a Pokemon is never lost or duplicated, even if
// the power dies half-way (see chest/Journal.hpp). Tells the user and returns false if it failed
// without changing anything. The box data must be re-encrypted (cryptBoxData(false)) beforehand.
bool commitAll(const std::string &savePath);

#endif
