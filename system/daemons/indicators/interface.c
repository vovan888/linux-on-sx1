/* interface.c
*
*  Graphics interface utils
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

#if 0

GR_IMAGE_ID int_load_image(char *filename)
{
	GR_GC_ID gc;
	GR_IMAGE_ID iid;
	GR_WINDOW_ID pid;
	GR_IMAGE_INFO iif;

	if (!(iid = GrLoadImageFromFile(filename, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", filename);
		return 0;
	}
	GrGetImageInfo(iid, &iif);
	pid = GrNewPixmap(iif.width, iif.height, NULL);
	gc = GrNewGC();
	printf("load image: %d %d  %d\n", iif.width, iif.height, pid);

	GrDrawImageToFit(pid, gc, 0, 0, -1, -1, iid);
	GrDestroyGC(gc);
	GrFreeImage(iid);

	return pid;
}

static int multi_create(struct indicator *ind)
{
	int xcoord, ycoord;
	GR_WINDOW_ID pid, off_pid;	/* picture ID */
	GR_WINDOW_ID wid;	/* window ID */
//      GR_IMAGE_INFO  iinfo;
	GR_WINDOW_INFO iinfo;
//      GR_IMAGE_ID pid;

	if (ind->frames_num <= 0)
		return -1;

	/* load coords and image from theme */
	int ret =
	    theme_get_image(THEME_GROUP_MAINSCREEN, ind->image_index, &xcoord,
			    &ycoord, &pid);
//      pid = int_load_image("/battery-big.png");
//int ret = 0;
//xcoord = 5; ycoord = 5;
	if (!ret && pid > 0)
		ind->pict_id = pid;
	else {
		ind->pict_id = 0;
		return -1;
	}

	/* create main window */
//      GrGetImageInfo(pid, &iinfo);
	GrGetWindowInfo(pid, &iinfo);
	int frame_width = iinfo.width / ind->frames_num;
	wid = GrNewWindowEx(GR_WM_PROPS_NODECORATE | GR_WM_PROPS_NOBACKGROUND, 0, GR_ROOT_WINDOW_ID, xcoord, ycoord, frame_width, iinfo.height, MWNOCOLOR);	//GR_COLOR_WHITE);
	if (!wid)
		return -1;
	ind->wind_id = wid;
	ind->width = frame_width;
	ind->height = iinfo.height;
	ind->frame_width = frame_width;
	ind->xcoord = xcoord;
	ind->ycoord = ycoord;

	printf("multi: %d %d %d %d\n", wid, ind->frames_num, iinfo.width,
	       iinfo.height);

	/* Select events for this window */
	GrSelectEvents(wid, GR_EVENT_MASK_EXPOSURE);
	/* show window */
	GrMapWindow(wid);

	return 0;
}

static int multi_show(struct indicator *ind)
{
	GR_WINDOW_ID pid = ind->pict_id;
	GR_WINDOW_ID wid = ind->wind_id;
	if (!pid)
		return -1;

	GR_GC_ID gc = GrNewGC();
//      GrDrawImageToFit(wid, gc, 0, 0, -1, -1, pid);
	printf("show: %d %d\n", pid, wid);
//      off_pid = GrNewPixmap(ind-> width, ind-> height, NULL); /* allocate off screen buffer */
	/* copy image from screen to our window */
	GrCopyArea(wid, gc, 0, 0, ind->width, ind->height, GR_ROOT_WINDOW_ID,
		   ind->xcoord, ind->ycoord, MWROP_COPY);
	/* copy new image with alpha blend */
	GrCopyArea(wid, gc, 0, 0, ind->width, ind->height, pid,
		   ind->frame_current * ind->frame_width, 0, MWROP_COPY);

//      GrCopyArea(wid, gc, 0, 0, ind-> width, ind-> height, pid,
//              ind->frame_current * ind->frame_width, 0, MWROP_BLENDCONSTANT | 127);
	GrDestroyGC(gc);
}

static void mainbattery_event_callback(GR_WINDOW_ID window, GR_EVENT * event)
{
	printf("mainbattery_event_callback\n");
	switch (event->type) {
	case GR_EVENT_TYPE_EXPOSURE:
		multi_show(&indicators[0]);
	}
}

static void mainsignal_event_callback(GR_WINDOW_ID window, GR_EVENT * event)
{
	printf("mainsignal_event_callback\n");
//      switch(event->type) {
//              case GR_EVENT_TYPE_EXPOSURE:
	multi_show(&indicators[1]);
//      }
}

int init_mainbattery(struct indicator *ind)
{
	/* MainBattery */
	ind->image_index = THEME_MAINBATTERY;
	ind->frames_num = 6;
	ind->frame_current = 3;
	ind->callback = &mainbattery_event_callback;
	multi_create(ind);
}
int init_mainsignal(struct indicator *ind)
{
	/* MainSignal */
	ind->image_index = THEME_MAINSIGNAL;
	ind->frames_num = 6;
	ind->frame_current = 0;
	ind->callback = &mainsignal_event_callback;
	multi_create(ind);

}
#endif
