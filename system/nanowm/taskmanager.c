/* taskmanager.c
*
*  Display task manager
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#define MWINCLUDECOLORS
#include <nano-X.h>

#include "nanowm.h"
#include <ipc/tbus.h>
#include <flphone/theme.h>

static GR_WINDOW_ID tmwid;

static void draw_taskmanager(GR_EVENT *ep)
{
}

static void do_key(GR_EVENT *ep)
{
}

static void do_button(GR_EVENT *ep)
{
}

int show_taskmanager()
{
	GR_EVENT ev;
	GR_WINDOW_ID old_focus_win;

	tmwid = GrNewWindowEx(GR_WM_PROPS_BORDER, "Taskmanager", GR_ROOT_WINDOW_ID,
		0, APPVIEW_STATUS_HEIGHT * 2,
  		APPVIEW_WIDTH, GRID_ICON_HEIGHT + APPVIEW_CONTROL_HEIGHT,
    		RED);

	GrSelectEvents(tmwid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_CLOSE_REQ |
			GR_EVENT_MASK_KEY_UP | GR_EVENT_MASK_BUTTON_UP);
	GrMapWindow(tmwid);

	old_focus_win = GrGetFocus();
	GrSetFocus(tmwid);

	while (1) {
		GrGetNextEvent(&ev);

		if (ev.type == GR_EVENT_TYPE_CLOSE_REQ)
			break;
		if (ev.type == GR_EVENT_TYPE_EXPOSURE)
			draw_taskmanager(&ev);
		if (ev.type == GR_EVENT_TYPE_KEY_UP)
			do_key(&ev);
		if (ev.type == GR_EVENT_TYPE_BUTTON_UP)
			do_button(&ev);
	}

	GrSetFocus(old_focus_win);

	return tmwid;
}
