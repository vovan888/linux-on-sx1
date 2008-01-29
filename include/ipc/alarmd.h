/* alarmd.h
*
*  Alarm daemon IPC messages
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#ifndef _alarmd_h_
#define _alarmd_h_

/* message group ID */
#define MSG_GROUP_ALARM      0x7005

/* messages IDs */
/* ----------------------------------------------------------------------------------*/
/* This message is sent every minute */
#define MSG_ALARM_PPM			0x01
/* This message is sent every hour */
#define MSG_ALARM_PPH			0x02

/* ----------------------------------------------------------------------------------*/
/* Union of all the message IDs */
struct msg_alarm {
	unsigned char id;	/* message ID */
	union {
	 /*TODO*/};
};

#endif
