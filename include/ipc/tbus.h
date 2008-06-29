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

#include <common/tpl.h>

/* buses */
#define TBUS_SYSTEM		0
//#define TBUS_APP		1

/* sockets */
#define TBUS_SOCKET_SYS		"/tmp/.tbus-sys.socket"
//#define TBUS_SOCKET_APP		"/tmp/.tbus-app.socket"

/* internal message types */
#define TBUS_MSG_REGISTER		0
#define TBUS_MSG_REGISTERED		1
#define TBUS_MSG_CALL_METHOD		2
//#define TBUS_MSG_RETURN_METHOD		3
#define TBUS_MSG_CONNECT_SIGNAL		4
#define TBUS_MSG_DISCON_SIGNAL		5
#define TBUS_MSG_EMIT_SIGNAL		6
#define TBUS_MSG_ERROR			7

//#define TBUS_MAX_NAME		23
//#define TBUS_MAX_MESSAGE	4000
#define TBUS_MAGIC		0x08880888

/* internal message struct */
struct tbus_message {
	int magic;	/* magic constant */
	int type;	/* type of message */
	char *service_sender;	/* sender */
	char *service_dest;	/* destination */
	char *object;		/* method name to call or emitted signal name */
	char *data;		/* message data (string) */
};

/* TPL format string for the internal message */
#define TBUS_MESSAGE_FORMAT	"iissss"

struct TBusConnection {
	int    socket_fd; /* fd for server connection */
	int    bus_id;    /* ID of the connected bus (SYS or APP) */
};

/* -fvisibility=hidden support macro */
#ifdef CONFIG_GCC_HIDDEN_VISIBILITY
    #define DLLEXPORT __attribute__ ((visibility("default")))
    #define DLLLOCAL __attribute__ ((visibility("hidden")))
#else
    #define DLLEXPORT
    #define DLLLOCAL
#endif

DLLEXPORT int tbus_register_service ( struct TBusConnection * bus, char * service );

DLLEXPORT int tbus_get_message( struct TBusConnection * bus, struct tbus_message * msg );

DLLEXPORT int tbus_call_method(struct TBusConnection * bus, char * service, char * method, char * args);

//DLLEXPORT void tbus_method_return(struct TBusConnection * bus, char * service, char * object, char * outvalue);

DLLEXPORT int tbus_connect_signal(struct TBusConnection * bus, char * service, char * object);

DLLEXPORT int tbus_disconnect_signal(struct TBusConnection * bus, char * service, char * object);

DLLEXPORT int tbus_emit_signal(struct TBusConnection * bus, char * object, char * value);

#ifdef __cplusplus
}
#endif

#endif
