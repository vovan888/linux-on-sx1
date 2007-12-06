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

struct SharedData {
	/* powerup data */
	struct {
		int reason;
		int selftest;
		int swreason;
		int 
	} powerup;
	/* nanowm data */
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
}

/* Map shared memory segment
 * returns its adress 
*/
struct SharedData * ShmMap();

/* UnMap shared memory segment
 * returns its adress 
*/
int ShmUnMap(struct SharedData * ptr);

#endif //shareddata_h_
