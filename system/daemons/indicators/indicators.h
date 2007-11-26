/* indicators.h
*
*  main module utils
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#ifndef _indicators_h_
#define _indicators_h_


#include <nano-X.h>

#include <theme.h>

typedef void (*event_callback_p)(GR_WINDOW_ID, GR_EVENT *);
typedef void (*timeout_callback_p)(void);


struct indicator {
	int image_index;	/* image index in theme file */
	int frames_num;		/* number of frames, if any */
	int frame_current;	/* current frame */
	event_callback_p callback;	/* event callback function */

	GR_WINDOW_ID pict_id;	/* picture ID*/
	GR_WINDOW_ID wind_id;	/* window ID*/
	int width;
	int height;
	int xcoord;
	int ycoord;
	int frame_width;	/* width of one frame */

};

/* from main.c */
extern struct indicator indicators[16];

/* from ipc.c */
int ipc_active (void);
int ipc_start(char * servername);
int ipc_handle (GR_EVENT * e);

/* from interface.c */
int multi_init (struct indicator * ind);


#endif
