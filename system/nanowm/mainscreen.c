/*
 * Mainscreen utils for NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2007 Vladimir Ananiev (vovan888 at gmail com)
 *
 * Licensed under GPLv2, see LICENSE
*/

#include <stdio.h>
#include <nano-X.h>

#include "nanowm.h"

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

	if (theme_get_image(group_index, image_index, &xcoord, &ycoord, &image_id)) {
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

/* Paint the Status Area of every Application window.
   Taken from nxdraw.c */
void
wm_paint_statusarea(GR_DRAW_ID id, int w, int h, GR_CHAR * title, GR_BOOL active, GR_WM_PROPS props)
{
	int x = 0;
	int y = 0;
	GR_GC_ID gc = GrNewGC();
//	GR_RECT r;
	/* *static GR_FONT_ID fontid = 0;** */

#if 0
	if (props & GR_WM_PROPS_APPFRAME) {
		/* draw 2-line 3d border around window */
		nxDraw3dOutset(id, x, y, w, h);
		x += 2;
		y += 2;
		w -= 4;
		h -= 4;

		/* draw 1-line inset inside border */
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
		GrRect(id, gc, x, y, w, h);
		x += 1;
		y += 1;
		w -= 2;
		h -= 2;
	} else if (props & GR_WM_PROPS_BORDER) {
		/* draw 1-line black border around window */
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_WINDOWFRAME));
		GrRect(id, gc, x, y, w, h);
		x += 1;
		y += 1;
		w -= 2;
		h -= 2;
	}
#endif
	if (!(props & GR_WM_PROPS_CAPTION))
		goto out;

	/* fill caption */
	GrSetGCForeground(gc,
			  GrGetSysColor(active ? GR_COLOR_ACTIVECAPTION :
					GR_COLOR_INACTIVECAPTION));
	GrFillRect(id, gc, x, y, w, APPVIEW_STATUS_HEIGHT);

	/* draw caption text */
	if (title) {
		GrSetGCForeground(gc,
				  GrGetSysColor(active ? GR_COLOR_ACTIVECAPTIONTEXT :
						GR_COLOR_INACTIVECAPTIONTEXT));
		GrSetGCUseBackground(gc, GR_FALSE);
		/* * no need to create special font now... TODO
		   if (!fontid)
		   fontid = GrCreateFont(GR_FONT_GUI_VAR, 0, NULL);
		   GrSetGCFont(gc, fontid);** */
		GrText(id, gc, x + 4, y - 1, title, -1, GR_TFASCII | GR_TFTOP);
	}
	y += APPVIEW_STATUS_HEIGHT;

	/* draw one line under caption */
	if (props & GR_WM_PROPS_APPFRAME) {
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
		GrLine(id, gc, x, y, x + w - 1, y);
	}
#if 0
	if (props & GR_WM_PROPS_CLOSEBOX) {
		/* draw close box */
		r.x = x + w - CXCLOSEBOX - 2;
		r.y = y - APPVIEW_STATUS_HEIGHT + 2;
		r.width = CXCLOSEBOX;
		r.height = CYCLOSEBOX;

		nxDraw3dBox(id, r.x, r.y, r.width, r.height,
			    GrGetSysColor(GR_COLOR_BTNHIGHLIGHT),
			    GrGetSysColor(GR_COLOR_WINDOWFRAME));
		nxInflateRect(&r, -1, -1);
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
		GrFillRect(id, gc, r.x, r.y, r.width, r.height);

		nxInflateRect(&r, -1, -1);
		GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_BTNTEXT));
		GrLine(id, gc, r.x, r.y, r.x + r.width - 1, r.y + r.height - 1);
		GrLine(id, gc, r.x, r.y + r.height - 1, r.x + r.width - 1, r.y);
	}
#endif
#if 0
	/* fill in client area */
	y++;
	h -= APPVIEW_STATUS_HEIGHT + 1;
	GrSetGCForeground(gc, GrGetSysColor(GR_COLOR_APPWINDOW));
	GrFillRect(id, gc, x, y, w, h);
#endif

      out:
	GrDestroyGC(gc);
}

/*
 * GR_WM_PROPS_NOBACKGROUND
 * GR_WM_PROPS_NOFOCUS
 * GR_WM_PROPS_NOMOVE
 * GR_WM_PROPS_NORAISE
 * GR_WM_PROPS_NODECORATE
 * GR_WM_PROPS_NOAUTOMOVE
 * GR_WM_PROPS_NOAUTORESIZE
 *
 * GR_WM_PROPS_BORDER
 * GR_WM_PROPS_APPFRAME
 * GR_WM_PROPS_CAPTION
 * GR_WM_PROPS_CLOSEBOX
 * GR_WM_PROPS_MAXIMIZE
 * GR_WM_PROPS_APPWINDOW
 *
 */

/*	Show application switcher
 */
