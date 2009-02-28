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

/** internal message types */
enum	{
	TBUS_MSG_REGISTER,
	TBUS_MSG_REGISTERED,
	TBUS_MSG_CALL_METHOD,
	TBUS_MSG_RETURN_METHOD,
	TBUS_MSG_CONNECT_SIGNAL,
	TBUS_MSG_DISCON_SIGNAL,
	TBUS_MSG_SIGNAL,
	TBUS_MSG_ERROR,
	TBUS_MSG_CLOSE,
};

/** internal message struct */
	struct tbus_message {
		int type;	/* type of message */
		char *service_sender;	/* sender */
		char *service_dest;	/* destination */
		char *object;	/* method name to call or emitted signal name */
		void *data;	/* message data0 (binary buffer) */
		int datalen;	/* length of data buffer */
	};

/** TPL format string for the internal message */
#define TBUS_MESSAGE_FORMAT	"S(isss)B"

	DLLEXPORT int tbus_register_service(const char *service);

	DLLEXPORT int tbus_close(void);

	DLLEXPORT int tbus_wait_message(int millisec);

	DLLEXPORT int tbus_get_message(struct tbus_message *msg);

	DLLEXPORT int tbus_get_message_args(struct tbus_message *msg, const char *fmt, ...);

	DLLEXPORT int tbus_call_method(const char *service, const char *method, const char *fmt, ...);

	DLLEXPORT int tbus_call_method_and_wait(struct tbus_message *answer, const char *service,
			const char *method, const char *fmt, ...);

	DLLEXPORT int tbus_method_return(const char *service, const char *method, const char *fmt, ...);

	DLLEXPORT int tbus_connect_signal(const char *service, const char *object);

	DLLEXPORT int tbus_disconnect_signal(const char *service, const char *object);

	DLLEXPORT int tbus_emit_signal(const char *object, const char *fmt, ...);

	DLLEXPORT void tbus_msg_free(struct tbus_message *msg);

#ifdef __cplusplus
}
#endif
#endif
