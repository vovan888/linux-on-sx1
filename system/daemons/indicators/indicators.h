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
#include <debug.h>
#include <theme.h>
#include <ipc/shareddata.h>
//#include <ipc/colosseum.h>
#include <ipc/phoneserver.h>
#include <ipc/alarmd.h>
#include <ipc/tbus.h>

typedef void (*event_callback_p) (GR_WINDOW_ID, GR_EVENT *);
typedef void (*changed_callback_p) (int);

struct indicator {
	event_callback_p callback;	/* event callback function */
	changed_callback_p changed;	/* "changed" callback function */
	GR_WINDOW_ID wind_id;	/* window ID */
};

/* from main.c */
extern struct indicator indicators[16];
extern struct SharedSystem *shdata;	/* shared memory segment */
extern GR_GC_ID gc;		/* current graphic context */

/* from ipc.c */
int ipc_active(void);
int ipc_start(char *servername);
int ipc_handle(GR_EVENT * e);
extern struct TBusConnection bus;	/* TBUS connection */

/* from mainbattery.c */
int mainbattery_create(struct indicator *ind);

/* from mainsignal.c */
int mainsignal_create(struct indicator *ind);

/* datetime.c - create maindatetime indicator */
int maindatetime_create(struct indicator *ind);

#endif
