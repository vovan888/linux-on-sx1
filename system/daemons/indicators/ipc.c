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
#include <ipc/tbus.h>

/* Register with IPC server */
int ipc_start(char *servername)
{
	int sock;
	sock = tbus_register_service(servername);

	if (sock >= 0)
		GrRegisterInput(sock);
	else
		return -1;

	/* Subscribe to different signals */
	tbus_connect_signal("PhoneServer", "NetworkBars");
	tbus_connect_signal("PhoneServer", "BatteryBars");

	tbus_connect_signal("AlarmServer", "PPM");

	tbus_connect_signal("nanowm", "debugkey");

	return 0;
}

/* handle signals */
static int ipc_signal(struct tbus_message *msg)
{
	DPRINT("%d,%s->%s/%s (%s)\n",msg->type, msg->service_sender,
		msg->service_dest, msg->object, (char *)msg->data);

	if(!strcmp(msg->service_dest, "PhoneServer")) {
		if(!strcmp(msg->object,"NetworkBars"))
			indicators[THEME_MAINSIGNAL].changed(0);
		else if(!strcmp(msg->object,"BatteryBars"))
			indicators[THEME_MAINBATTERY].changed(0);
	}

	if(!strcmp(msg->service_dest, "AlarmServer")) {
		if(!strcmp(msg->object,"PPM"))
			indicators[THEME_DATETIME].changed(0);
	}

/*DEBUG signal*/
	if(!strcmp(msg->service_dest, "nanowm")) {
		if(!strcmp(msg->object,"debugkey")) {
			int tmp;
			char *str;
			tbus_get_message_args(msg, "is", &tmp, &str);
			DPRINT("%d %s\n", tmp, str);
			free(str);
			shdata->battery.bars = 3;
			indicators[THEME_MAINBATTERY].changed(3);
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
	int ret;
	struct tbus_message msg;

	DBGMSG("indicatord: ipc_handle\n");

	ret = tbus_get_message(&msg);
	if (ret < 0)
		return -1;
	switch(msg.type) {
		case TBUS_MSG_EMIT_SIGNAL:
			/* we received a signal */
			ipc_signal(&msg);
			break;
	}

	tbus_msg_free(&msg);
	return 0;
}
