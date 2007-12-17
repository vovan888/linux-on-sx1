/* mainbattery.c
*
*  Display battery indicator on MainScreen
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

static GR_IMAGE_ID	battery_images[N_MAX_IMAGES];
static GR_IMAGE_ID	battery_image;
//static	GR_WINDOW_ID	battery_window;
static	int	battery_frame_width;
static	int	battery_frame_height;
static	int	battery_current = 0; /*FIXME*/
static GR_GC_ID		gc; /* current Graphic Context */ 
static	int xcoord, ycoord; /* XY coordinates of indicator */
#define MAINBATTERY_NUMFRAMES	6

static int mainbattery_show(int frame);

static void mainbattery_changed_callback(int new_value)
{
	battery_current = shdata -> battery.bars;

	mainbattery_show(battery_current);
}

static void mainbattery_event_callback(GR_WINDOW_ID window, GR_EVENT *event)
{
	DBGMSG("mainbattery_event_callback\n");
	switch(event->type) {
		case GR_EVENT_TYPE_EXPOSURE:
			mainbattery_show(battery_current);
	}
}

/* create mainbattery indicator */
int mainbattery_create( struct indicator * ind)
{
	GR_IMAGE_INFO	iinfo;

	gc = GrNewGC();
	/* Get the image from theme */
	int ret = theme_get_image(THEME_GROUP_MAINSCREEN, THEME_MAINBATTERY,
					&xcoord, &ycoord, &battery_image);

	if (ret == -1)
		return -1;

	if (ret != -2) {
		GrGetImageInfo(battery_image, &iinfo);
		battery_frame_width = iinfo.width / MAINBATTERY_NUMFRAMES;
		battery_frame_height = iinfo.height;
	}
	/* Select events for the ROOT window */
	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE);
	/* show indicator */
	mainbattery_show(1);
	/* set the callback */
	ind -> callback = &mainbattery_event_callback;
	ind -> changed = &mainbattery_changed_callback;
	ind -> wind_id = GR_ROOT_WINDOW_ID;

	return 0;	
}

/* show current battery state */
static int mainbattery_show(int frame)
{
	/*TODO - check if the root window is displayed, or not */

	/* draw frame (part) of the image */
	if (frame < MAINBATTERY_NUMFRAMES) {
		/* clear the area under the indicator to background pixmap */
		GrClearArea(GR_ROOT_WINDOW_ID, xcoord, ycoord, 
			battery_frame_width, battery_frame_height, 0);
		/* Draw the indicator */
		GrDrawImagePartToFit(GR_ROOT_WINDOW_ID, gc, xcoord, ycoord,
			battery_frame_width, battery_frame_height,
			battery_frame_width * frame, 0,
			battery_frame_width, battery_frame_height, 
			battery_image);
	}
	return 0;
}
