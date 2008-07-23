/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"
#include <debug.h>

struct tbus_client *clients_by_service, *clients_by_fd;

/**
 * Add a client struct to the hashed list
 * @param socket_fd client socket file descriptor
 * @param bus_id id of the connected bus
 * @param service client name
 * @return 0 - all is OK, -1 - error
 */
int tbus_client_add(int socket_fd, char *service)
{
	struct tbus_client *new_client;

	new_client = calloc(1, sizeof(struct tbus_client));
	if (new_client) {
		DPRINT("%d, %s\n",socket_fd, service);

		new_client->socket_fd = socket_fd;
//		new_client->bus_id = bus_id;
//		strncpy(new_client->service, service, TBUS_MAX_NAME);
//		HASH_ADD_STR(clients_by_service, service, new_client);
		new_client->service = service;
		HASH_ADD(hh1, clients_by_fd, socket_fd, sizeof(int), new_client);
		HASH_ADD_KEYPTR(hh2, clients_by_service, service, strlen(service), new_client);
		return 0;
	} else
		return -1;
}
/**
 * Find the client in the hashed list by socket id
 * @param socket_fd service socket to find
 * @return pointer to the found struct tbus_client
 */
struct tbus_client *tbus_client_find_by_socket(int socket_fd)
{
	struct tbus_client *found_client;
	int	fd = socket_fd;

	HASH_FIND(hh1, clients_by_fd, &fd, sizeof(int), found_client);

	DPRINT("sock=%d, found_client=%d\n",socket_fd, found_client);

	return found_client;
}

/**
 * Find the client in the hashed list by service name
 * @param service service name to find
 * @return pointer to the found struct tbus_client
 */
struct tbus_client *tbus_client_find_by_service(char *service)
{
	struct tbus_client *found_client;

//	HASH_FIND_STR(clients_by_service, service, found_client);
	HASH_FIND(hh2, clients_by_service, service, strlen(service), found_client);
	DPRINT("serv=%s, found_client=%d\n",service, found_client);

	return found_client;
}

/**
 * Delete client from clients list
 * @param client pointer to the struct tbus_client
 */
void tbus_client_del(struct tbus_client *client)
{
	DPRINT("%s\n",client->service);

	HASH_DELETE(hh1, clients_by_fd, client);
	HASH_DELETE(hh2, clients_by_service, client);
	free(client->service);
	free(client);
}
