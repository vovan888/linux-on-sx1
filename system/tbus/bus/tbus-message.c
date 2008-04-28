/*
 * T-BUS client list functions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#include "tbus-server.h"

/**
 * Write the buf to the file
 * @param fd file descriptor
 * @param buf source buffer
 * @param len pointer to the variable, containing length of buffer
 * @return 0 - OK, -1 - error
 */
static int tbus_write(int fd, char *buf, int *len)
{
	int total = 0;		/* how many bytes we've sent */
	int bytesleft = *len;	/* how many we have left to send */
	int n = 0;

	while (total < *len) {
		n = write(fd, buf + total, bytesleft);
		if (n == -1) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total;	/* return number actually sent here */

	return n == -1 ? -1 : 0; /* return -1 on failure, 0 on success */
}

/**
 * Read client message, msg and arg buffer are allocated automatically
 * @param fd file descriptor of destination
 * @param msg message struct
 * @param args message arguments
 */
int tbus_write_message(int fd, struct tbus_message *msg, char *args)
{
	int len;
	int ret;

	/* send the message body */
	len = sizeof(struct tbus_message);
	ret = tbus_write(fd, (char *)msg, &len);

	/* send the argument */
	len = msg->length;
	ret = tbus_write(fd, args, &len);

	/*FIXME check all the 'ret' values*/
	return 0;
}

/**
 * Read client message, msg and arg buffer are allocated automatically
 * @param fd file descriptor to read from
 * @param msg_p pointer to the struct tbus_message address
 * @param arg_p pointer to the arg buffer address
 */
static int tbus_read_message(int fd, struct tbus_message **msg_p, char **arg_p)
{
	int len_read;
	struct tbus_message *msg;
	char *arg = NULL;

	/* read the header of the message */
	msg = calloc(1, sizeof(struct tbus_message));
	if (msg == NULL)
		/*out of memory error */
		goto error;

	len_read = read(fd, (void *)msg, sizeof(struct tbus_message));
	if ((len_read <= 0) || (len_read < sizeof(struct tbus_message)))
		/* EOF or client closed socket */
		goto error_msg;

	if (msg->magic != TBUS_MAGIC) {
		/* it is not TBUS message */
		/*FIXME what to do here ? */
		goto error_msg;
	}

	if (msg->length > 0) {
		/* read the rest of message */
		arg = calloc(1, msg->length);
		if (arg == NULL)
			/*out of memory error */
			goto error_msg;
		len_read = read(fd, (void *)arg, msg->length);
		if (len_read < msg->length)
			goto error_arg;
	}

	*msg_p = msg;
	*arg_p = arg;

	return 0;
error_arg:
	free(arg);
error_msg:
	free(msg);
error:
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
	struct tbus_message *msg;
	char *args;
	struct tbus_client *dest_client, *sender_client;

	ret = tbus_read_message(socket_fd, &msg, &args);
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
