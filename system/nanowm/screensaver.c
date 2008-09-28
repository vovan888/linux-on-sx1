/* Screensaver support for
*/

#include <stdio.h>
#include <stdlib.h>
#include <nano-X.h>
#include <common/libini.h>

#define WMDEBUG
#include "nanowm.h"

void do_screensaver (GR_EVENT_SCREENSAVER *event)
{
	int br, type = event->activate;
	Dprintf("do_screensaver: %d\n", type);

	shdata->WM.ScreenSaverRunning = type;

	if (type == 1) {
		br = 0;
		tbus_call_method("T-HAL", "DisplaySetBrightness", "i", &br);
	} else {
		br = 5;
		tbus_call_method("T-HAL", "DisplaySetBrightness", "i", &br);
		return;
	}

	char *filename = cfg_readString("etc/nanowm.cfg", "ScreenSaver", "App");
	char *apparg = cfg_readString("etc/nanowm.cfg", "ScreenSaver", "AppArg");

	fl_spawn(filename, apparg);

	free(filename);
	free(apparg);
}
