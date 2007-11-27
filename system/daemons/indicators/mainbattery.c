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

#include <nano-X.h>
#include <nxcolors.h>
#include "indicators.h"

static GR_IMAGE_ID	battery_images[N_MAX_IMAGES];
static GR_IMAGE_ID	battery_image;
//static	GR_WINDOW_ID	battery_window;
static	int	battery_frame_width;
static	int	battery_frame_height;
static	int	battery_current = 1; /*FIXME*/
static GR_GC_ID		gc;
#define MAINBATTERY_NUMFRAMES	6

static int mainbattery_show(int frame);

static void mainbattery_changed_callback(int new_value)
{
	if (new_value == battery_current)
		return;
	battery_current = new_value;

	mainbattery_show(battery_current);
}

static void mainbattery_event_callback(GR_WINDOW_ID window, GR_EVENT *event)
{
printf("mainbattery_event_callback\n");
	switch(event->type) {
		case GR_EVENT_TYPE_EXPOSURE:
			mainbattery_show(battery_current);
	}
	battery_current++;
}

/* create mainbattery indicator */
int mainbattery_create( struct indicator * ind)
{
	GR_IMAGE_INFO	iinfo;
	int xcoord, ycoord;


	gc = GrNewGC();
	/* Get the image from theme */
	int ret = theme_get_image(THEME_GROUP_MAINSCREEN, THEME_MAINBATTERY,
					&xcoord, &ycoord, &battery_image);

	if (ret)
		return -1;

	GrGetImageInfo(battery_image, &iinfo);
	battery_frame_width = iinfo.width / MAINBATTERY_NUMFRAMES;
	battery_frame_height = iinfo.height;
	
	/* Select events for the ROOT window */
	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_EXPOSURE);
	/* show indicator */
	mainbattery_show(1);
	/* set the callback */
	ind -> callback = &mainbattery_event_callback;
	ind -> changed = &mainbattery_changed_callback;

	return GR_ROOT_WINDOW_ID;	
}

/* show current battery state */
static int mainbattery_show(int frame)
{
	/*TODO - check if the root window is displayed, or not */

	/* draw frame (part) of the image */
	if (frame < MAINBATTERY_NUMFRAMES)
	GrDrawImagePartToFit(GR_ROOT_WINDOW_ID, gc, 0, 0, battery_frame_width, battery_frame_height,
			battery_frame_width * frame, 0, battery_frame_width, battery_frame_height, 
			battery_image);
	return 0;
}
