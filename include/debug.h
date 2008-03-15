/* debug.h
*
* debugging helper macros
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/
#ifndef DEBUG__H
#define DEBUG__H


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#define EPRINT(fmt, arg...) do {								\
		printf("[%d] ERROR: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf("errno = %d\n", errno);							\
		printf(fmt, ##arg); } while(0)

#define WPRINT(fmt, arg...) do {										\
		printf("[%d] WARNING: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf(fmt, ##arg); } while(0)

#define DPRINT(fmt, arg...) do{											\
		printf("[%d] %s-%s:%d, " fmt, getpid(), __BASE_FILE__, __FUNCTION__, __LINE__, ##arg); \
		} while(0)

#define FATAL(fmt, arg...) do {											\
		printf("[%d] %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf("errno = %d\n", errno);								\
		printf(fmt "FATAL", ##arg); exit(0);} while(0)


#ifdef DEBUG
#define INITSYSLOG(name) openlog(name, LOG_NDELAY | LOG_PID | LOG_PERROR, LOG_LOCAL0)
#define DBGMSG(fmt, arg...)  DPRINT(fmt, ##arg)
#define INFOLOG(fmt, arg...) do {										\
		syslog(LOG_INFO, "[%d] INFO: %s-%s:%d: ", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_INFO, fmt, ##arg); } while(0)
#define DBGLOG(fmt, arg...)  do { DBGMSG(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] DEBUG: %s-%s: %d, " fmt, getpid(), __BASE_FILE__, __FUNCTION__, __LINE__, ##arg); \
		} while(0)
#else
#define INITSYSLOG(name) openlog(name, LOG_NDELAY | LOG_PID, LOG_LOCAL0)
#define DBGMSG(fmt, arg...)  do {;}while(0)
#define INFOLOG(fmt, arg...) do {;}while(0)
#define DBGLOG(fmt, arg...)  do {;}while(0)
#endif

#define WARNLOG(fmt, arg...) do { WPRINT(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] WARNING: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_WARNING, fmt, ##arg); } while(0)
#define ERRLOG(fmt, arg...)  do { EPRINT(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] ERROR: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_ERR, fmt, ##arg); } while(0)
#define FATALLOG(fmt, arg...)  do { EPRINT(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] FATAL: %s--%s: %d\n", getpid(),__BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_ERR, fmt, ##arg); while(1);} while(0)

#endif /* DEBUG__H */
