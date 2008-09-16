/*
 * T-BUS server definitions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include <common/uthash.h>
#include <common/tpl.h>

#include <ipc/tbus.h>

/* clients list struct */
struct tbus_client {
	int socket_fd;		/* fd for server connection */
//      int bus_id;             /* ID of the connected bus (SYS or APP) */
	char *service;		/* service name (hash key) */

	UT_hash_handle hh1, hh2;	/* makes this structure hashable by socket_fd and *service */
};

/* clients list for the connection struct */
struct subscription {
	struct tbus_client *client;

	struct subscription *next;
};

/* signal connection struct */
struct tbus_signal_conn {
	char *service_signal;	/* emitter service name + signal name */

	struct subscription *clients_head;	/* connected clients list head */
//      struct subscription *clients_tail;      /* connected clients list tail */

	UT_hash_handle hh;	/* makes this structure hashable by service_dest+object */
};

/* from tbus-clients.c */
void tbus_init_clients();
int tbus_client_add(int socket_fd, char *service);
struct tbus_client *tbus_client_find_by_socket(int socket_fd);
struct tbus_client *tbus_client_find_by_service(char *service);
void tbus_client_del(struct tbus_client *client);

/* from tbus-message.c */
int tbus_client_message(int socket_fd, int bus_id);
int tbus_write_message(int fd, struct tbus_message *msg);

/* from tbus-signal.c */
int tbus_client_connect_signal(struct tbus_client *sender_client, struct tbus_message *msg);
int tbus_client_disconnect_signal(struct tbus_client *sender_client, struct tbus_message *msg);
int tbus_client_emit_signal(struct tbus_client *sender_client, struct tbus_message *msg);
int tbus_remove_client_connections(struct tbus_client *client);

/* from tbus-method.c */
int tbus_client_method(struct tbus_client *sender_client,
		       struct tbus_client *dest_client, struct tbus_message *msg);
int tbus_client_method_return(struct tbus_client *sender_client,
			      struct tbus_client *dest_client, struct tbus_message *msg);
