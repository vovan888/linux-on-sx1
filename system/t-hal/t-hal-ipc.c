/*
 * T-HAL ipc functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include <stdlib.h>

#include "t-hal-mach.h"
#include <ipc/tbus.h>
#include <flphone/debug.h>

void handle_signal(struct tbus_message *msg)
{
	DPRINT("%d,%s->%s/%s\n", msg->type, msg->service_sender,
	       msg->service_dest, msg->object);

	if (!strcmp(msg->service_dest, "PhoneServer")) {
		if (!strcmp(msg->object, "NetworkBars"))
			;
	}
}

void handle_method(struct tbus_message *msg)
{
	DPRINT("%d,%s->%s/%s\n", msg->type, msg->service_sender,
	       msg->service_dest, msg->object);

	if (!strcmp(msg->object, "DisplaySetBrightness")) {
		int bright = 5;
		tbus_get_message_args(msg, "i", &bright);
		mach_set_display_brightness(bright);
	} else if (!strcmp(msg->object, "")) {
		int bright = 1;
		tbus_get_message_args(msg, "i", &bright);
//		mach_set_display_dim_brightness(bright);
	}
}
