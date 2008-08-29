/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include <stddef.h>

#include "tbus-server.h"
#include <flphone/debug.h>

struct tbus_signal_conn *signal_connections = NULL;

/**
 * Make the name+signal string
 * @param service service name
 * @param signal signal name
 * @param length ouput - length of the string
 * @return pointer to the string
 */
static char *service_signal_str (char *service, char *signal, int *length)
{
	int len1, len2;
	char *str;

	len1 = strlen (service);
	len2 = strlen (signal);
	str = malloc (len1 + len2 + 1);
	strcpy (str, service);
	strcat (str, signal);
	*length = len1 + len2;

	return str;
}

/**
 * Find the connection to signal in the list
 * @param str signal service name + signal name to find
 * @param length length of the str
 * @return pointer to the struct tbus_signal_conn, or NULL if not found
 */
static struct tbus_signal_conn *tbus_find_connection (char *str, int length)
{
	struct tbus_signal_conn *conn_ptr;

	HASH_FIND (hh, signal_connections, str, length, conn_ptr);

	return conn_ptr;
}

/**
 * Connect client to the signal
 * @param sender_client client, that whats to connect to signal
 * @param msg	message struct
 * @return 0 - OK, -1 - error
 */
int tbus_client_connect_signal (struct tbus_client *sender_client,
				struct tbus_message *msg)
{
	struct tbus_signal_conn *connection;
	struct subscription *sub;
	char *str;
	int len;

	str = service_signal_str (msg->service_dest, msg->object, &len);

	connection = tbus_find_connection (str, len);

	DPRINT ("%s\n", str);

	sub = malloc (sizeof (struct subscription));
	if (!sub)
		goto connect_error;

	/* add client data to the list */
	sub->client = sender_client;
	sub->next = NULL;

	if (!connection) {
		/* add new connection struct to the hashed list */
		connection = malloc (sizeof (struct tbus_signal_conn));
		if (!connection)
			goto connect_error;

		connection->service_signal = str;
		connection->clients_head = sub;

		HASH_ADD_KEYPTR (hh, signal_connections, str, len, connection);
	} else {
		/* there is an connection in the list
		 * add new client to this connection */
		if (connection->clients_head != NULL)
			sub->next = connection->clients_head;
		connection->clients_head = sub;
	}

	return 0;

      connect_error:
	free (str);
	free (sub);
	free (connection);
	return -1;
}

/**
 * Disconnect client from the signal
 * @param sender_client client, that whats to disconnect from signal
 * @param msg	message struct
 * @return 0 - OK, -1 - error
 */
int tbus_client_disconnect_signal (struct tbus_client *sender_client,
				   struct tbus_message *msg)
{
	struct tbus_signal_conn *connection;
	struct subscription *cur, *prev;
	char *str;
	int len;

	str = service_signal_str (msg->service_dest, msg->object, &len);

	connection = tbus_find_connection (str, len);

	DPRINT ("%s\n", str);

	if (connection) {
		/* find the exact client */
		cur = connection->clients_head;
		prev = NULL;
		while (cur != NULL) {
			if (cur->client == sender_client) {
				/* delete from the list of subscriptions */
				if (cur == connection->clients_head)
					/* shift the header */
					connection->clients_head = cur->next;
				else
					prev->next = cur->next;

				free (cur);
				break;
			}
			prev = cur;
			cur = cur->next;
		}

		if (connection->clients_head == NULL) {
			HASH_DEL (signal_connections, connection);
			free (connection);
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
int tbus_client_emit_signal (struct tbus_client *sender_client,
			     struct tbus_message *msg)
{
	struct tbus_signal_conn *connection;
	struct subscription *cur, *cur2;
	int ret;
	char *str, *tmp;
	int len;

	str = service_signal_str (sender_client->service, msg->object, &len);

	connection = tbus_find_connection (str, len);

	DPRINT ("%s, %d\n", str, connection);

	if (connection && (connection->clients_head != NULL)) {
		tmp = msg->service_dest;
		msg->service_dest = sender_client->service;
		DPRINT ("%s, %s\n", tmp,
			connection->clients_head->client->service);

		cur = connection->clients_head;
		while (cur != NULL) {
			int sock = cur->client->socket_fd;
			if (sock > 0) {
				ret = tbus_write_message (sock, msg);
				DPRINT ("sent to %s ,ret = %d\n",
					cur->client->service, ret);
			} else {
				DPRINT ("empty client!\n");
			}
			cur = cur->next;
		}
		msg->service_dest = tmp;
		return 0;	/* OK */
	} else
		return -1;	/* no connected clients */
}
