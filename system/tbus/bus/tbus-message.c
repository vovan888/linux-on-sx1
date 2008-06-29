/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"

/**
 * Free all the parts of the message
 * @param msg pointer to the message
 */
static void tbus_msg_free(struct tbus_message *msg)
{
	free(msg->service_sender);
	free(msg->service_dest);
	free(msg->object);
	free(msg->data);
}

/**
 * Write message to the client
 * @param fd file descriptor of destination
 * @param msg message struct
 * @param args message arguments
 */
static int tbus_write_message(int fd, struct tbus_message *msg)
{
	int err;
	tpl_node *tn;

	tn = tpl_map(TBUS_MESSAGE_FORMAT, &msg->magic, &msg->type,
		      &msg->service_sender, &msg->service_dest, &msg->object, &msg->data);
	tpl_pack(tn,0);  /* copies message data into the tpl */
	err = tpl_dump(tn, TPL_FD, fd);	/* write the tpl image to file descriptor */
	tpl_free(tn);

	if(err < 0) {
		/* error while writing means we lost connection to server */
		tbus_socket_sys = 0;
		/*FIXME*/
	}

	return err;
}

/**
 * Read client message, msg buffer is allocated automatically
 * @param fd file descriptor to read from
 * @param msg pointer to the struct tbus_message
 *
 * Buffers for the data in the structure are allocated automatically!
 */
static int tbus_read_message(int fd, struct tbus_message *msg)
{
	tpl_node *tn;

	tn = tpl_map(TBUS_MESSAGE_FORMAT, &msg->magic, &msg->type,
		      &msg->service_sender, &msg->service_dest, &msg->object, &msg->data);
	if (tpl_load(tn, TPL_FD, fd))
		goto error_msg;
	tpl_unpack(tn,0);   /* allocates space and unpacks data */

	tpl_free(tn);

	if (msg->magic != TBUS_MAGIC) {
		/* it is not TBUS message */
		/*FIXME what to do here ? */
		goto error_tpl;
	}

	return 0;
error_tpl:
	tbus_msg_free(msg);
error_msg:
	free(msg);
	tpl_free(tn);
	return -1;
}

/**
 * Handle message from client
 * @param socket_fd client socket fd
 * @return 0 - OK, -1 - error or socket closed
 */
int tbus_client_message(int socket_fd, int bus_id)
{
	int ret;
	struct tbus_message msg;
	char *args;
	struct tbus_client *dest_client, *sender_client;

	ret = tbus_read_message(socket_fd, &msg);
	if (ret < 0)
		return -1;

	/* try to find the target and source services in clients list */
	sender_client = tbus_client_find_by_service(msg->service_sender);

	/* we have a message, decode it */
	if (sender_client == NULL) {
		/* client is not registered, so only REGISTER allowed */
		if (msg->type == TBUS_MSG_REGISTER) {
			ret =
			    tbus_client_add(socket_fd, bus_id,
					    msg->service_sender);
			msg->type = TBUS_MSG_REGISTERED;
			msg->length = 0;
			ret |= tbus_write_message(socket_fd, msg, args);
		} else {
			/* error - not connected */
			msg->type = TBUS_MSG_ERROR;
			msg->length = 0;
			ret = tbus_write_message(socket_fd, msg, args);
		}
	} else {		
		switch (msg->type) {
		case TBUS_MSG_REGISTER:
			/* error - already connected */
			msg->type = TBUS_MSG_ERROR;
			msg->length = 0;
			ret = tbus_write_message(socket_fd, msg, args);
			break;
		case TBUS_MSG_CALL_METHOD:
			dest_client = tbus_client_find_by_service(msg->service_dest);
			ret =
			    tbus_client_method(sender_client, dest_client, msg,
					       args);
			break;
		case TBUS_MSG_RETURN_METHOD:
			dest_client = tbus_client_find_by_service(msg->service_dest);
			ret =
			    tbus_client_method_return(sender_client,
						      dest_client, msg, args);
			break;
		case TBUS_MSG_CONNECT_SIGNAL:
			ret = tbus_client_connect_signal(sender_client, msg);
			break;
		case TBUS_MSG_DISCON_SIGNAL:
			ret = tbus_client_disconnect_signal(sender_client, msg);
			break;
		case TBUS_MSG_EMIT_SIGNAL:
			ret = tbus_client_emit_signal(sender_client, msg, args);
			break;
		default:
			/*goto error;*/
			break;
		};
	};
	/* check ret */

	 /**/
	free(msg);
	free(args);

	return 0;
}
