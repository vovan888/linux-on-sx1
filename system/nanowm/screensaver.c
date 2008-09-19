/* Screensaver support for
*/

#include <stdio.h>
#include <stdlib.h>
#include "nanowm.h"
#include <nano-X.h>
#include <common/libini.h>

#define WMDEBUG

void do_screensaver (GR_EVENT_SCREENSAVER *event)
{
	int type = event->activate;
	Dprintf("do_screensaver: %d\n", type);

	if (type != 1)
		return;

	char *filename = cfg_readString("etc/nanowm.cfg", "ScreenSaver", "App");
	char *apparg = cfg_readString("etc/nanowm.cfg", "ScreenSaver", "AppArg");

	fl_spawn(filename, apparg);

	free(filename);
	free(apparg);
}
