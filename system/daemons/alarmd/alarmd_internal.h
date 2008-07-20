/* alarmdef.h
*
*  main module utils
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#ifndef _alarmdef_h_
#define _alarmdef_h_

#include <nano-X.h>
#include <debug.h>
#include <theme.h>
#include <ipc/shareddata.h>
#include <ipc/alarmd.h>
#include <ipc/tbus.h>

extern struct SharedSystem *shdata;	/* shared memory segment */
extern struct TBusConnection bus;	/* TBUS connection */

/* from ipc.c */
int ipc_handle(int fd);

#endif
