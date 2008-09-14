/* shareddata.h
*
*  Shared memory segment functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <flphone/libflphone.h>

#ifndef _shareddata_h_
#define _shareddata_h_

/* shm segment shared_id for different data */
#define SHARED_SYSTEM	0
#define SHARED_APPS	1

#define SHARED_NUMSEGMENTS	2

#define BATTERY_STATUS_UNKNOWN		0x00
#define BATTERY_STATUS_POWERED		0x01
#define BATTERY_STATUS_EXTERNAL		0x02
#define BATTERY_STATUS_CHARGING		0x03
#define BATTERY_STATUS_LOW		0x04

/* shared memory structure for SHARED_SYSTEM*/
struct SharedSystem {
/* powerup data */
	struct {
		char egold_ping_ok;
		char reason;
		char selftest;
		char swreason;
		char temp;
		char hiddenreset;
		char rtccheck;
	} powerup;
/* nanowm data */
	struct {
		char root_displayed;	/* is root window displayed ? */
	} wm;
/* SIM */
	struct {
		int state;
		int lang;
		int changed;
		int owned;
		int pendinglock;
		int lockstatus;
		char domesticlang;
		char simlock;
	} sim;
/* battery */
	struct {
		int status;
		int bars;	/* capacity - 0..7 */
		int capacity;	/* capacity - 0%..100% */
	} battery;
/* PhoneServer status */
	struct {
		int signal;	/* network signal strength in dBm */
		int bars;	/* network bars */
		int missed_calls;
		int messages;
		char oper[16];	/* operator name */
		int profile;

	} PhoneServer;
};

/* Map shared memory segment
 * returns its adress
*/
DLLEXPORT void *ShmMap (unsigned int shared_id);

/* UnMap shared memory segment
 * returns
*/
DLLEXPORT int ShmUnmap (void *ptr);

/* blocks till the shared mem is busy then locks it */
DLLEXPORT int ShmLock (unsigned int shared_id);

/* unlocks shared memory segment */
DLLEXPORT int ShmUnlock (unsigned int shared_id);

#endif //shareddata_h_
