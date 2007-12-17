/* shareddata.h
*
*  Shared memory segment functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#ifndef _shareddata_h_
#define _shareddata_h_

/* shared memory structure */

typedef struct {
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
		char bars; /* network signal */
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
		int chargingstate;
		int status;
		int bars;
		int lowwarn;
	} battery;
/* phone status */
	struct {
		int missed_calls;
		int messages;
		char oper[16]; /* operator name */
		int profile;
		
	} status;
} SharedData;

/* Map shared memory segment
 * returns its adress
*/
SharedData * ShmMap();

/* UnMap shared memory segment
 * returns 
*/
int ShmUnmap(SharedData * ptr);

/* blocks till the shared mem is busy then locks it */
int ShmLock ();

/* unlocks shared memory segment */
int ShmUnlock ();


#endif //shareddata_h_
