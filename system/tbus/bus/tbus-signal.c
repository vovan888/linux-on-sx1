/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include <stddef.h>

#include "tbus-server.h"

struct tbus_signal_conn *signal_connections = NULL;
const int keylen = offsetof(struct tbus_signal_conn, object) + TBUS_MAX_NAME + 1;

/**
 * Find the connection to signal in the list
 * @param service_dest signal service name
 * @param object signal name
 * @return pointer to the struct tbus_signal_conn, or NULL if not found
 */
static struct tbus_signal_conn *tbus_find_connection (char * service_dest, char * object)
{
	struct tbus_signal_conn connection, *conn_ptr;

	memset (&connection, 0, sizeof(struct tbus_signal_conn));
	strncpy(connection.service_dest, service_dest, TBUS_MAX_NAME);
	strncpy(connection.object, object, TBUS_MAX_NAME);

	HASH_FIND(hh, signal_connections, &connection, keylen, conn_ptr);

	return conn_ptr;
}

/**
 * Connect client to the signal
 * @param sender_client client, that whats to connect to signal
 * @param msg	message struct
 * @return 0 - OK, -1 - error
 */
int tbus_client_connect_signal(struct tbus_client *sender_client, struct tbus_message *msg)
{
	struct tbus_signal_conn *connection;
	struct subscription *sub;

	connection = tbus_find_connection(msg->service_dest, msg->object);

	if (!connection) {
		/* add new connection struct to the hashed list */
		connection = calloc(1, sizeof(struct tbus_signal_conn));
		if(connection)
			return -1;

		strncpy(connection->service_dest, msg->service_dest, TBUS_MAX_NAME);
		strncpy(connection->object, msg->object, TBUS_MAX_NAME);
		
		/* add client data to the list */
		sub = malloc(sizeof(struct subscription));
		if(!sub) {
			free(connection);
			return -1;
		}
		sub->client = sender_client;
		INIT_LLIST_HEAD(&connection->clients.list);
		connection->num_of_clients = 1;
		llist_add_tail(&sub->list, &connection->clients.list);

		HASH_ADD(hh, signal_connections, service_dest, keylen, connection);
	} else {
		/* there is an connection in the list
		 * add new client to this connection */
		sub = malloc(sizeof(struct subscription));
		if(!sub)
			return -1;
		sub->client = sender_client;
		llist_add_tail(&sub->list, &connection->clients.list);
	}

	return 0;
}

/**
 * Disconnect client from the signal
 * @param sender_client client, that whats to disconnect from signal
 * @param msg	message struct
 * @return 0 - OK, -1 - error
 */
int tbus_client_disconnect_signal(struct tbus_client *sender_client, struct tbus_message *msg)
{
	struct tbus_signal_conn *connection;
	struct subscription *cur, *cur2;

	connection = tbus_find_connection(msg->service_dest, msg->object);

	if(connection) {
		/* find the exact client */
		llist_for_each_entry_safe(cur, cur2, &connection->clients.list, list) {
			if (cur->client == sender_client ) {
				/* delete from the list of subscriptions */
				llist_del(&cur->list);
				free(&cur->list);
			}
		}

		if(llist_empty(&connection->clients.list)) {
			HASH_DEL(signal_connections, connection);
			free(connection);
		}
		return 0;	/* OK */
	} else
		return -1;	/* connection not found */
}

/**
 * Emit the signal
 * @param sender_client sender (emitter) of the signal
 * @param msg	message struct
 * @param args args
 * @return 0 - OK, -1 - error
 */
int tbus_client_emit_signal(struct tbus_client *sender_client, struct tbus_message *msg, char *args)
{
	struct tbus_signal_conn *connection;
	struct subscription *cur, *cur2;
	int ret;

	connection = tbus_find_connection(msg->service_dest, msg->object);

	if(connection) {
		/* find the exact client */
		llist_for_each_entry_safe(cur, cur2, &connection->clients.list, list) {
			ret = tbus_write_message(cur->client->socket_fd, msg, args);
		}

		if(llist_empty(&connection->clients.list)) {
			HASH_DEL(signal_connections, connection);
			free(connection);
		}
		return 0;	/* OK */
	} else
		return -1;	/* no connected clients */
}
