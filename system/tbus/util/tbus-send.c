/*
 * "T-BUS Send" - test utility
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include <stdio.h>
#include <string.h>
#include <ipc/tbus.h>

void print_usage(void)
{
	printf("\ntbus-send T-BUS test utility\n");
	printf("Usage: tbus-send [-w] Service Method fmt_str [arg1]...[argN]\n");
	printf("Example: tbus-send [-w] PhoneServer PIN/Input ss arg1 arg2\n\n");
}

int main (int argc, char **argv)
{
	int wait_for_answer = 0, arg = 1, ret = -255;

	if(argc < 2) {
		print_usage();
		return 0;
	}
	if(!strcmp(argv[1],"-w")) {
		wait_for_answer = 1;
		arg = 2;
	}

	struct tbus_message msg;
	
	ret = tbus_register_service("tbus-send");
	printf("connect to T-BUS : %d\n", ret);

	/* switch depending on number of arguments */
	switch(argc - arg - 3) {
	case -1:	/* no fmt_str */
	case 0:
		ret = tbus_call_method(argv[arg], argv[arg+1], "");
		break;
	case 1:
		ret = tbus_call_method(argv[arg], argv[arg+1], argv[arg+2],
			argv[arg+3]);
		break;
	case 2:
		ret = tbus_call_method(argv[arg], argv[arg+1], argv[arg+2], 
			argv[arg+3], argv[arg+4]);
		break;
	case 3:
		ret = tbus_call_method(argv[arg], argv[arg+1], argv[arg+2], 
			argv[arg+3], argv[arg+4], argv[arg+5]);
		break;
	default:
		ret = -255;
		break;
	}
	printf("%s/%s(%s) = %d\n", argv[arg], argv[arg+1], argv[arg+2], ret);

	if(wait_for_answer) {
		ret = tbus_get_message(&msg);
		char *fmt_str = "NONE";
		if(msg.datalen > 0) {
			fmt_str = tpl_peek(TPL_MEM, msg.data, msg.datalen);
		}
		printf("=>%s/%s(%s) = %d\n", msg.service_sender, msg.object,
			fmt_str, ret);
	}

	ret = tbus_close();

	return 0;
}
