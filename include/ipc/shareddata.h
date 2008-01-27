/* shareddata.h
*
*  Shared memory segment functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <libflphone.h>

#ifndef _shareddata_h_
#define _shareddata_h_

/* shm segment shared_id for different data */
#define SHARED_SYSTEM	0
#define SHARED_APPS	1

#define SHARED_NUMSEGMENTS	2

/* shared memory structure for SHARED_SYSTEM*/
struct SharedSystem {
/* powerup data */
	struct {
		int reason;
		int selftest;
		int swreason;
		int temp;
	} powerup;
/* nanowm data */
	struct {
		int root_displayed; /* is root window displayed ? */
	} wm;
/* network */
	struct {
		char signal; 	/* network signal strength in dBm*/
		char bars; 	/* network bars */
	} network;
/* SIM */
	struct {
		int state;
		int lang;
		int changed;
		int owned;
		int pendinglock;
		int lockstatus;
		int domesticlang;
		int simlock;
	} sim;
/* battery */
	struct {
		int status;
		int bars;	/* capacity - 0..7 */
		int capacity;	/* capacity - 0%..100% */
	} battery;
/* phone status */
	struct {
		int missed_calls;
		int messages;
		char oper[16]; /* operator name */
		int profile;
		
	} status;
};

/* Map shared memory segment
 * returns its adress
*/
DLLEXPORT struct SharedSystem *ShmMap(unsigned int shared_id);

/* UnMap shared memory segment
 * returns 
*/
DLLEXPORT int ShmUnmap(struct SharedSystem *ptr);

/* blocks till the shared mem is busy then locks it */
DLLEXPORT int ShmLock (unsigned int shared_id);

/* unlocks shared memory segment */
DLLEXPORT int ShmUnlock (unsigned int shared_id);


#endif //shareddata_h_
