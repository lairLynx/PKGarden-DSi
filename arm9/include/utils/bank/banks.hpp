#ifndef BANKS_HPP
#define BANKS_HPP

#include <memory>
#include <string>

#include "Bank.hpp"
#include "chest/Journal.hpp"

// PKGarden has a single chest (chest.pgc, 99 boxes) on the SD card.
namespace Banks {
	inline std::shared_ptr<Bank> bank = nullptr;
	// Text to show the user after start-up (e.g. "Finished an interrupted save."), if any.
	inline std::string notice;

	// Opens the chest and finishes or rolls back an interrupted save. Throws on a damaged chest.
	void init();
	pkg::Journal journal();
	std::string chestPath();
}

#endif
