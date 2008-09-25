/*
 * Keyboard functions for nanowm
 *
 * Copyright (C) 2008 Vladimir Ananiev (vovan888 at gmail com)
 *
 * Licensed under GPLv2, see LICENSE
*/

#include <stdio.h>
#include <nano-X.h>

#include "nanowm.h"

static struct {
	char *keyname;
	GR_KEY  ch;
} keybindings = {
  {"Green", Key_Green},
  {"Red", Key_Red},
  {"Menu", Key_Menu},
  {"LeftSoft", Key_LeftSoft},
  {"RightSoft", Key_RightSoft},
}

int handle_key_up(GR_EVENT_KEYSTROKE *event)
{
	if (event->wid != GR_ROOT_WINDOW_ID)
		return 0;

	
}