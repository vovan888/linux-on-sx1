/*
 * T-BUS client library
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

#include <ipc/tbus.h>
#include <debug.h>

/* socket fds for two buses */
static int tbus_socket_sys;
//static int tbus_socket_app;

/**
 * Free all the parts of the message
 * @param msg pointer to the message
 */
void tbus_msg_free(struct tbus_message *msg)
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
 * Connect to the server UNIX socket
 * @param socket_path filename for the UNIX socket
 * @return file descriptor of the socket, or -1 for error
 */
static int tbus_connect_socket(char *socket_path)
{
	int sock;
	struct sockaddr_un saddr;
	int tries = 0;

	/* try 5 times to connect to server */
	do {
		sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sock == -1)
			goto init_socket_error;

		saddr.sun_family = AF_UNIX;
		strncpy(saddr.sun_path, socket_path, sizeof(saddr.sun_path));

		/* Try to connect.  If the connection is refused, then we will */
		/* assume that no server is available */
		if (connect(sock, (struct sockaddr *)&saddr, sizeof(saddr))
		    == -1) {
			close(sock);
			sock = 0;
			DPRINT("Waiting for TBUS server\n");
			sleep(1); /* 1 second */
		} else
			break;
		if (tries++ == 5)
			/* no tries left */
			DPRINT("No TBUS server!\n");
			goto init_socket_error;
	} while (1);
	
	return sock;

init_socket_error:

	perror("tbus_init_socket");
	if (sock >=0)
		close(sock);
	return -1;
}

/**
 * Register service to the message bus
 * @param service name of the service to register
 */
DLLEXPORT int tbus_register_service ( char * service )
{
	struct tbus_message msg;

	if ( !service )
		return -1;

	tbus_socket_sys = tbus_connect_socket(TBUS_SOCKET_SYS);
	if(tbus_socket_sys < 0)
		return -1;

	msg.magic = TBUS_MAGIC;
	msg.type  = TBUS_MSG_REGISTER;
	msg.service_sender = service;
	msg.service_dest = "tbus";
	msg.object = "daemon";
	msg.data = "register";
	
	tbus_write_message(tbus_socket_sys, &msg);

	memset ( &msg, 0, sizeof(struct tbus_message) );
	tbus_read_message(tbus_socket_sys, &msg);

	tbus_msg_free(&msg);

	if(msg.type == TBUS_MSG_REGISTERED)
		return tbus_socket_sys;
	else
		return -1;
}

/**
 * Close connection to the TBUS server
 */
DLLEXPORT int tbus_close (void)
{
	int	err;
	struct tbus_message msg;

	if(tbus_socket_sys == -1)
		return -1;

	msg.magic = TBUS_MAGIC;
	msg.type  = TBUS_MSG_CLOSE;
	msg.service_sender = "";
	msg.service_dest = "";
	msg.object = "";
	msg.data = "";

	err = tbus_write_message(tbus_socket_sys, &msg);

	return err;
}

/**
 * Read message from server
 * @param msg pointer to the message structure
 *
 * returns msg->type, or -1 if error
 */
DLLEXPORT int tbus_get_message( struct tbus_message * msg )
{
	int	err;
	
	if(!msg || (tbus_socket_sys == -1))
		return -1;

	err = tbus_read_message(tbus_socket_sys, msg);

	if(err < 0) {
		/* some error happened */
		tbus_msg_free(msg);
		return -1;
	} else {
		return msg->type;
	}
}

/**Call the method on the remote service
 * @param service service name of the method
 * @param method called method
 * @param args arguments to path to the method
 */
DLLEXPORT int tbus_call_method( char * service, char * method, char * args)
{
	int	err;
	struct tbus_message msg;

	if((tbus_socket_sys == -1) || !service || !method || !args)
		return -1;

	msg.magic = TBUS_MAGIC;
	msg.type  = TBUS_MSG_CALL_METHOD;
	msg.service_sender = ""; /*FIXME*/
	msg.service_dest = service;
	msg.object = method;
	msg.data = args;

	err = tbus_write_message(tbus_socket_sys, &msg);

	return err;
}

/*DLLEXPORT void tbus_method_return(  char * service, char * interface, char * object, char * outvalue)
{
}
*/

/**
 * Connect to the remote signal
 * @param service service name of the method
 * @param object signal name to connect
 */
DLLEXPORT int tbus_connect_signal( char * service, char * object)
{
	int	err;
	struct tbus_message msg;

	if((tbus_socket_sys == -1) || !service || !object)
		return -1;

	msg.magic = TBUS_MAGIC;
	msg.type  = TBUS_MSG_CONNECT_SIGNAL;
	msg.service_sender = ""; /*FIXME*/
	msg.service_dest = service;
	msg.object = object;
	msg.data = "connect";

	err = tbus_write_message(tbus_socket_sys, &msg);

	return err;
}

/**
 * Disconnect from the remote signal
 * @param service service name of the method
 * @param object signal name to connect
 */
DLLEXPORT int tbus_disconnect_signal( char * service, char * object)
{
	int	err;
	struct tbus_message msg;

	if((tbus_socket_sys == -1) || !service || !object)
		return -1;

	msg.magic = TBUS_MAGIC;
	msg.type  = TBUS_MSG_DISCON_SIGNAL;
	msg.service_sender = ""; /*FIXME*/
	msg.service_dest = service;
	msg.object = object;
	msg.data = "disconnect";

	err = tbus_write_message(tbus_socket_sys, &msg);

	return err;
}

/**
 * Emit the signal so the connected clients receive notification
 * @param object signal name to connect
 * @param value arguments to path to the method
 */
DLLEXPORT int tbus_emit_signal( char * object, char * value)
{
	int	err;
	struct tbus_message msg;

	if((tbus_socket_sys == -1) || !object || !value)
		return -1;

	msg.magic = TBUS_MAGIC;
	msg.type  = TBUS_MSG_EMIT_SIGNAL;
	msg.service_sender = ""; /*FIXME*/
	msg.service_dest = "";
	msg.object = object;
	msg.data = value;

	err = tbus_write_message(tbus_socket_sys, &msg);

	return err;
}
