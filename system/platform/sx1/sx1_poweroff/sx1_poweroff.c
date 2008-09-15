/*
 * power off SX1
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/tbus.h>
#include <common/tpl.h>

void print_usage(void)
{
	printf("\nsx1_poweroff\n");
	printf("Usage: sx1_poweroff [reason] | [-h]\n");
	printf("if [reason] is set - then does reboot and sets reboot reason\n");
	printf("-h - print this help screen\n");
}

int main (int argc, char **argv)
{
	int ret, reason = 100;

	if(argc == 2) {
		if(!strcmp(argv[1],"-h")) {
			print_usage();
			return 0;
		} else {
		// reboot with some reason
		reason = atoi(argv[1]);
		ret = tbus_register_service("sx1_poweroff");
		ret |= tbus_call_method("sx1_ext", "Reboot", "i", &reason);
		}
	} else {
		// power off
		ret = tbus_register_service("sx1_poweroff");
		ret |= tbus_call_method("sx1_ext", "PowerOff", "");
	}

	// should never be here
	ret = tbus_close();

	return 0;
}
