/*
 * Display current network operator
 *
 * Copyright (C) 2008 Vladimir Ananiev (vovan888 at gmail com)
 *
 * Licensed under GPLv2, see LICENSE
*/

#include <flphone_config.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <flphone/theme.h>

#include <nxcolors.h>
#include "indicators.h"

static GR_SIZE operator_frame_width;
static GR_SIZE operator_frame_height;
static GR_SIZE operator_frame_baseheight;
/*static struct tm operator_current;*/
static int xcoord, ycoord;	/* XY coordinates of indicator */

static int operator_show(void);

static void operator_changed_callback(int new_value)
{
	DBGMSG("operator_changed_callback\n");

	operator_show();
}

static void operator_event_callback(GR_WINDOW_ID window, GR_EVENT * event)
{
	DBGMSG("operator_event_callback\n");

	operator_show();
}

/* create operator indicator */
int operator_create(struct indicator *ind)
{
	/* set the callback */
	ind->callback = &operator_event_callback;
	ind->changed = &operator_changed_callback;
	ind->wind_id = GR_ROOT_WINDOW_ID;

	/*Set the indicator text style */
	GrSetGCForeground(gc, GR_COLOR_BLACK);
	GrSetGCUseBackground(gc, GR_FALSE);

	/* show indicator */
	operator_show();
	return 0;
}

/* show current operator state */
static int operator_show(void)
{
	time_t curtime;
	struct tm *loctime;
	char *opname = shdata->PhoneServer.Network_Operator;
	int creg_state = shdata->PhoneServer.CREG_State;
	int registered = 0;
	char opname_new[32];

	if (creg_state == GSMD_NETREG_REG_HOME) {
		strncpy(opname_new, opname, 32);
		registered = 1;
	}
	if (creg_state == GSMD_NETREG_REG_ROAMING) {
		strncpy(opname_new, "*", 32);
		strncat(opname_new, opname, 32);
		registered = 1;
	}

	if(registered) {
		GrGetGCTextSize(gc, opname_new, -1, GR_TFUTF8, &operator_frame_width,
				&operator_frame_height, &operator_frame_baseheight);
	}
	/* clear the area under the indicator to background pixmap */
	GrClearArea(GR_ROOT_WINDOW_ID, xcoord, ycoord,
		    operator_frame_width, operator_frame_height, 0);

	if(registered) {
		/* Draw operator string */
		GrText(GR_ROOT_WINDOW_ID, gc, xcoord, ycoord, opname_new, -1, GR_TFUTF8 | GR_TFTOP);
	}

	return 0;
}
