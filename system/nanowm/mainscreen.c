/*
 * Mainscreen utils for NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2007 Vladimir Ananiev (vovan888 at gmail com)
 *
 * Licensed under GPLv2, see LICENSE
*/

#include <stdio.h>
#include <nano-X.h>
#include <theme.h>

/* 
 * Load wallpaper "filename" to the background of "wid" window
 */
int wm_loadwallpaper(GR_WINDOW_ID wid, int group_index, int image_index)
{
	GR_WINDOW_ID pid;
	int xcoord, ycoord;

	if( theme_get_image( group_index, image_index, &xcoord, &ycoord, &pid) ) {
		return -1;
	} else {
		GrSetBackgroundPixmap(wid, pid, GR_BACKGROUND_CENTER);
		GrClearWindow(GR_ROOT_WINDOW_ID, GR_TRUE); /*FIXME*/
	}
	return 0;
}
