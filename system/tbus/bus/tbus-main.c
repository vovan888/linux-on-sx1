/*
 * T-BUS main functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"

/* socket fds for two buses */
static int tbus_socket_sys;
static int tbus_socket_app;

static int terminate;

/**
 * Bind to the UNIX socket
 * @param socket_path filename for the UNIX socket
 * @return file descriptor of the socket, or -1 for error
 */
static int tbus_init_socket(char *socket_path)
{
	int sock;
	struct sockaddr_un saddr;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		goto init_socket_error;

	saddr.sun_family = AF_UNIX;
	strncpy(saddr.sun_path, socket_path, sizeof(saddr.sun_path));

	if (bind(sock, (struct sockaddr *)&saddr, (socklen_t)SUN_LEN(&saddr)) < 0)
		goto init_socket_error;

	if (listen(sock, 10) == -1)
		goto init_socket_error;

	return sock;

init_socket_error:

	perror("tbus_init_socket");
	if (sock >=0)
		close(sock);
	return -1;
}

/**
 * UNIX signals handler
 * @param signr signal number
 */
static void signal_handler(int signr)
{
	switch (signr) {
	case SIGTERM:
		terminate = 1;
		break;
	case SIGINT:
		exit(EXIT_SUCCESS);
	case SIGUSR1:
	case SIGALRM:
		/*gsmd_timer_check_n_run(); */
		break;
	}
}

/**
 * Init TBUS server
 */
static int tbus_init()
{
#ifndef DEBUG
	int ret;
	/* become daemon */
	ret = daemon(0, 0);
	if (ret < 0)
		return -1;
#endif
	terminate = 0;

	unlink(TBUS_SOCKET_SYS);
	unlink(TBUS_SOCKET_APP);

	tbus_socket_sys = tbus_init_socket(TBUS_SOCKET_SYS);
	tbus_socket_app = tbus_init_socket(TBUS_SOCKET_APP);

	if ((tbus_socket_sys < 0) || (tbus_socket_app < 0))
		return -1;

	/* setup signals handlers */
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGUSR1, signal_handler);
	signal(SIGALRM, signal_handler);
	
	return 0;
}

/**
 * Main sockets processing loop
 */
static void tbus_mainloop()
{
	int maxfd, ret, i, newfd;
	struct sockaddr_un saddr;
	socklen_t sadrlen = (socklen_t)sizeof(saddr);

	/* max value for socket */
	maxfd =
	    (tbus_socket_app >
	     tbus_socket_sys) ? tbus_socket_app : tbus_socket_sys;
	fd_set active_fd_set;	/* stores all the sockets to listen */
	fd_set read_fd_set;	/* used in "select" call */

	/* Initialize the file descriptor set. */
	FD_ZERO(&active_fd_set);
	FD_SET(tbus_socket_sys, &active_fd_set);
	FD_SET(tbus_socket_app, &active_fd_set);

	while (!terminate) {
		/* process sockets */
		read_fd_set = active_fd_set;
		/* we never timeout here */
		ret = select(maxfd + 1, &read_fd_set, NULL, NULL, NULL);

		if (ret == -1) {
			/* An EAGAIN or EINTR forces to continue */
			if (errno == EAGAIN || errno == EINTR)
				continue;
			perror("tbus_mainloop: select");
			return;
		}

		/* seek the socket with data */
		for (i = 0; i <= maxfd; i++) {
			if (!FD_ISSET(i, &read_fd_set))
				continue;
			/* we have ready-to-read socket */
			if ((i == tbus_socket_sys) || (i == tbus_socket_app)) {
				/*handle client connection to the bus */
				newfd = accept(i, (struct sockaddr *)&saddr, &sadrlen);
				if (newfd == -1) {
					perror("tbus_mainloop: accept");
					/*FIXME exit() or continue ? */
					continue;
				}
				/* add to master set */
				FD_SET(newfd, &active_fd_set);
				if (newfd > maxfd)
					maxfd = newfd;
				/*FIXME we will add a connection to the list
				   with "register" message */
				   /*handle_connection_sys(newfd); */
			} else {
				/* handle message from client */
				ret = tbus_client_message(i,1);
				if (ret == -1) {
					/* error or client closed socket */
					/*FIXME add debug message */
					FD_CLR(i, &active_fd_set);
					close(i);
				}
			}
		}
	}
}

/**
 * Deallocate all resources
 */
static void tbus_exit()
{
}

/**
 * main - tbus server entry point
 */
int main(int argc, char **argv)
{
	int ret;
	
	ret = tbus_init();
	if(ret < 0)
		exit(EXIT_FAILURE);

	tbus_mainloop();

	tbus_exit();

	return 0;
}
