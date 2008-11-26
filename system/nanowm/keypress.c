/*
 * Keyboard functions for nanowm
 *
 * Copyright (C) 2008 Vladimir Ananiev (vovan888 at gmail com)
 *
 * Licensed under GPLv2, see LICENSE
*/

#include <stdio.h>
#include <stdlib.h>
#include <nano-X.h>

#include "nanowm.h"

static struct {
	char *keyname;
	GR_KEY  ch;
} keybindings[] = {
  {"Green",	Key_Green},
  {"Red",	Key_Red},
  {"Menu",	Key_Menu},
  {"LeftSoft",	Key_LeftSoft},
  {"RightSoft",	Key_RightSoft},
  {"JoyDown",	Key_JoyDown},
  {NULL, 0}
};

static int start_app_for_key(GR_KEY ch)
{
	int i = 0, found = 0;

	while (keybindings[i].ch > 0) {
		if(keybindings[i].ch == ch) {
			found = 1;
			break;
		}
		i++;
	}

	if(!found)
		return 0;

	char *AppPath = cfg_readString ("etc/nanowm.cfg", "HotKeys", keybindings[i].keyname);

	if(AppPath != NULL) {
		Dprintf("Spawn application : %s\n", AppPath);
		if (strlen(AppPath) > 0)
			app_run(AppPath, "");
		//fl_spawn(AppPath, "");

		free(AppPath);
	}

	return 0;
}

/** Alt-Tab switcher
*/
static void handle_menu_key(void)
{
	Dprintf("handle_menu_key\n");
/*	if (shdata->WM.top_active_window == GR_ROOT_WINDOW_ID) {
		start_app_for_key(Key_Menu);
	} else */
		show_taskmanager();

}

void handle_key_up(GR_EVENT_KEYSTROKE *event)
{
	Dprintf("handle_key_up\n");
	/* handle special cases */
	/* case 1 - Menu key pressed - global Hotkeys */
	if (event->ch == Key_Menu) {
		handle_menu_key();
		return;
	}

	if (event->wid != GR_ROOT_WINDOW_ID)
		return;

	start_app_for_key(event->ch);
}
