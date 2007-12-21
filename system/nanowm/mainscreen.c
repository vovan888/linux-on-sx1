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
	GR_IMAGE_ID image_id;
	GR_WINDOW_ID pid;
	GR_IMAGE_INFO iif;
	int xcoord, ycoord;
	GR_GC_ID gc;

	if( theme_get_image( group_index, image_index, &xcoord, &ycoord, &image_id) ) {
		return -1;
	} else {
		GrGetImageInfo(image_id, &iif);
		pid = GrNewPixmap(iif.width, iif.height, NULL);
		gc = GrNewGC();
		GrDrawImageToFit(pid, gc, 0, 0, -1, -1, image_id);
		GrSetBackgroundPixmap(wid, pid, GR_BACKGROUND_TOPLEFT | GR_BACKGROUND_TRANS);
		GrClearWindow(GR_ROOT_WINDOW_ID, GR_TRUE);
		GrDestroyGC(gc);
		GrFreeImage(image_id);
	}
	return 0;
}
