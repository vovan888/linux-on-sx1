/*
 * T-BUS client library
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include "tbus.h"
#include <common/tpl.h>
#include <flphone/debug.h>

/* socket fds for two buses */
static int tbus_socket_sys = -1;
//static int tbus_socket_app;

/**
 * Free all the parts of the message
 * @param msg pointer to the message
 */
DLLEXPORT void tbus_msg_free (struct tbus_message *msg)
{
	free (msg->service_sender);
	free (msg->service_dest);
	free (msg->object);
	if(msg->datalen > 0) {
		free (msg->data);
		msg->datalen = 0;
	}
}

/**
 * Write message to the client
 * @param fd file descriptor of destination
 * @param msg message struct
 */
static int tbus_write_message (int fd, struct tbus_message *msg)
{
	int err;
	tpl_node *tn;
	tpl_bin tb;

	tn = tpl_map (TBUS_MESSAGE_FORMAT, msg, &tb);
	tb.sz = msg->datalen;
	tb.addr = msg->data;
	tpl_pack (tn, 0);	/* copies message data into the tpl */
	err = tpl_dump (tn, TPL_FD, fd);	/* write the tpl image to file descriptor */
	tpl_free (tn);

	/* free data - it is always malloc`ed */
	if(msg->datalen > 0) {
		free (msg->data);
		msg->datalen = 0;
	}

	if (err < 0) {
		/* error while writing means we lost connection to server */
		tbus_socket_sys = -1;
	 /*FIXME*/}

	return err;
}

/**
 * Read client message, msg buffer is allocated automatically
 * @param fd file descriptor to read from
 * @param msg pointer to the struct tbus_message
 *
 * Buffers for the data in the structure are allocated automatically!
 */
static int tbus_read_message (int fd, struct tbus_message *msg)
{
	tpl_node *tn;
	tpl_bin tb;

	tn = tpl_map (TBUS_MESSAGE_FORMAT, msg, &tb);
	if (tpl_load (tn, TPL_FD, fd))
		goto error_msg;
	tpl_unpack (tn, 0);	/* allocates space and unpacks data */
	tpl_free (tn);

	msg->datalen = tb.sz;
	msg->data = tb.addr;

	return 0;
      error_msg:
	free (msg);
	tpl_free (tn);
	return -1;
}

/**
 * Pack messages arguments
 * @param msg message structure
 * @param fmt message format, like in libtpl
 * @param ap arguments list (pointers to vars!)
 * @return 0 if OK
 */
static int tbus_pack_args(struct tbus_message *msg, char *fmt, va_list ap)
{
	void *data = NULL;
	int datalen = 0;

	if (fmt && (strlen(fmt) > 0)) {
		tpl_node *tn;
		tn = tpl_vmap(fmt, ap);
		tpl_pack(tn, 0);
		tpl_dump(tn, TPL_MEM, &data, &datalen);
		tpl_free(tn);
	}

	msg->data = data;
	msg->datalen = datalen;

	return 0;
}

/**
 * Connect to the server UNIX socket
 * @param socket_path filename for the UNIX socket
 * @return file descriptor of the socket, or -1 for error
 */
static int tbus_connect_socket (char *socket_path)
{
	int sock;
	struct sockaddr_un saddr;
	int tries = 0;

	/* try 5 times to connect to server */
	do {
		sock = socket (AF_UNIX, SOCK_STREAM, 0);
		if (sock == -1)
			goto init_socket_error;

		saddr.sun_family = AF_UNIX;
		strncpy (saddr.sun_path, socket_path, sizeof (saddr.sun_path));

		/* Try to connect.  If the connection is refused, then we will */
		/* assume that no server is available */
		if (connect (sock, (struct sockaddr *)&saddr, sizeof (saddr))
		    == -1) {
			close (sock);
			sock = 0;
			DPRINT ("Waiting for TBUS server - %d\n", tries);
			sleep (1);	/* 1 second */
		} else
			return sock;

		if (tries++ == 5) {
			/* no tries left */
			DPRINT ("No TBUS server!\n");
			goto init_socket_error;
		}
	} while (1);

      init_socket_error:
	perror ("tbus_init_socket");
	if (sock >= 0)
		close (sock);
	return -1;
}

