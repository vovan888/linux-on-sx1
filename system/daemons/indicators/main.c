/*
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

#define DAEMON_NAME "indicatord"

const char lockfile[]="/tmp/"DAEMON_NAME".lock"

/* signal handler */
void signal_handler(int param)
{
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
		printf("Warnning - found a stale lockfile.  Deleting it...\n");
		unlink(path);
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
//	GrReqShmCmds(65536L); /*FIXME we dont need it ? */
	
	ipc_start("indicatord");
	
	nxLoadConfig();
	
	/* pass errors through main loop, don't exit */
	GrSetErrorHandler(NULL);
	
	interface_create();
	
	mainloop();
	
	unlink(lockfile);
	return 0;
}

static void mainloop(void)
{
		
}

