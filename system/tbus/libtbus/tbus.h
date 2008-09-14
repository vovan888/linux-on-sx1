/*
 * T-BUS global definitions
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

#ifndef _TBUS_H_
#define _TBUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

/* buses */
#define TBUS_SYSTEM		0
//#define TBUS_APP              1

/* sockets */
#define TBUS_SOCKET_SYS		"/tmp/.tbus-sys.socket"
//#define TBUS_SOCKET_APP               "/tmp/.tbus-app.socket"

/* internal message types */
#define TBUS_MSG_REGISTER		0
#define TBUS_MSG_REGISTERED		1
#define TBUS_MSG_CALL_METHOD		2
#define TBUS_MSG_RETURN_METHOD		3
#define TBUS_MSG_CONNECT_SIGNAL		4
#define TBUS_MSG_DISCON_SIGNAL		5
#define TBUS_MSG_EMIT_SIGNAL		6
#define TBUS_MSG_ERROR			7
#define TBUS_MSG_CLOSE			8

/* internal message struct */
struct tbus_message {
	int type;	/* type of message */
	char *service_sender;	/* sender */
	char *service_dest;	/* destination */
	char *object;	/* method name to call or emitted signal name */
	void *data;	/* message data0 (binary buffer) */
	int datalen;	/* length of data buffer */
//	char *data1;	/* message data1 (string) */
//	char *data2;	/* message data2 (string) */
};

/* TPL format string for the internal message */
#define TBUS_MESSAGE_FORMAT	"S(isss)B"

DLLEXPORT int tbus_register_service (char *service);

DLLEXPORT int tbus_close (void);

DLLEXPORT int tbus_wait_message (int millisec);

DLLEXPORT int tbus_get_message (struct tbus_message *msg);

DLLEXPORT int tbus_get_message_args (struct tbus_message *msg, char *fmt, ...);

DLLEXPORT int tbus_call_method (char *service, char *method, char *fmt, ...);

DLLEXPORT int tbus_call_method_and_wait (struct tbus_message *answer, char *service, char *method, char *fmt, ...);

DLLEXPORT int tbus_method_return(char *service, char *method, char *fmt, ...);

DLLEXPORT int tbus_connect_signal (char *service, char *object);

DLLEXPORT int tbus_disconnect_signal (char *service, char *object);

DLLEXPORT int tbus_emit_signal (char *object, char *fmt, ...);

DLLEXPORT void tbus_msg_free (struct tbus_message *msg);

#ifdef __cplusplus
}
#endif
#endif
