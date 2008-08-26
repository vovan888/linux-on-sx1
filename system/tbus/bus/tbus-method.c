/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"
#include <flphone/debug.h>

int tbus_client_method(struct tbus_client *sender_client, struct tbus_client *dest_client, struct tbus_message *msg)
{
	int sock, ret;
	if (!dest_client) {
		return -1;
	}
	sock = dest_client->socket_fd;
	if (sock > 0) {
		ret = tbus_write_message(sock, msg);
		DPRINT("sent to %s ,ret = %d\n",dest_client->service, ret);
	}else {
		DPRINT("empty client!\n");
	}
	return 0;
}

int tbus_client_method_return(struct tbus_client *sender_client, struct tbus_client *dest_client, struct tbus_message *msg)
{
	return 0;
}
