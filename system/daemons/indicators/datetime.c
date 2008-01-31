/* datetime.c
*
*  Display date and time on MainScreen
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#include <flphone_config.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <theme.h>

#include <nxcolors.h>
#include "indicators.h"

static GR_IMAGE_ID datetime_image;
static GR_SIZE datetime_frame_width;
static GR_SIZE datetime_frame_height;
static GR_SIZE datetime_frame_baseheight;
/*static struct tm datetime_current;*/
static int xcoord, ycoord;	/* XY coordinates of indicator */

static int maindatetime_show(void);

static void maindatetime_changed_callback(int new_value)
{
	//      datetime_current = shdata -> datetime.capacity;

	maindatetime_show();
}

static void maindatetime_event_callback(GR_WINDOW_ID window, GR_EVENT * event)
{
	DBGMSG("maindatetime_event_callback\n");

	maindatetime_show();
}

/* create maindatetime indicator */
int maindatetime_create(struct indicator *ind)
{
	/* Get the image from theme */
	int ret = theme_get_image(THEME_GROUP_MAINSCREEN, THEME_DATETIME,
				  &xcoord, &ycoord, &datetime_image);

	if (ret == -1)
		return -1;

	/* set the callback */
	ind->callback = &maindatetime_event_callback;
	ind->changed = &maindatetime_changed_callback;
	ind->wind_id = GR_ROOT_WINDOW_ID;

	/*Set the indicator text style */
//      GrSetGCFont(gc, GrCreateFont(GR_FONT_SYSTEM_FIXED, 0, NULL));
	GrSetGCForeground(gc, GR_COLOR_BLACK);
	GrSetGCUseBackground(gc, GR_FALSE);

	/* show indicator */
	maindatetime_show();
	return 0;
}

/* show current datetime state */
static int maindatetime_show(void)
{
	/*TODO - check if the root window is displayed, or not */
	/*FIXME just a test implementation */
	time_t curtime;
	struct tm *loctime;
	char string_time[32];

	/* Get the current time. */
	curtime = time(NULL);
	/* Convert it to local time representation. */
	loctime = localtime(&curtime);
	/* Convert to the string */
	asctime_r(loctime, string_time);

	GrGetGCTextSize(gc, string_time, -1, GR_TFASCII, &datetime_frame_width,
			&datetime_frame_height, &datetime_frame_baseheight);
	/* clear the area under the indicator to background pixmap */
	GrClearArea(GR_ROOT_WINDOW_ID, xcoord, ycoord,
		    datetime_frame_width, datetime_frame_height, 0);
	/* Draw time string */
	GrText(GR_ROOT_WINDOW_ID, gc, xcoord, ycoord, string_time, -1,
	       GR_TFASCII | GR_TFTOP);
	return 0;
}
