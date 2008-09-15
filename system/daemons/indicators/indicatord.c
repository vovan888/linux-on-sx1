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

const char lockfile[] = "/tmp/" DAEMON_NAME ".lock";

struct SharedSystem *shdata;	/* shared memory segment */
struct indicator indicators[16];
GR_GC_ID gc;			/* current Graphic Context */

static int indicators_number;
static int terminate = 0;

/* signal handler */
void signal_handler(int param)
{
    terminate = 1;
}

static void mainloop(void)
{
	int i;
	GR_WINDOW_ID wid;
	GR_EVENT event;

	while (!terminate) {

		GrGetNextEvent(&event);
		switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			wid = event.general.wid;
			for (i = 0; i < indicators_number; i++) {
				if (indicators[i].wind_id > 0) {
					indicators[i].callback(wid, &event);
				}
			}
			break;
		case GR_EVENT_TYPE_FDINPUT:
			ipc_handle(&event);
			break;
		}
	}
}

int main_load_indicators()
{
	memset(indicators, 0, sizeof(indicators));

	/* Select events for the ROOT window */
	GrSelectEvents(GR_ROOT_WINDOW_ID,
		       GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_FDINPUT);

// debug - draw a cross
	GrLine(GR_ROOT_WINDOW_ID, gc, 0, 0, 176, 220);
	GrLine(GR_ROOT_WINDOW_ID, gc, 0, 220, 176, 0);

	/* setup THEME_MAINBATTERY */
	mainbattery_create(&indicators[THEME_MAINBATTERY]);

	/* setup THEME_MAINSIGNAL */
	mainsignal_create(&indicators[THEME_MAINSIGNAL]);

	/* setup THEME_DATETIME */
	maindatetime_create(&indicators[THEME_DATETIME]);

	indicators_number = THEME_DATETIME;

	return 0;
}

int main(int argc, char *argv[])
{

	pid_t pid = 0;
	int pathfd_;
	char buf[512];
	int ret;

      signal(SIGINT, signal_handler);
      signal(SIGTERM, signal_handler);
      signal(SIGHUP, signal_handler);

	memset(buf, 0, sizeof(buf));
	pid = getpid();

	if (!access(lockfile, F_OK)) {
		printf("Warning - found a stale lockfile.  Deleting it...\n");
		unlink(lockfile);
	}

	pathfd_ = open(lockfile, O_RDWR | O_TRUNC | O_CREAT, 0644);
	if (pathfd_ == -1) {
		perror("open(): " DAEMON_NAME);
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
		perror("Couldn't connect to Nano-X server!\n");
		unlink(lockfile);
		exit(-1);
	}
	GrReqShmCmds(4096);
	gc = GrNewGC();

	main_load_indicators();

	ipc_start("indicatord");

	shdata = ShmMap(SHARED_SYSTEM);

	mainloop();

	GrClose();
	ShmUnmap(shdata);
	unlink(lockfile);

	return 0;
}
