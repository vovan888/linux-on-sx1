/*
 * T-HAL main functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include <stdlib.h>

#include "t-hal-mach.h"
#include <ipc/tbus.h>
#include <flphone/debug.h>

static int terminate = 0;
struct SharedSystem *shdata;	/* shared memory segment */

static void t_hal_mainloop(void)
{
	int type;
	struct tbus_message msg;

	while (!terminate) {
		// wait for message on T-BUS
		type = tbus_get_message(&msg);
		if (type < 0)
			return;

		switch (type) {
			case TBUS_MSG_SIGNAL:
				handle_signal(&msg);
			break;
			case TBUS_MSG_CALL_METHOD:
				handle_method(&msg);
			break;
		}

		tbus_msg_free(&msg);
	}
}

static void t_hal_init(void)
{
	int ret;
	/* IPC init */
	ret = tbus_register_service("T-HAL");
	if (ret < 0)
		exit(1);

	shdata = ShmMap(SHARED_SYSTEM);
	if (shdata == NULL)
		exit(1);

	mach_connect_signals();
}

static void t_hal_exit(void)
{
	tbus_close();
}

int main(int argc, char *argv[])
{
	t_hal_init();

	t_hal_mainloop();

	t_hal_exit();

	return 0;
}
