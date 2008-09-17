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

static int terminate = 0;

static void mainloop(void)
{
	int res;
	struct tbus_message msg;

	while (!terminate) {
		// wait for message on T-BUS
		res = tbus_get_message(&msg);
		if (res < 0)
			return;
		
	}
}

static void t_hal_init(void)
{
	int ret;
	/* IPC init */
	ret = tbus_register_service("AlarmServer");
	if (ret < 0)
		exit(1);

}

static void t_hal_exit(void)
{
	tbus_close();
}

int main(int argc, char *argv[])
{
	t_hal_init();

	mainloop();

	t_hal_exit();

	return 0;
}
