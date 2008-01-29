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

#include <nano-X.h>
#include "indicators.h"

static int client_fd = 0;
static int val = 0;

/* Register with IPC server */
int ipc_start(char *servername)
{
	int cl_flags;

	client_fd = ClRegister(servername, &cl_flags);

	if (client_fd <= 0)
		fprintf(stderr, "%s: Unable to locate the IPC server.\n",
			servername);
	else
		GrRegisterInput(client_fd);

	/* Subscribe to different groups */
	ClSubscribeToGroup(MSG_GROUP_PHONE);

	return 0;
}

/* handle group messages */
int ipc_group_message(unsigned short src, unsigned char *msg_buf)
{
	if (src == MSG_GROUP_PHONE) {
		struct msg_phone *message = (struct msg_phone *)msg_buf;
		DBGMSG("indicatord: MSG_GROUP_PHONE message from %x, id=%d\n",
		       src, message->id);
		switch (message->id) {
		case MSG_PHONE_NETWORK_BARS:

			break;
		case MSG_PHONE_BATTERY_STATUS:
			break;
		case MSG_PHONE_BATTERY_BARS:
			indicators[THEME_MAINBATTERY].changed(0);
			break;
		}
	}

	return 0;
}

/* Handle IPC message
 * This message only tells indicator that its value is changed
 * Actual value is stored in sharedmem
*/
int ipc_handle(GR_EVENT * e)
{
	int ack = 0, size = 64;
	unsigned short src = 0;
	unsigned char msg_buf[64];

	DBGMSG("indicatord: ipc_handle\n");

	if ((ack = ClGetMessage(&msg_buf, &size, &src)) < 0)
		return ack;

	if (ack == CL_CLIENT_BROADCAST) {
		/* handle broadcast message */
	}

	if (IS_GROUP_MSG(src))
		ipc_group_message(src, msg_buf);

	return 0;
}
