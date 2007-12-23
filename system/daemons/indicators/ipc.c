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
static int val = 0;

/* Register with IPC server */
int ipc_start(unsigned char * servername)
{
	int cl_flags;
	
	client_fd = ClRegister(servername, &cl_flags);
	
	if (client_fd <= 0)
		fprintf(stderr,"%s: Unable to locate the IPC server.\n",servername);
	else
		GrRegisterInput(client_fd);
	return 0;
}

/* Handle IPC message 
 * Message format [Group][Indicator] - 2 bytes
 * This message only tells indicator that its value is changed
 * Actual value is stored in sharedmem
*/
int ipc_handle (GR_EVENT * e)
{
	int ack = 0, size = 32;
	unsigned short src = 0;
	unsigned char msg[32];

	if( (ack = ClGetMessage(&msg, &size, &src)) < 0 )
		return ack;
	if (ack == CL_CLIENT_BROADCAST)
		/**/;
	if (msg[0] == '0') {
		/* DEBUG message from nanowm */
		indicators[0].changed(val++);
		if (val > 5)
			val = 0;
	}
	
	if (msg[0] == THEME_GROUP_MAINSCREEN) {
		if(indicators[msg[1]].changed)
			indicators[msg[1]].changed(1);
	}
}
