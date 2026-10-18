#include "banks.hpp"

#include "flashcard.hpp"

std::string Banks::chestPath() {
	return mainDrive() + ":/_nds/pkgarden/chest.pgc";
}

pkg::Journal Banks::journal() {
	return pkg::Journal(mainDrive() + ":/_nds/pkgarden/journal.txt");
}

void Banks::init() {
	notice.clear();
	auto store = pkg::LocalChestStore::open(chestPath());
	if(store->recoveredFromBackup())
		notice = "The chest file was damaged. The previous version was restored.";

	// A crash between writing the save and publishing the chest is finished (or undone) here.
	auto result = pkg::recover(journal(), *store);
	if(result.action != pkg::Recovery::Nothing)
		notice = result.message;

	bank = std::make_shared<Bank>(std::move(store));
}