/**
 * Register service to the message bus
 * @param service name of the service to register
 */
DLLEXPORT int tbus_register_service (char *service)
{
	struct tbus_message msg;

	if (!service)
		return -1;

	tbus_socket_sys = tbus_connect_socket (TBUS_SOCKET_SYS);
	if (tbus_socket_sys < 0)
		return -1;

	msg.type = TBUS_MSG_REGISTER;
	msg.service_sender = service;
	msg.service_dest = "tbus";
	msg.object = "daemon";
	msg.data = NULL;
	msg.datalen = 0;

	tbus_write_message (tbus_socket_sys, &msg);

	memset (&msg, 0, sizeof (struct tbus_message));
	tbus_read_message (tbus_socket_sys, &msg);

	tbus_msg_free (&msg);

	if (msg.type == TBUS_MSG_REGISTERED)
		return tbus_socket_sys;
	else
		return -1;
}

/**
 * Close connection to the TBUS server
 */
DLLEXPORT int tbus_close (void)
{
	int err;
	struct tbus_message msg;

	if (tbus_socket_sys == -1)
		return -1;

	msg.type = TBUS_MSG_CLOSE;
	msg.service_sender = "";
	msg.service_dest = "";
	msg.object = "";
	msg.data = NULL;
	msg.datalen = 0;

	err = tbus_write_message (tbus_socket_sys, &msg);

	return err;
}

/**
 * Read message from server
 * @param msg pointer to the message structure
 *
 * returns msg->type, or -1 if error
 */
DLLEXPORT int tbus_get_message (struct tbus_message *msg)
{
	int err;

	if (!msg || (tbus_socket_sys == -1))
		return -1;

	err = tbus_read_message (tbus_socket_sys, msg);

	if (err < 0) {
		/* some error happened */
		tbus_msg_free (msg);
		return -1;
	} else {
		return msg->type;
	}
}

/**
 * Extract message args from message struct
 * @param msg pointer to the message structure
 *
 * returns msg->type, or -1 if error
 */
DLLEXPORT int tbus_get_message_args (struct tbus_message *msg, char *fmt, ...)
{
	int err;
	va_list ap;

	if (!msg || (tbus_socket_sys == -1))
		return -EINVAL;

	if (fmt && (strlen(fmt) > 0) && (msg->datalen > 0)) {
		va_start(ap, fmt);
		tpl_node *tn;
		tn = tpl_vmap(fmt, ap);
		err = tpl_load(tn, TPL_MEM, msg->data, msg->datalen);
		if(err < 0) return err;
		err = tpl_unpack(tn, 0);
		if(err < 0) return err;
		tpl_free(tn);
	}
	return 0;
}

/**Call the method on the remote service
 * @param service service name of the method
 * @param method called method
 * @param fmt format string for the arguments, as in libtpl
 * @param args method arguments, should be pointers to variables!
 */
DLLEXPORT int tbus_call_method (char *service, char *method, char *fmt, ...)
{
	int err;
	struct tbus_message msg;
	va_list ap;

	if ((tbus_socket_sys < 0) || !service || !method || !fmt)
		return -1;

	va_start(ap, fmt);
	tbus_pack_args(&msg, fmt, ap);
	msg.type = TBUS_MSG_CALL_METHOD;
	msg.service_sender = "";
	msg.service_dest = service;
	msg.object = method;

	err = tbus_write_message (tbus_socket_sys, &msg);

	return err;
}

/**Return from the method with value
 * @param service service name of the method caller
 * @param method called method
 * @param fmt format string for the arguments, as in libtpl
 * @param args arguments to pass to the method
 */
DLLEXPORT int tbus_method_return(char *service, char *method, char *fmt, ...)
{
	int err;
	struct tbus_message msg;
	va_list ap;

	if ((tbus_socket_sys == -1) || !service || !method || !fmt)
		return -1;

	va_start(ap, fmt);
	tbus_pack_args(&msg, fmt, ap);
	msg.type = TBUS_MSG_RETURN_METHOD;
	msg.service_sender = "";
	msg.service_dest = service;
	msg.object = method;

	err = tbus_write_message (tbus_socket_sys, &msg);

	return err;
}

