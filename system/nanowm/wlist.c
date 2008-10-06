/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */
#include <stdio.h>
#include <stdlib.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#include "nanowm.h"

static win *windows = NULL;

/*
 * Find the windowlist entry for the specified window ID and return a pointer
 * to it, or NULL if it isn't in the list.
 */
win *find_window(GR_WINDOW_ID wid)
{
	win *w = windows;

	Dprintf("Looking for window %d... ", wid);

	while (w) {
		Dprintf("%d ", w->wid);
		if (w->wid == wid) {
			Dprintf("found it!\n");
			return w;
		}
		w = w->next;
	}

	Dprintf("Nope, %d is not in the list\n", wid);
	return NULL;
}

/*
 * Add a new entry to the top of the windowlist.
 * Returns -1 on failure or 0 on success.
 */
int add_window(win * window)
{
	win *w;

	Dprintf("Adding window %d\n", window->wid);

	if (!(w = malloc(sizeof(win))))
		return -1;

	w->wid = window->wid;
	w->pid = window->pid;
	w->type = window->type;
	w->sizing = GR_FALSE;	/* window->sizing */
	w->active = window->active;
	w->clientid = window->clientid;
	w->data = window->data;
	w->next = windows;
	w->prev = NULL;
	if(windows != NULL)
		windows->prev = w;
	windows = w;

	return 0;
}

/*
 * Remove an entry from the windowlist.
 * Returns -1 on failure or 0 on success.
 */
int remove_window(win * window)
{
	if (window->prev == NULL)
		windows = window->next;
	else
		window->prev->next = window->next;
	if (window->next == NULL)
		window->prev->next = NULL;
	else
		window->next->prev = window->prev;

	if (window->data)
		free(window->data);
	free(window);
	return 0;
}

/*
 * Remove an entry and all it's children from the windowlist.
 * Returns -1 on failure or 0 on success.
 */
int remove_window_and_children(win * window)
{
	win *t, *w = windows;
	GR_WINDOW_ID pid = window->wid;

	Dprintf("Removing window %d and children\n", window->wid);

	while (w) {
		Dprintf("Examining window %d (pid %d)\n", w->wid, w->pid);
		if ((w->pid == pid) || (w == window)) {
			Dprintf("Removing window %d (pid %d)\n", w->wid, w->pid);
			t = w->next;
			remove_window (w);
			w = t;
			continue;
		}
		w = w->next;
	}

	return 0;
}

/** Get top active window from the window stack */
win *get_top_window()
{
	win *w = windows;
// 	while (w) {
// 		if (w->active == GR_TRUE) {
// 			break;
// 		}
// 		w = w->next;
// 	}

	Dprintf("Top window=%d, type=%d\n", w->wid, w->type);
	return w;
}

/** Raise window - move it to the top of the list */
int raise_window(win *window)
{
	Dprintf("Raise window=%d, type=%d\n", window->wid, window->type);
	if(window->prev == NULL)
		return 0;
	
	/* remove window from list */
	if (window->prev == NULL)
		windows = window->next;
	else
		window->prev->next = window->next;
	if (window->next == NULL)
		window->prev->next = NULL;
	else
		window->next->prev = window->prev;

	/* add window to the top */
	window->next = windows;
	window->prev = NULL;
	windows = window;
	return 0;
}
