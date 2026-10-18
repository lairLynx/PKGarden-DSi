#include "bootMenu.hpp"

#include "colors.hpp"
#include "config.hpp"
#include "graphics.hpp"
#include "gui.hpp"
#include "i18n_ext.hpp"
#include "manager.hpp"
#include "sound.hpp"
#include "splash.h"
#include "version.hpp"

#include <nds.h>

namespace {

struct BootEntry {
	BootChoice choice;
	const char *label; // i18n key
	bool enabled;
	int x, y;
};

// Three buttons in a column, centered on the bottom screen
// clang-format off
std::vector<BootEntry> entries = {
	{BootChoice::Local,   "localGarden",  true,  65, 27},
	{BootChoice::Online,  "onlineGarden", false, 65, 75},
	{BootChoice::Options, "options",      true,  65, 123},
};
// clang-format on

void drawTop(void) {
	drawRectangle(0, 0, 256, 192, DARKERER_GRAY, DARKER_GRAY, true, false);
	drawRectangle(0, 0, 256, 192, CLEAR, true, true);

	const std::string title = "PKGarden";
	printTextTintedScaled(title, 2, 2, TextColor::white, (256 - getTextWidthScaled(title, 2)) / 2, 56, true, true);
	printTextCenteredTinted(i18n::localize(Config::getLang("lang"), "bootTagline"), TextColor::gray, 0, 100, true, true);
	printTextCenteredTinted(i18n::localize(Config::getLang("lang"), "bootHint"), TextColor::gray, 0, 150, true, true);
	printTextScaled(VER_NUMBER, 1, 1, 256 - getTextWidthScaled(VER_NUMBER, 1) - 1, 176, true, true);
}

void drawButtons(int selection) {
	drawImageDMA(0, 0, menuBg, false, false);
	drawRectangle(0, 0, 256, 192, CLEAR, false, true);

	for(int i = 0; i < (int)entries.size(); i++) {
		const BootEntry &e = entries[i];
		drawImage(e.x, e.y, (i == selection && e.enabled) ? menuButtonBlue : menuButton, false, false);
		printTextCenteredTinted(i18n::localize(Config::getLang("lang"), e.label),
								e.enabled ? TextColor::white : TextColor::gray,
								0,
								e.y + 13,
								false,
								true);
	}
}

// Next enabled entry in the given direction (wrapping), so the cursor never rests on a disabled one
int step(int selection, int direction) {
	int n = entries.size();
	for(int i = 0; i < n; i++) {
		selection = (selection + direction + n) % n;
		if(entries[selection].enabled)
			return selection;
	}
	return selection;
}

} // namespace

BootChoice bootMenu(bool firstShow, const std::string &notice) {
	int selection = 0;

	drawTop();
	drawButtons(selection);
	if(firstShow)
		screensFadeIn();

	if(!notice.empty()) {
		Gui::warn(notice);
		drawTop();
		drawButtons(selection);
	}

	touchPosition touch;
	while(1) {
		int pressed;
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
		} while(!pressed);

		if(pressed & KEY_UP) {
			selection = step(selection, -1);
			drawButtons(selection);
		} else if(pressed & KEY_DOWN) {
			selection = step(selection, 1);
			drawButtons(selection);
		} else if(pressed & KEY_A) {
			Sound::play(Sound::click);
			return entries[selection].choice;
		} else if(pressed & KEY_B) {
			Sound::play(Sound::back);
			return BootChoice::Exit;
		} else if(pressed & KEY_TOUCH) {
			touchRead(&touch);
			for(int i = 0; i < (int)entries.size(); i++) {
				const BootEntry &e = entries[i];
				// A disabled entry ignores touches
				if(e.enabled && touch.px >= e.x && touch.px <= e.x + menuButton.width && touch.py >= e.y &&
				   touch.py <= e.y + menuButton.height) {
					Sound::play(Sound::click);
					return e.choice;
				}
			}
		}
	}
}
