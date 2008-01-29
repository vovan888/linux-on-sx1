/* main.c
*
*  main module utils
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
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
#include <linux/rtc.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "alarmdef.h"

#define DAEMON_NAME "alarmd"

const char lockfile[]="/tmp/"DAEMON_NAME".lock";

/* RTC interface */
static const char default_rtc[] = "/dev/rtc";
static int rtc_fd;		/* RTC file descriptor */
static int irq_count = 0;	/* irq (1 second) counter */

/* IPC interface */
struct SharedSystem *shdata; /* shared memory segment */
static int ipc_fd;		/* IPC file descriptor */

/* signal handler */
void signal_handler(int param)
{
}

static void handle_rtc(int fd)
{
	int	retval;
	unsigned long data;

	/* This read won't block */
	retval = read(fd, &data, sizeof(unsigned long));
	if (retval == -1) {
	        perror("read");
	        exit(errno);
	}
	
	/* here we have one second passed, so increase irq count */
	irq_count++;
	printf("sec=%d",irq_count);
	
	/*TODO send PPS (pulse per second message)*/
	
	/*TODO send PPM (pulse per minute message)
	this message should be sent on the first second of minute 
	(syncronized with time)*/

	/*TODO send PPH (pulse per hour message)
	this message should be sent on the first second of hour
	(syncronized with time)*/
}

static void mainloop(void)
{
	int retval;
	struct timeval tv = {5, 0};     /* 5 second timeout on select */
	fd_set readfds, activefds;

	FD_ZERO(&activefds);
	FD_SET(rtc_fd, &activefds);
	FD_SET(ipc_fd, &activefds);
	
	while(1) {
		readfds = activefds;
		/* The select will wait until an RTC interrupt or IPC message happens. */
		retval = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
		if (retval == -1) {
		        perror("select");
		        exit(errno);
		}
		if (FD_ISSET (rtc_fd, &readfds)) {
			handle_rtc(rtc_fd);
		}
		if (FD_ISSET (ipc_fd, &readfds)) {
			ipc_handle(ipc_fd);
		}
	}
}

static void alarmd_init(void)
{
	int retval;

	/* open RTC device */
	rtc_fd = open(default_rtc, O_RDONLY);
	
	if (rtc_fd ==  -1) {
		perror(default_rtc);
		exit(errno);
        }
	
	/* Turn on update interrupts (one per second) */
	retval = ioctl(rtc_fd, RTC_UIE_ON, 0);
	if (retval == -1) {
		if (errno == ENOTTY) {
			fprintf(stderr,
				"\n...Update IRQs not supported.\n");
		}
		perror("RTC_UIE_ON ioctl");
		exit(errno);
	}

	/* IPC init */
	ipc_fd = ipc_start("alarmd");

	shdata = ShmMap(SHARED_SYSTEM);
}

static void alarmd_check_lockfile(void)
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
		printf("Warning - found a stale lockfile.  Deleting it...\n");
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

}

int main(int argc, char *argv[])
{
	
	alarmd_check_lockfile();
	
	alarmd_init();

	mainloop();

	ShmUnmap(shdata);
	unlink(lockfile);

	return 0;
}