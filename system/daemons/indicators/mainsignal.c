/* mainsignal.c
*
*  Display signal indicator on MainScreen
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#include <flphone_config.h>

#include <stdio.h>
#include <stdarg.h>

#include <theme.h>

#include <nxcolors.h>
#include "indicators.h"

static GR_IMAGE_ID signal_image;
static int signal_frame_width;
static int signal_frame_height;
static int signal_current = 0;
/*FIXME*/ static int xcoord, ycoord;	/* XY coordinates of indicator */
#define MAINSIGNAL_NUMFRAMES	6

static int mainsignal_show(int frame);

static void mainsignal_changed_callback(int new_value)
{
	DBGMSG("%d\n", shdata->network.bars);

	if (signal_current != shdata->network.bars) {
		signal_current = shdata->network.bars;
		mainsignal_show(signal_current);
	}
}

static void mainsignal_event_callback(GR_WINDOW_ID window, GR_EVENT * event)
{
	DBGMSG("\n");

	mainsignal_show(signal_current);
}

/* create mainsignal indicator */
int mainsignal_create(struct indicator *ind)
{
	GR_IMAGE_INFO iinfo;

	/* Get the image from theme */
	int ret = theme_get_image(THEME_GROUP_MAINSCREEN, THEME_MAINSIGNAL,
				  &xcoord, &ycoord, &signal_image);

	if (ret == -1)
		return -1;

	if (ret != -2) {
		GrGetImageInfo(signal_image, &iinfo);
		signal_frame_width = iinfo.width / MAINSIGNAL_NUMFRAMES;
		signal_frame_height = iinfo.height;
	}
	/* show indicator */
	mainsignal_show(1);
	/* set the callback */
	ind->callback = &mainsignal_event_callback;
	ind->changed = &mainsignal_changed_callback;
	ind->wind_id = GR_ROOT_WINDOW_ID;

	return 0;
}

/* show current signal state */
static int mainsignal_show(int frame)
{
	/*TODO - check if the root window is displayed, or not */

	if (frame >= MAINSIGNAL_NUMFRAMES)
		frame = MAINSIGNAL_NUMFRAMES - 1;
	else  if (frame < 0)
		frame = 0;
	
	/* draw frame (part) of the image */
	/* clear the area under the indicator to background pixmap */
	GrClearArea(GR_ROOT_WINDOW_ID, xcoord, ycoord,
			signal_frame_width, signal_frame_height, 0);
	/* Draw the indicator */
	GrDrawImagePartToFit(GR_ROOT_WINDOW_ID, gc, xcoord, ycoord,
				signal_frame_width, signal_frame_height,
				signal_frame_width * frame, 0,
				signal_frame_width, signal_frame_height,
				signal_image);
	return 0;
}
