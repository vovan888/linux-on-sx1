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

/* buses */
#define TBUS_SYSTEM		0
#define TBUS_APP		1

/* sockets */
#define TBUS_SOCKET_SYS		"/tmp/.tbus-sys.socket"
#define TBUS_SOCKET_APP		"/tmp/.tbus-app.socket"

/* internal message types */
#define TBUS_MSG_REGISTER		0
#define TBUS_MSG_REGISTERED		1
#define TBUS_MSG_CALL_METHOD		2
#define TBUS_MSG_RETURN_METHOD		3
#define TBUS_MSG_CONNECT_SIGNAL	4
#define TBUS_MSG_DISCON_SIGNAL		5
#define TBUS_MSG_EMIT_SIGNAL		6
#define TBUS_MSG_ERROR			7

#define TBUS_MAX_NAME		23
#define TBUS_MAX_MESSAGE	4000
#define TBUS_MAGIC		0x08880888

/* internal message struct */
struct tbus_message {
	int magic;	/* magic constant */
	int length;	/* length of the message data (can be zero) */
	int type;	/* type of message */
	int serial;	/* serial number for the method call and return */
	char service_sender[TBUS_MAX_NAME+1];	/* sender */
	char service_dest[TBUS_MAX_NAME+1];	/* destination */
/*	char interface[TBUS_MAX_NAME+1];*/	/* interface */
	char object[TBUS_MAX_NAME+1];		/* method name to call or emitted signal name */
};

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

DLLEXPORT void tbus_register_service ( struct TBusConnection * bus, char * service, int bus_id );

/* returns int - type of the message (Method, Method return, error, signal)
   sourceservice - name of the source Service 
   interface - name of the called interface ( the group of methods or signals )
   object - name of the method or signal
   value - arguments for the method and return value for method and signal */
DLLEXPORT int tbus_get_message( struct TBusConnection * bus, struct tbus_message * message, char * args );

DLLEXPORT int tbus_call_method(struct TBusConnection * bus, char * service, char * interface, char * method, char * args);

DLLEXPORT void tbus_method_return(struct TBusConnection * bus, char * service, char * interface, char * object, char * outvalue);

DLLEXPORT int tbus_connect_signal(struct TBusConnection * bus, char * service, char * interface, char * object);

DLLEXPORT int tbus_disconnect_signal(struct TBusConnection * bus, char * service, char * interface, char * object);

DLLEXPORT void tbus_emit_signal(struct TBusConnection * bus, char * interface, char * object, char * value);

#ifdef __cplusplus
}
#endif

#endif
