#include "banks.hpp"
#include "bootMenu.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "configMenu.hpp"
#include "fileBrowse.hpp"
#include "flashcard.hpp"
#include "graphics.hpp"
#include "gui.hpp"
#include "i18n.hpp"
#include "i18n_ext.hpp"
#include "loader.hpp"
#include "manager.hpp"
#include "miscUtils.hpp"
#include "myDSiMode.h"
#include "sound.hpp"
#include "splash.h"

#include <fat.h>
#include <filesystem.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

bool useTwlCfg     = false;
u32 *twlCfgPointer = (u32 *)0x02FFFDFC;
u8 *twlCfgAddr     = (u8 *)0;

extern std::vector<std::string> songs;

void init(int argc, char **argv) {
	if(dsiFeatures()) {
		if(*twlCfgPointer < 0x02000000 || *twlCfgPointer >= 0x03000000) {
			*twlCfgPointer = 0x02000400;
		}
		twlCfgAddr = (u8 *)*twlCfgPointer;
		useTwlCfg  = ((twlCfgAddr[0] != 0) && (twlCfgAddr[1] == 0) && (twlCfgAddr[2] == 0) && (twlCfgAddr[4] == 0));
	}
	// The splash covers everything up to the point where the real graphics are set up
	splashBegin();
	keysSetRepeat(25, 5);
	sysSetCardOwner(BUS_OWNER_ARM9); // Set ARM9 as Slot-1 owner (for dumping/injecting DS saves)
	sysSetCartOwner(BUS_OWNER_ARM9); // Set ARM9 as Slot-2 owner (for dumping/injecting GBA saves)
	defaultExceptionHandler();
	scanKeys(); // So it doesn't open the SD if A is held
	srand(time(NULL));

	// Init filesystem
	if(!fatInitDefault()) {
		// Prints error if fatInitDefault() fails
		consoleDemoInit();
		printf("fatInitDefault() failed...");
		while(1)
			swiWaitForVBlank();
	}

	// Make directories
	mkdir((mainDrive() + ":/_nds").c_str(), 0777);
	mkdir((mainDrive() + ":/_nds/pkgarden").c_str(), 0777);
	mkdir((mainDrive() + ":/_nds/pkgarden/in").c_str(), 0777);
	mkdir((mainDrive() + ":/_nds/pkgarden/out").c_str(), 0777);
	mkdir((mainDrive() + ":/_nds/pkgarden/banks").c_str(), 0777);
	mkdir((mainDrive() + ":/_nds/pkgarden/themes").c_str(), 0777);
	mkdir((mainDrive() + ":/_nds/pkgarden/backups").c_str(), 0777);

	// Try to init NitroFS from argv provided to the app when it was launched
	if(!nitroFSInit(nullptr)) {
		// If that fails, try to init NitroFS on 'pkgarden.nds'
		if(!nitroFSInit("pkgarden.nds") || !nitroFSGood()) {
			// If that fails, try '/_nds/pkgarden/pkgarden.nds'
			if(!nitroFSInit("/_nds/pkgarden/pkgarden.nds")) {
				// Print error that nitroFSInit() failed
				consoleDemoInit();
				printf("nitroFSInit() failed...\n\n");
				printf("Please copy pkgarden.nds to:\n\n");
				printf("%s:/_nds/pkgarden/\n", mainDrive().c_str());
				printf("                  pkgarden.nds\n\n");
				printf("or launch pkgarden using\n\n");
				printf("      TWiLight Menu++ or HBMenu\n\n\n\n\n");
				while(1)
					swiWaitForVBlank();
			} else if(!nitroFSGood()) {
				// Print error that the NitroFS was out of date
				consoleDemoInit();
				printf("NitroFS is out of date!\n\n");
				printf("Please update pkgarden.nds at:\n\n");
				printf("%s:/_nds/pkgarden/\n", mainDrive().c_str());
				while(1)
					swiWaitForVBlank();
			}
		}
	}

	wideScreen = (argc > 1 && strcmp(argv[1], "wide") == 0) || keysDown() & KEY_Y;

	Config::load();
	Colors::load();
	loadFont();

	i18n::addCallbacks(i18n::initGui, i18n::exitGui);
	i18n::removeCallbacks(i18n::initForm, i18n::exitForm);
	i18n::removeCallbacks(i18n::initType, i18n::exitType);
	i18n::removeCallbacks(i18n::initGeo, i18n::exitGeo);
	i18n::init(Config::getLang("lang"));

	if(Config::getString("music") == "theme") {
		Sound::load((Config::getString("themeDir") + "/sound.msl").c_str());
	} else {
		Sound::load(("nitro:/sound/" + Config::getString("music") + ".msl").c_str());
	}

	// A damaged chest is reported once the graphics are up (below) and ends the program
	std::string chestError;
	try {
		Banks::init();
	} catch(const std::exception &e) {
		chestError = e.what();
	}

	// End of the splash; the screens stay black while the real graphics are set up
	splashEnd();

	initGraphics();
	drawRectangle(0, 0, 256, 192, DARKERER_GRAY, DARKER_GRAY, true, false);
	drawRectangle(0, 0, 256, 192, DARKERER_GRAY, DARKER_GRAY, false, false);
	initSprites();
	loadGraphics();
	loadTypes(Config::getLang("lang"));

	if(!chestError.empty()) {
		screensFadeIn();
		throw std::runtime_error(chestError);
	}
}

// Local Garden: pick a save file and move Pokémon between it and the chest.
// Returns when the user goes back to the main menu.
void localGarden() {
	showTopMenuOnExit = true;
	savesHomeDir.clear();

	// Start in the default saves folder if there is one
	std::string dir = Config::getString("savesDir");
	if(!dir.empty()) {
		if(chdir(dir.c_str()) == 0) {
			savesHomeDir      = dir;
			showTopMenuOnExit = false;
		} else {
			Gui::warn(i18n::localize(Config::getLang("lang"), "savesFolderMissing"));
		}
	}

	while(1) {
		savePath = browseForSave();
		if(savePath == "%EXIT%")
			break;

		if(!loadSave(savePath)) {
			Gui::warn(i18n::localize(Config::getLang("lang"), "invalidSave"));
			continue;
		}
		currentSaveBox = save->currentBox();
		currentBankBox = 0;
		// Decrypt the box data
		save->cryptBoxData(true);

		manageBoxes();
	}
}

// The main menu: Local Garden, Online Garden (not available yet), Options
void bootLoop() {
	bool first         = true;
	std::string notice = Banks::notice; // e.g. "Finished an interrupted save."
	while(1) {
		switch(bootMenu(first, notice)) {
			case BootChoice::Local:
				localGarden();
				break;
			case BootChoice::Options:
				configMenu();
				break;
			case BootChoice::Online: // can't be selected yet
				break;
			case BootChoice::Exit:
				return;
		}
		first = false;
		notice.clear();
	}
}

int main(int argc, char **argv) {
	try {
		init(argc, argv);

		// A save passed on the command line skips the menu
		bool foundInArgv = false;
		for(int i = 1; i < argc; i++) {
			if(!loadSave(argv[i])) {
				continue;
			}
			if(!foundInArgv) {
				screensFadeIn();
				if(!Banks::notice.empty())
					Gui::warn(Banks::notice);
			}
			foundInArgv    = true;
			currentSaveBox = save->currentBox();
			currentBankBox = 0;
			// Decrypt the box data
			save->cryptBoxData(true);

			manageBoxes();
		}

		if(!foundInArgv)
			bootLoop();
	} catch(std::exception &e) {
		Gui::warn(e.what());
	}

	return 0;
}
