/* phoneserver.h
*
*  Phoneserver IPC messages
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#ifndef _phoneserver_h_
#define _phoneserver_h_

/* message group ID */
#define CL_MSG_GROUP_PHONE      0xFF09

/* messages ID */
/* ----------------------------------------------------------------------------------*/
/* how many signal strength bars to display to the user. Is sent when value is changed. 
   Current value is in SharedData segment */
#define MSG_PHONE_NETWORK_BARS			0x01

/* ----------------------------------------------------------------------------------*/
/* Notification about battery status changes */
#define MSG_PHONE_BATTERY_STATUS		0x02
/* Status values */
#define BATTERY_STATUS_UNKNOWN		0x00
#define BATTERY_STATUS_POWERED		0x01
#define BATTERY_STATUS_EXTERNAL		0x02
#define BATTERY_STATUS_CHARGING		0x03
#define BATTERY_STATUS_LOW		0x04
/* ----------------------------------------------------------------------------------*/
/* Battery bars (capacity) changes */
#define MSG_PHONE_BATTERY_BARS			0x03


#endif
