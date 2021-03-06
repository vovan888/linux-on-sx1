/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"
#include <flphone/debug.h>

int tbus_client_method(struct tbus_client *sender_client,
		       struct tbus_client *dest_client,
		       struct tbus_message *msg)
{
	int sock, ret = 0;

	sock = dest_client->socket_fd;
	if (sock > 0) {
		char *tmp;
		tmp = msg->service_sender;
		msg->service_sender = sender_client->service;

		ret = tbus_write_message(sock, msg);

		msg->service_sender = tmp;
		DPRINT("sent from %s to %s/%s ,ret = %d\n",
		       sender_client->service, dest_client->service,
		       msg->object, ret);
	} else {
		DPRINT("empty client!\n");
	}
	return ret;
}

int tbus_client_method_return(struct tbus_client *sender_client,
			      struct tbus_client *dest_client,
			      struct tbus_message *msg)
{
	int sock, ret = 0;

	sock = dest_client->socket_fd;
	if (sock > 0) {
		char *tmp;
		tmp = msg->service_sender;
		msg->service_sender = sender_client->service;

		ret = tbus_write_message(sock, msg);

		msg->service_sender = tmp;
		DPRINT("sent from %s to %s/%s ,ret = %d\n",
		       sender_client->service, dest_client->service,
		       msg->object, ret);
	} else {
		DPRINT("empty client!\n");
	}
	return ret;
}
