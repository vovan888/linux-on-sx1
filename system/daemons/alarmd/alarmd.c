/* alarmd.c
*
*  main module utils
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
* portions of the code are from Linux kernel/Documentation/rtc.txt
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
#include <linux/rtc.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "alarmd_internal.h"

#define DAEMON_NAME "alarmserver"

const char lockfile[] = "/tmp/" DAEMON_NAME ".lock";

/* RTC interface */
static const char default_rtc[] = "/dev/rtc";
static int rtc_fd;		/* RTC file descriptor */
static int irq_count = 0;	/* irq (1 second) counter */
static int minutes_sync = 0;	/* sync to minutes change ? */
static int minutes_count = 0;	/* sync to minutes change ? */

/* IPC interface */
struct SharedSystem *shdata;	/* shared memory segment */
static int ipc_fd;		/* IPC file descriptor */

/* signal handler */
void signal_handler(int param)
{
}

static void handle_rtc(int fd)
{
	int retval;
	unsigned long data;
	struct rtc_time rtc_tm;
	struct msg_alarm msg;

	/* This read won't block */
	retval = read(fd, &data, sizeof(unsigned long));
	if (retval == -1) {
		perror("read");
		exit(errno);
	}

	/* here we have one second passed, so increase irq count */
	irq_count++;

	/* sync with the minutes changes */
	if (!minutes_sync) {
		/* Read the RTC time/date */
		retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
		if (retval == -1) {
			perror("ioctl");
			exit(errno);
		}

		/* Check current time: minutes */	
		if (rtc_tm.tm_min == 0) {
			irq_count = 0;
			minutes_sync = 1;
		}
	}


	/*TODO send PPS (pulse per second message)
	  maybe it is too often ?*/

	/*Send PPM (pulse per minute message)
	   this message should be sent on the first second of minute 
	   (syncronized with time) */
	if ((irq_count % 60) == 0) {
		DBGMSG("ALARMD: minute message sent\n");
		tbus_emit_signal(&bus, "PPM","1");

		minutes_count++;
	}

	/*Send PPH (pulse per hour message)
	   this message should be sent on the first second of hour
	   (syncronized with time) */
	if (((minutes_count % 60) == 0) && minutes_sync) {
		DBGMSG("ALARMD: hour message sent\n");
		tbus_emit_signal(&bus, "PPH","1");
	}
}

static void mainloop(void)
{
	int retval;
	struct timeval tv;

	fd_set readfds, activefds;

	FD_ZERO(&activefds);
	FD_SET(rtc_fd, &activefds);
	FD_SET(ipc_fd, &activefds);

	while (1) {
		readfds = activefds;
		/* Initialize the timeout data structure. */
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		/* The select will wait until an RTC interrupt or IPC message happens. */
		retval = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
		if (retval == -1) {
			perror("select");
			exit(errno);
		}
		/*TODO handle timeout in select */

		if (FD_ISSET(rtc_fd, &readfds)) {
			handle_rtc(rtc_fd);
		}
		if (FD_ISSET(ipc_fd, &readfds)) {
			ipc_handle(ipc_fd);
		}
	}
}

static void alarmd_init(void)
{
	int retval;

	/* open RTC device */
	rtc_fd = open(default_rtc, O_RDONLY);

	if (rtc_fd == -1) {
		perror(default_rtc);
		exit(errno);
	}

	/* Turn on update interrupts (one per second) */
	retval = ioctl(rtc_fd, RTC_UIE_ON, 0);
	if (retval == -1) {
		if (errno == ENOTTY) {
			fprintf(stderr, "\n...Update IRQs not supported.\n");
		}
		perror("RTC_UIE_ON ioctl");
		exit(errno);
	}

	/* IPC init */
	ipc_fd = ipc_start("AlarmServer");
	/*TODO handle error from ipc_start */

	/*TODO*/
	/* Subscribe to different signals */
	tbus_connect_signal(&bus, "nanowm", "debugkey");

	shdata = ShmMap(SHARED_SYSTEM);
}

static void alarmd_check_lockfile(void)
{
	pid_t pid = 0;
	int pathfd_;
	char buf[512];
	int ret;

	//      signal(SIGINT, signal_handler);
	//      signal(SIGTERM, signal_handler);
	//      signal(SIGHUP, signal_handler);

	memset(buf, 0, sizeof(buf));
	pid = getpid();

	if (!access(lockfile, F_OK)) {
		printf("Warning - found a stale lockfile.  Deleting it...\n");
		unlink(lockfile);
	}

	pathfd_ = open(lockfile, O_RDWR | O_TRUNC | O_CREAT);
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

}

static void alarmd_exit(void)
{
	int retval;

	/* Turn off update interrupts */
	retval = ioctl(rtc_fd, RTC_UIE_OFF, 0);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	/* IPC stuff */
	ShmUnmap(shdata);
	unlink(lockfile);
	close(rtc_fd);

}

int main(int argc, char *argv[])
{

	alarmd_check_lockfile();

	alarmd_init();

	mainloop();

	alarmd_exit();

	return 0;
}