/** Wait for data on socket
  * @param millisec timeout in Milliseconds
  * Returns 0 if timeout, 1 if input available, -1 if error
**/
DLLEXPORT int tbus_wait_message (int millisec)
{
	int seconds;
	fd_set set;
	struct timeval timeout;

	FD_ZERO (&set);
	FD_SET (tbus_socket_sys, &set);
	seconds = (millisec * 1000) / 1000000;
	timeout.tv_sec = seconds;
	 /*Microseconds*/
	timeout.tv_usec =(millisec * 1000 - seconds * 1000000);

	return TEMP_FAILURE_RETRY (select (FD_SETSIZE, &set, NULL, NULL, &timeout));
}

/** Call method and wait for the method return
 * After sending method call ignores all incoming messages!
 * Waits for method_return message and then return
 * @param answer method_return message
 * @param service service name of the method caller
 * @param method called method
 * @param fmt format string for the arguments, as in libtpl
 * @param args arguments to pass to the method
 */
DLLEXPORT int tbus_call_method_and_wait (struct tbus_message *answer, char *service, char *method, char *fmt, ...)
{
	int err, type, counter = 0;
	struct tbus_message msg;
	va_list ap;

	if ((tbus_socket_sys == -1) || !service || !method || !fmt)
		return -EINVAL;

	va_start(ap, fmt);
	tbus_pack_args(&msg, fmt, ap);
	msg.type = TBUS_MSG_CALL_METHOD;
	msg.service_sender = "";
	msg.service_dest = service;
	msg.object = method;

	err = tbus_write_message (tbus_socket_sys, &msg);
	if(err < 0)
		return err;

	do {
		type = tbus_get_message (answer);
		/* check the answer */
		if(type == TBUS_MSG_RETURN_METHOD)
			if( !strcmp(method, answer->object) && !strcmp(service, answer->service_sender) ) {
				return 0;
			}
		tbus_msg_free(answer);
	} while(counter++ < 16);

	return -EPROTO;
}

/**
 * Connect to the remote signal
 * @param service service name of the method
 * @param object signal name to connect
 */
DLLEXPORT int tbus_connect_signal (char *service, char *object)
{
	int err;
	struct tbus_message msg;

	if ((tbus_socket_sys == -1) || !service || !object)
		return -EINVAL;

	msg.type = TBUS_MSG_CONNECT_SIGNAL;
	msg.service_sender = "";
	msg.service_dest = service;
	msg.object = object;
	msg.data = NULL;
	msg.datalen = 0;

	err = tbus_write_message (tbus_socket_sys, &msg);

	return err;
}

/**
 * Disconnect from the remote signal
 * @param service service name of the method
 * @param object signal name to connect
 */
DLLEXPORT int tbus_disconnect_signal (char *service, char *object)
{
	int err;
	struct tbus_message msg;

	if ((tbus_socket_sys == -1) || !service || !object)
		return -1;

	msg.type = TBUS_MSG_DISCON_SIGNAL;
	msg.service_sender = "";
	msg.service_dest = service;
	msg.object = object;
	msg.data = NULL;
	msg.datalen = 0;

	err = tbus_write_message (tbus_socket_sys, &msg);

	return err;
}

/**
 * Emit the signal so the connected clients receive notification
 * @param object signal name to connect
 * @param fmt format string for the arguments, as in libtpl
 * @param value arguments to path to the method
 */
DLLEXPORT int tbus_emit_signal (char *object, char *fmt, ...)
{
	int err;
	struct tbus_message msg;
	va_list ap;

	if ((tbus_socket_sys == -1) || !object || !fmt)
		return -1;

	va_start(ap, fmt);
	tbus_pack_args(&msg, fmt, ap);
	msg.type = TBUS_MSG_EMIT_SIGNAL;
	msg.service_sender = "";
	msg.service_dest = "";
	msg.object = object;

	err = tbus_write_message (tbus_socket_sys, &msg);

	return err;
}
