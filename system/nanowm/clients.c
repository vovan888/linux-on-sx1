/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000, 2003 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

/* Vovan888: Assume that application window is already on its correct position so do not automove
 * and autoresize
 */

#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"
#include "nxdraw.h"

#include "nanowm.h"

/* default window style for GR_WM_PROPS_APPWINDOW*/
#define DEFAULT_WINDOW_STYLE  ( GR_WM_PROPS_CAPTION | GR_WM_PROPS_CLOSEBOX | \
				GR_WM_PROPS_NOAUTOMOVE | GR_WM_PROPS_NOAUTORESIZE | \
				GR_WM_PROPS_NODECORATE )
/*				GR_WM_PROPS_APPFRAME | */
#define DEFAULT_APP_STYLE  ( GR_WM_PROPS_NODECORATE )
#define DEFAULT_MENU_STYLE  ( GR_WM_PROPS_NODECORATE | GR_WM_PROPS_MENU)

/*static GR_COORD lastx = FIRST_WINDOW_LOCATION;
static GR_COORD lasty = FIRST_WINDOW_LOCATION;
*/

/*
 * A new client window has been mapped, so we need to reparent and decorate it.
 * Returns -1 on failure or 0 on success.
 */
int new_client_window(GR_WINDOW_ID wid)
{
	win window;
	GR_WINDOW_ID pid;
	GR_WINDOW_INFO winfo;
	GR_COORD x, y, width, height, xoffset, yoffset;
	GR_WM_PROPS style;
	GR_WM_PROPERTIES props;
	int wtype;	/* window type */

	/* get client window information */
	GrGetWindowInfo(wid, &winfo);
	style = winfo.props;

	/* if not redecorating or not child of root window, return */
	if (winfo.parent != GR_ROOT_WINDOW_ID || (style & GR_WM_PROPS_NODECORATE))
		return 0;
#if 0
	/* deal with replacing borders with window decorations */
	if (winfo.bordersize) {
		/*
		 * For complex reasons, it's easier to unmap,
		 * remove the borders, and then map again,
		 * rather than try to recalculate the window
		 * position in the server w/o borders.  By
		 * the time we get this event, the window has
		 * already been painted with borders...
		 * This currently causes a screen flicker as
		 * the window is painted twice.  The workaround
		 * is to create the window without borders in
		 * the first place.
		 */
		GrUnmapWindow(wid);

		/* remove client borders, if any */
		props.flags = style | GR_WM_FLAGS_BORDERSIZE;
		props.bordersize = 0;
		GrSetWMProperties(wid, &props);

		/* remap the window without borders, call this routine again */
		GrMapWindow(wid);
		return 0;
	}
#endif
	/* if default decoration style asked for, set real draw bits */
	if ((style & GR_WM_PROPS_APPMASK) == GR_WM_PROPS_APPWINDOW) {
		style = (style & ~GR_WM_PROPS_APPMASK) | DEFAULT_APP_STYLE;
		wtype = WINDOW_TYPE_APP;
	} else if (style & GR_WM_PROPS_MENU) {
		style = DEFAULT_MENU_STYLE;
		wtype = WINDOW_TYPE_MENU;
	} else {
		style = DEFAULT_WINDOW_STYLE;
		wtype = WINDOW_TYPE_CLIENT;
	}
	GR_WM_PROPERTIES pr;
	pr.flags = GR_WM_FLAGS_PROPS;
	pr.props = style;
	GrSetWMProperties(wid, &pr);

	/* determine container widths and client child window offsets */
	if (style & GR_WM_PROPS_APPFRAME) {
		width = winfo.width + CXFRAME;
		height = winfo.height + CYFRAME;
		xoffset = CXBORDER;
		yoffset = CYBORDER;
	} else if (style & GR_WM_PROPS_BORDER) {
		width = winfo.width + 2;
		height = winfo.height + 2;
		xoffset = 1;
		yoffset = 1;
	} else {
		width = winfo.width;
		height = winfo.height;
		xoffset = 0;
		yoffset = 0;
	}
	if (style & GR_WM_PROPS_CAPTION) {
		height += APPVIEW_STATUS_HEIGHT;
		yoffset += APPVIEW_STATUS_HEIGHT;
		if (style & GR_WM_PROPS_APPFRAME) {
			/* extra line under caption with appframe */
			++height;
			++yoffset;
		}
	}
#if 0
	/* determine x,y window location */
	if (style & GR_WM_PROPS_NOAUTOMOVE) {
		x = winfo.x;
		y = winfo.y;
	} else {
		/* We could proably use a more intelligent algorithm here */
		x = lastx + WINDOW_STEP;
		if ((x + width) > si.cols)
			x = FIRST_WINDOW_LOCATION;
		lastx = x;
		y = lasty + WINDOW_STEP;
		if ((y + height) > si.rows)
			y = FIRST_WINDOW_LOCATION;
		lasty = y;
	}
#else
	/* we assume that application window is already placed on the right location
	   here we set container window coords */
	x = 0;
	y = 0;
#endif

	if (wtype == WINDOW_TYPE_CLIENT) {
		/* create container window */
		pid = GrNewWindow(GR_ROOT_WINDOW_ID, x, y, width, height, 0, LTGRAY, BLACK);
		window.wid = pid;
		window.pid = GR_ROOT_WINDOW_ID;
		window.type = WINDOW_TYPE_CONTAINER;
	/*	window.sizing = GR_FALSE;
		window.active = GR_TRUE;*/
		window.state = WM_STATE_ACTIVE;
		window.data = NULL;
		window.clientid = wid;
		add_window(&window);

		/* don't erase background of container window */
		props.flags = GR_WM_FLAGS_PROPS;
		props.props = style | GR_WM_PROPS_NOBACKGROUND;
		GrSetWMProperties(pid, &props);

		Dprintf("New client window %d container %d\n", wid, pid);

		GrSelectEvents(pid, GR_EVENT_MASK_CHLD_UPDATE | GR_EVENT_MASK_UPDATE
			| GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_BUTTON_DOWN
			/*| GR_EVENT_MASK_MOUSE_POSITION*/ | GR_EVENT_MASK_EXPOSURE);

		/* reparent client to container window (child is already mapped) */
		GrReparentWindow(wid, pid, xoffset, yoffset);

		/* map container window */
		GrMapWindow(pid);
	}

	GrSetFocus(wid);	/* force fixed focus */

	/* add client window */
	window.wid = wid;
	window.pid = pid;
	window.type = wtype;
/*	window.sizing = GR_FALSE;
	window.active = GR_TRUE;*/
	window.state = WM_STATE_ACTIVE;
	window.clientid = 0;
	window.data = NULL;
	add_window(&window);

#if 0000
	/* add system utility button */
	nid = GrNewWindowWithTitle(pid, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0,
				   LTGRAY, BLACK, "clients_b");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_UTILITYBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
		 utilitybutton_notpressed);

	nid = GrNewWindowWithTitle(pid, TITLE_BAR_HEIGHT + 1, 1, width - (4 *
									  TITLE_BAR_HEIGHT) - 3,
				   TITLE_BAR_HEIGHT - 3, 1, LTGRAY, BLACK, "clients_c");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_TOPBAR;
	window.active = GR_FALSE;
	window.data = NULL;

	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(nid);

	nid = GrNewWindowWithTitle(pid, width - (3 * TITLE_BAR_HEIGHT), 0,
				   TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0, LTGRAY, BLACK,
				   "clients_d");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_ICONISEBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
		 iconisebutton_notpressed);

	nid = GrNewWindowWithTitle(pid, width - (2 * TITLE_BAR_HEIGHT), 0,
				   TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0, LTGRAY, BLACK,
				   "clients_e");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_MAXIMISEBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT,
		 maximisebutton_notpressed);

	nid = GrNewWindowWithTitle(pid, width - TITLE_BAR_HEIGHT, 0,
				   TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, 0, LTGRAY, BLACK,
				   "clients_f");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_CLOSEBUTTON;
	window.active = GR_FALSE;
	window.data = NULL;
	add_window(&window);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_MOUSE_EXIT);
	GrMapWindow(nid);
	GrBitmap(nid, buttonsgc, 0, 0, TITLE_BAR_HEIGHT, TITLE_BAR_HEIGHT, closebutton_notpressed);

	nid = GrNewWindowWithTitle(pid, 1, TITLE_BAR_HEIGHT + 1, BORDER_WIDTHS - 2,
				   height - TITLE_BAR_HEIGHT - BORDER_WIDTHS - 1,
				   1, LTGRAY, BLACK, "clients_g");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_LEFTBAR;
	window.active = GR_FALSE;
	window.data = NULL;

	add_window(&window);

	GrSetCursor(nid, horizontal_resize_columns, horizontal_resize_rows,
		    horizontal_resize_hotx, horizontal_resize_hoty,
		    BLACK, WHITE, horizontal_resize_fg, horizontal_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindowWithTitle(pid, 1, height - BORDER_WIDTHS + 1, BORDER_WIDTHS - 2,
				   BORDER_WIDTHS - 2, 1, LTGRAY, BLACK, "clients_h");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_LEFTRESIZE;
	window.active = GR_FALSE;
	window.data = NULL;

	add_window(&window);

	GrSetCursor(nid, lefthand_resize_columns, lefthand_resize_rows,
		    lefthand_resize_hotx, lefthand_resize_hoty,
		    BLACK, WHITE, lefthand_resize_fg, lefthand_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindowWithTitle(pid, BORDER_WIDTHS, height - BORDER_WIDTHS + 1,
				   width - (2 * BORDER_WIDTHS), BORDER_WIDTHS - 2, 1,
				   LTGRAY, BLACK, "clients_i");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_BOTTOMBAR;
	window.active = GR_FALSE;
	window.data = NULL;
	add_window(&window);

	GrSetCursor(nid, vertical_resize_columns, vertical_resize_rows,
		    vertical_resize_hotx, vertical_resize_hoty,
		    BLACK, WHITE, vertical_resize_fg, vertical_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindowWithTitle(pid, width - BORDER_WIDTHS + 1,
				   height - BORDER_WIDTHS + 1, BORDER_WIDTHS - 2,
				   BORDER_WIDTHS - 2, 1, LTGRAY, BLACK, "clients_j");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_RIGHTRESIZE;
	window.active = GR_FALSE;
	window.data = NULL;

	add_window(&window);

	GrSetCursor(nid, righthand_resize_columns, righthand_resize_rows,
		    righthand_resize_hotx, righthand_resize_hoty,
		    BLACK, WHITE, righthand_resize_fg, righthand_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_MOUSE_POSITION);

	GrMapWindow(nid);

	nid = GrNewWindowWithTitle(pid, width - BORDER_WIDTHS + 1, TITLE_BAR_HEIGHT + 1,
				   BORDER_WIDTHS - 2, height - TITLE_BAR_HEIGHT - BORDER_WIDTHS - 1,
				   1, LTGRAY, BLACK, "clients_k");
	window.wid = nid;
	window.pid = pid;
	window.type = WINDOW_TYPE_RIGHTBAR;
	window.active = GR_FALSE;
	window.data = NULL;

	add_window(&window);

	GrSetCursor(nid, horizontal_resize_columns, horizontal_resize_rows,
		    horizontal_resize_hotx, horizontal_resize_hoty,
		    BLACK, WHITE, horizontal_resize_fg, horizontal_resize_bg);

	GrSelectEvents(nid, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP
		       | GR_EVENT_MASK_MOUSE_POSITION);
	GrMapWindow(nid);
#endif
	return 0;
}

void client_window_remap(win * window)
{

	GR_WINDOW_INFO winfo;
	win *pwin;

	if (!(pwin = find_window(window->pid))) {
		fprintf(stderr, "Couldn't find parent of destroyed window " "%d\n", window->wid);
		return;
	}

	Dprintf("client_window_remap %d (parent %d)\n", window->wid, window->pid);
	GrGetWindowInfo(pwin->wid, &winfo);
	container_show(pwin);
//	if (winfo.mapped == GR_FALSE)
//		GrMapWindow(pwin->wid);
}

/* If the client chooses to unmap the window, then we should also unmap the container */

void client_window_unmap(win * window)
{
	win *pwin;

	if (!(pwin = find_window(window->pid))) {
		fprintf(stderr, "Couldn't find parent of unmapped window " "%d\n", window->wid);
		return;
	}
	Dprintf("client_window_unmap %d (parent %d)\n", window->wid, window->pid);

	container_hide(pwin);
}

void client_window_resize(win * window)
{
	win *pwin;
	GR_COORD width, height;
	GR_WM_PROPS style;
	GR_WINDOW_INFO winfo;

	Dprintf("client_window_resize %d (parent %d)\n", window->wid, window->pid);
	if (!(pwin = find_window(window->pid))) {
		fprintf(stderr, "Couldn't find parent of resize window %d\n", window->wid);
		return;
	}

	/* get client window style and size, determine new container size */
	GrGetWindowInfo(pwin->clientid, &winfo);
	style = winfo.props;

	if (style & GR_WM_PROPS_APPFRAME) {
		width = winfo.width + CXFRAME;
		height = winfo.height + CYFRAME;
	} else if (style & GR_WM_PROPS_BORDER) {
		width = winfo.width + 2;
		height = winfo.height + 2;
	} else {
		width = winfo.width;
		height = winfo.height;
	}
	if (style & GR_WM_PROPS_CAPTION) {
		height += APPVIEW_STATUS_HEIGHT;
		if (style & GR_WM_PROPS_APPFRAME) {
			/* extra line under caption with appframe */
			++height;
		}
	}

	/* resize container window based on client size */
	GrResizeWindow(pwin->wid, width, height);
}

/*
 * We've just received an event notifying us that a client window has been
 * unmapped, so we need to destroy all of the decorations.
 */
void client_window_destroy(win * window)
{
	win *pwin, *zwin;
	GR_WINDOW_ID pid;

	Dprintf("Client window %d has been destroyed\n", window->wid);

	if (!(pwin = find_window(window->pid))) {
		fprintf(stderr, "Couldn't find parent of destroyed window " "%d\n", window->wid);
		return;
	}

	/* Do it this way around so we don't handle events after destroying */
	pid = pwin->wid;
	remove_window_and_children(pwin);

	zwin = get_top_window();
	if (zwin != NULL)
		container_show(zwin);

	Dprintf("Destroying container %d\n", pid);
	GrDestroyWindow(pid);
}

/* ----------------------------------------------------------- */
void remap_app_window(win * window)
{
	Dprintf("remap_app_window %d\n", window->wid);
}

void unmap_app_window(win * window)
{
	Dprintf("unmap_app_window %d\n", window->wid);
}

void destroy_app_window(win * window)
{
	win *zwin;
	Dprintf("destroy_app_window - %d\n", window->wid);

	remove_window_and_children(window);

	zwin = get_top_window();
	if (zwin != NULL)
		container_show(zwin);
}
