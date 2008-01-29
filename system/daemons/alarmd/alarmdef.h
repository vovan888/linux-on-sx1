/* alarmd.h
*
*  main module utils
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#ifndef _alarmd_h_
#define _alarmd_h_


#include <nano-X.h>
#include <debug.h>
#include <theme.h>
#include <ipc/shareddata.h>
#include <ipc/colosseum.h>
#include <ipc/phoneserver.h>

extern struct SharedSystem *shdata; /* shared memory segment */

/* from ipc.c */
int ipc_start (char * servername);
int ipc_handle (int fd);

#endif
