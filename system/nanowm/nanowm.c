/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 * Parts based on npanel.c Copyright (C) 1999 Alistair Riddoch.
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include <nano-X.h>
#include <libini.h>
#include <theme.h>
#include <libflphone.h>
/* Uncomment this if you want debugging output from this file */
/*#define DEBUG*/

#include "nanowm.h"

GR_SCREEN_INFO si;

/* Register with IPC server */
int ipc_start(unsigned char * servername)
{
	int cl_flags, client_fd;

	client_fd = ClRegister(servername, &cl_flags);

	if (client_fd <= 0)
		fprintf(stderr,"%s : Unable to locate the IPC server.\n", servername);
	else
		GrRegisterInput(client_fd);

	return client_fd;
}

/* Handle IPC message */
int ipc_handle (GR_EVENT * e)
{
	int ack = 0, size = 32;
	unsigned short src = 0;
	char msg[32];

	ack = ClGetMessage(&msg, &size, &src);

	if (ack < 0)
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	GR_EVENT event;
	GR_WM_PROPERTIES props;
	win window;

	if(GrOpen() < 0) {
		fprintf(stderr, "Couldn't connect to Nano-X server!\n");
		exit(1);
	}

	/* pass errors through main loop, don't exit*/
	GrSetErrorHandler(NULL);

	GrGetScreenInfo(&si);

	/* add root window*/
	window.wid = GR_ROOT_WINDOW_ID;
	window.pid = GR_ROOT_WINDOW_ID;
	window.type = WINDOW_TYPE_ROOT;
	window.clientid = 1;
	window.sizing = GR_FALSE;
	window.active = 0;
	window.data = NULL;
	add_window(&window);

	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_CHLD_UPDATE |
				GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_KEY_UP |
				GR_EVENT_MASK_SCREENSAVER | GR_EVENT_MASK_TIMER /*| GR_EVENT_MASK_FDINPUT*/);

	ipc_start("nanowm");

	GrGrabKey(GR_ROOT_WINDOW_ID, Menu ,GR_GRAB_EXCLUSIVE);

	/* Set new root window background color*/
	props.flags = GR_WM_FLAGS_BACKGROUND;
	props.background = GrGetSysColor(GR_COLOR_DESKTOP);
	GrSetWMProperties(GR_ROOT_WINDOW_ID, &props);
	
	/* load wallpaper to the root window */
	wm_loadwallpaper(GR_ROOT_WINDOW_ID, THEME_GROUP_MAINSCREEN, THEME_WALLPAPER);

	while(1) { 
		GrGetNextEvent(&event);

		switch(event.type) {
			case GR_EVENT_TYPE_ERROR:
				printf("nanowm: error %d\n", event.error.code);
				break;
			case GR_EVENT_TYPE_EXPOSURE:
				do_exposure(&event.exposure);
				break;
			case GR_EVENT_TYPE_BUTTON_DOWN:
				do_button_down(&event.button);
				break;
			case GR_EVENT_TYPE_BUTTON_UP:
				do_button_up(&event.button);
				break;
			case GR_EVENT_TYPE_MOUSE_ENTER:
				do_mouse_enter(&event.general);
				break;
			case GR_EVENT_TYPE_MOUSE_EXIT:
				do_mouse_exit(&event.general);
				break;
			case GR_EVENT_TYPE_MOUSE_POSITION:
				do_mouse_moved(&event.mouse);
				break;
			case GR_EVENT_TYPE_KEY_DOWN:
				do_key_down(&event.keystroke);
				break;
			case GR_EVENT_TYPE_KEY_UP:
				do_key_up(&event.keystroke);
				break;
			case GR_EVENT_TYPE_FOCUS_IN:
				do_focus_in(&event.general);
				break;
			case GR_EVENT_TYPE_CHLD_UPDATE:
				do_update(&event.update);
				break;
			default:
				fprintf(stderr, "Got unexpected event %d\n",
								event.type);
				break;
		}
	}

	GrClose();
}
