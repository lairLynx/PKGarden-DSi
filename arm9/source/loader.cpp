#include "loader.hpp"

#include "banks.hpp"
#include "config.hpp"
#include "fileBrowse.hpp"
#include "flashcard.hpp"
#include "gui.hpp"
#include "i18n_ext.hpp"
#include "util/Crc32.hpp"

#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

static std::string saveFileName;

std::shared_ptr<pksm::Sav> save;

bool loadSave(std::string savePath) {
	save         = nullptr;
	saveFileName = savePath;
	FILE *in     = fopen(savePath.c_str(), "rb");
	if(in) {
		fseek(in, 0, SEEK_END);
		u32 size = ftell(in);
		fseek(in, 0, SEEK_SET);
		std::shared_ptr<u8[]> saveData = std::shared_ptr<u8[]>(new u8[size]);
		fread(saveData.get(), 1, size, in);
		fclose(in);
		save = pksm::Sav::getSave(saveData, size);
	} else {
		saveFileName = "";
		return false;
	}
	if(!save) {
		saveFileName = "";
		return false;
	}
	return true;
}

bool saveChanges(std::string savePath) {
	// Make backup
	std::string saveFile = savePath.substr(savePath.find_last_of("/") + 1);
	char backupDir[PATH_MAX];
	snprintf(backupDir,
			 sizeof(backupDir),
			 "%s:/_nds/pkgarden/backups/%s",
			 mainDrive().c_str(),
			 saveFile.substr(0, saveFile.find_last_of(".")).c_str());
	mkdir(backupDir, 0777);

	char backupPath[PATH_MAX];
	DateTime now = DateTime::now();
	snprintf(backupPath,
			 sizeof(backupPath),
			 "%s:/_nds/pkgarden/backups/%s/%.4ld%.2d%.2d-%.2d%.2d%.2d.sav.bak",
			 mainDrive().c_str(),
			 saveFile.substr(0, saveFile.find_last_of(".")).c_str(),
			 now.year(),
			 now.month(),
			 now.day(),
			 now.hour(),
			 now.minute(),
			 now.second());

	fcopy(savePath.c_str(), backupPath);

	if(Config::getInt("backupAmount") > 0) { // 0 = unlimited
		char savDir[PATH_MAX];
		getcwd(savDir, PATH_MAX);
		chdir(backupDir);
		std::vector<DirEntry> dirContents;
		getDirectoryContents(dirContents, {"bak"});

		if((int)dirContents.size() > Config::getInt("backupAmount"))
			remove(dirContents[1].name.c_str()); // index 0 is '..'

		chdir(savDir);
	}

	save->finishEditing();

	bool ok   = false;
	FILE *out = fopen(savePath.c_str(), "rb+");
	if(out) {
		ok = fwrite(save->rawData().get(), 1, save->getLength(), out) == save->getLength();
		ok = (fclose(out) == 0) && ok;
	}

	save->beginEditing();
	return ok;
}

bool commitAll(const std::string &savePath) {
	Bank &bank            = *Banks::bank;
	const bool chestDirty = bank.hasChanged();
	pkg::Journal journal  = Banks::journal();

	if(chestDirty) {
		// Record what the save file will look like afterwards, then stage the new chest. If the power
		// dies after this point, the next start compares the save with this record and finishes or
		// undoes the chest update.
		save->finishEditing();
		pkg::JournalRecord rec;
		rec.savePath  = savePath;
		rec.afterSize = save->getLength();
		rec.afterCrc  = pkg::crc32(save->rawData().get(), save->getLength());
		try {
			pkg::Bytes before = pkg::readFile(savePath);
			rec.beforeCrc     = pkg::crc32(before.data(), before.size());
			rec.chestCrc      = bank.stage();
			journal.write(rec);
		} catch(const std::exception &) {
			bank.store().discardStagedFile();
			journal.clear();
			Gui::warn(i18n::localize(Config::getLang("lang"), "saveFailedChest"));
			return false;
		}
	}

	if(!saveChanges(savePath)) {
		if(chestDirty) {
			bank.store().discardStagedFile();
			journal.clear();
		}
		Gui::warn(i18n::localize(Config::getLang("lang"), "saveFailedSave"));
		return false;
	}

	if(chestDirty) {
		try {
			bank.publish();
		} catch(const std::exception &) {
			// The save is already written; the journal is kept so the next start completes the chest update
			Gui::warn(i18n::localize(Config::getLang("lang"), "saveFailedChest"));
			return true;
		}
		journal.clear();
	}
	return true;
}
