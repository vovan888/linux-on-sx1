/* main.c
*
*  main module utils
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#include <flphone_config.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <nano-X.h>
#include "indicators.h"

#define DAEMON_NAME "indicatord"

const char lockfile[]="/tmp/"DAEMON_NAME".lock";

struct indicator indicators[16];

/* signal handler */
void signal_handler(int param)
{
}

static void mainloop(void)
{
	int i;
	GR_WINDOW_ID wid;
	GR_EVENT event;

	while (1) {

		GrGetNextEvent(&event);
		switch (event.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				wid = event.general.wid;
				for (i = 0; i < 16; i++) {
					if (indicators[i].wind_id == wid) {
						indicators[i].callback(wid, &event);
					}
				}
				break;
			case GR_EVENT_TYPE_FD_ACTIVITY:
				if (event.fd.can_read)
					ipc_handle(&event);
				break;
		}
	}
}



int main(int argc, char *argv[])
{
	
	pid_t pid = 0;
	int pathfd_;
	char buf[512];
	int ret;
	
//	signal(SIGINT, signal_handler);
//	signal(SIGTERM, signal_handler);
//	signal(SIGHUP, signal_handler);
	
	memset(buf, 0, sizeof(buf));
	pid = getpid();
	
	if (!access(lockfile, F_OK)) {
		printf("Warnning - found a stale lockfile.  Deleting it...\n");
		unlink(lockfile);
	}
	
	pathfd_ = open(lockfile, O_RDWR | O_TRUNC | O_CREAT);
	if (pathfd_ == -1) {
		perror("open(): "DAEMON_NAME);
		exit(errno);
	}
	sprintf(buf, "%d", pid);
	ret = write(pathfd_, buf, strlen(buf));
	if (-1 == ret) {
		perror("write(): pid");
		exit(errno);
	}
	close(pathfd_);
	
	/* This is the sigchild handler, useful for when our children die */
	
	//    signal(SIGCHLD, application_handler);
	
	if (GrOpen() < 0) {
		error("Couldn't connect to Nano-X server!\n");
		unlink(lockfile);
		exit(-1);
	}
	GrReqShmCmds(4096L); /*FIXME we dont need it ? */
	
//	ipc_start("indicatord");
	
	/* pass errors through main loop, don't exit */
//	GrSetErrorHandler(NULL);
	init_mainbattery(&indicators[0]);
	
	mainloop();
	
//	GrClose();

	unlink(lockfile);
	return 0;
}

