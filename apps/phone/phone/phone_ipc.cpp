/* phone_ipc.cpp
*
*  Common IPC functions for all programs
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/
extern "C" {
#include <flphone_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <nano-X.h>
#include <debug.h>
#include <theme.h>
#include <ipc/shareddata.h>
#include <ipc/colosseum.h>
}

int init_ipc()
{
	int cl_flags, client_fd;

	client_fd = ClRegister("phone", &cl_flags);

	if (client_fd <= 0)
		fprintf(stderr, "phone: Unable to locate the IPC server.\n");
	return client_fd;
}


/* Handle IPC message
 * This message only tells indicator that its value is changed
 * Actual value is stored in sharedmem
*/
void handle_ipc(int fd, void * data)
{
	int ack = 0, size = 64;
	unsigned short src = 0;
	unsigned char msg_buf[64];

	DBGMSG("alarmserver: ipc_handle\n");

	if ((ack = ClGetMessage(&msg_buf, &size, &src)) < 0)
		return;

	if (ack == CL_CLIENT_BROADCAST) {
		/* handle broadcast message */
	}

	/*      if (IS_GROUP_MSG(src))
	   ipc_group_message(src, msg_buf); */
}

