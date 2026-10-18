#ifndef BOOT_MENU_HPP
#define BOOT_MENU_HPP

#include <string>

enum class BootChoice {
	Local,   // Local Garden: work with save files on this console
	Online,  // Online Garden: not available yet (shown but cannot be selected)
	Options, // Options menu
	Exit,    // Leave the app
};

/*
 * The first menu after the startup splash.
 * bool firstShow is whether the screens are still black from the splash: they are faded in once the menu is drawn
 * const std::string &notice is a message to show on top of the menu once it is visible (may be empty)
 * Returns what the user picked
 */
BootChoice bootMenu(bool firstShow, const std::string &notice);

#endif
