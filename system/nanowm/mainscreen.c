/*
 * Mainscreen utils for NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2007 Vladimir Ananiev (vovan888 at gmail com)
 */

#include <stdio.h>
#include <nano-X.h>

/* 
 * Load wallpaper "filename" to the background of "wid" window
 */
int wm_loadwallpaper(GR_WINDOW_ID wid, char * filename)
{
	GR_GC_ID gc;
	GR_IMAGE_ID iid;
	GR_WINDOW_ID pid;
	GR_IMAGE_INFO iif;

	if(!(iid = GrLoadImageFromFile(filename, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", filename);
		return 0;
	}
	GrGetImageInfo(iid, &iif);
	pid = GrNewPixmap(iif.width, iif.height, NULL);
	gc = GrNewGC();
	GrDrawImageToFit(pid, gc, 0, 0, iif.width, iif.height, iid);
	GrDestroyGC(gc);
	GrFreeImage(iid);

	GrSetBackgroundPixmap(wid, pid, GR_BACKGROUND_CENTER);
	
	GrClearWindow(GR_ROOT_WINDOW_ID, GR_TRUE);

	return 0;
}
