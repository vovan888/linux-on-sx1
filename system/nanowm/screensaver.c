/* Screensaver support for
*/

#include <stdio.h>
#include <stdlib.h>
#include <nano-X.h>
#include <common/libini.h>

#include "nanowm.h"

void do_screensaver (GR_EVENT_SCREENSAVER *event)
{
	int br, type = event->activate;
	Dprintf("do_screensaver: %d\n", type);

	shdata->WM.ScreenSaverRunning = type;

	if (type == 1) {
		br = cfg_readInt("etc/nanowm.cfg", "Brightness", "Dim");
		tbus_call_method("T-HAL", "DisplaySetBrightness", "i", &br);
	} else {
		br = cfg_readInt("etc/nanowm.cfg", "Brightness", "Full");
		tbus_call_method("T-HAL", "DisplaySetBrightness", "i", &br);
		return;
	}

	char *filename = cfg_readString("etc/nanowm.cfg", "ScreenSaver", "App");
	char *apparg = cfg_readString("etc/nanowm.cfg", "ScreenSaver", "AppArg");

	fl_spawn(filename, apparg);

	free(filename);
	free(apparg);
}
