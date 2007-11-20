/* ipc.c
*
*  Common IPC functions for all daemons
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/

#include <flphone_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include <ipc/colosseum.h>
#include <nano-X.h>
#include "indicators.h"

static int client_fd = 0;

int ipc_active (void)
{
	return ((client_fd > 0) ? 1 : 0);
}

/* Register with IPC server */
int ipc_start(char * servername)
{
	int cl_flags;
	
	client_fd = ClRegister(servername, &cl_flags);
	
	if (client_fd <= 0)
		fprintf(stderr,"indicatord: Unable to locate the IPC server.\n");
	else
		GrRegisterInput(client_fd);
}

/* Handle IPC message */
int ipc_handle (GR_EVENT_FD_ACTIVITY * e)
{
	
}
