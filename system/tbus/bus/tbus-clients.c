/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"

struct tbus_client *clients_by_service;

/**
 * Init clients list
 */
void tbus_init_clients()
{
	clients_by_service = NULL;
}

/**
 * Add a client struct to the hashed list
 * @param socket_fd client socket file descriptor
 * @param bus_id id of the connected bus
 * @param service client name
 * @return 0 - all is OK, -1 - error
 */
int tbus_client_add(int socket_fd, int bus_id, char *service)
{
	struct tbus_client *new_client;

	new_client = calloc(1, sizeof(struct tbus_client));
	if (new_client) {
		new_client->socket_fd = socket_fd;
		new_client->bus_id = bus_id;
		strncpy(new_client->service, service, TBUS_MAX_NAME);
		HASH_ADD_STR(clients_by_service, service, new_client);
		return 0;
	} else
		return -1;
}

/**
 * Find the client in the hashed list by service name
 * @param service service name to find
 * @return pointer to the found struct tbus_client
 */
struct tbus_client *tbus_client_find_by_service(char *service)
{
	struct tbus_client *found_client;

	HASH_FIND_STR(clients_by_service, service, found_client);

	return found_client;
}

/**
 * Delete client from client list
 * @param client pointer to the struct tbus_client
 */
void tbus_client_del(struct tbus_client *client)
{
	HASH_DEL(clients_by_service, client);
	free(client);
}
