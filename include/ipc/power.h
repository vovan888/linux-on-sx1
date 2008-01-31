/* power.h
*
*  Power management daemon IPC messages
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#ifndef _powerd_h_
#define _powerd_h_

/* message group ID */
#define CL_MSG_GROUP_POWER	0x7000
/* messages IDs */
/* ----------------------------------------------------------------------------------*/
/* This message is sent every minute */
#define MSG_ALARM_PPM			0x01
/* This message is sent every hour */
#define MSG_ALARM_PPH			0x02

/* ----------------------------------------------------------------------------------*/
/* Union of all the message IDs */
struct msg_alarm {
	unsigned short	group; /* message group */
	unsigned char id;	/* message ID */

	union {
	 /*TODO*/};
};

#endif
