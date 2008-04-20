/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"

int tbus_client_method(struct tbus_client *sender_client, struct tbus_client *dest_client, struct tbus_message *msg, char *args)
{
	return 0;
}

int tbus_client_method_return(struct tbus_client *sender_client, struct tbus_client *dest_client, struct tbus_message *msg, char *args)
{
	return 0;
}