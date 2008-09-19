/* shareddata.h
*
*  Shared memory segment functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <flphone/libflphone.h>

#include <gsmd/event.h>
#include <gsmd/usock.h>

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
		/* Status:
		0 - MT is powered by battery,
		1 - MT has a battery connected, but is not powered by it
		2 - MT does not have a battery connected
		3 - Recognized power fault, calls inhibited */
		enum gsmd_bat_conn_status Status;
		int ChargeLevel;	/* charge level - 0...5 */
		int Charging;		/* is battery charging ? */
		int Low;		/* 1 when battery is very low */
	} Battery;
/* PhoneServer status */
	struct {
		int Ready;	/* 1 - if GSMD finished init (after PIN enter)*/
		enum gsmd_netreg_state CREG_State;
		int CREG_Lac;
		int CREG_Ci;
		enum gsmd_pin_type PIN_Type;

		char Phone_Manuf[32];		/* manufacturer */
		char Phone_Model[32];		/* model */
		char Phone_Revision[32];	/* revision */
		char Phone_Serial[32];		/* serial number */

		int  Network_Service_Avail;	/* service abailable */
		int  Network_Signal;		/* network signal strength 0..5 */
		char Network_Operator[64+8];	/* current operator alpha in HEX */
		char Network_OperatorNum[8];	/* current operator num in HEX (MCC+MNC)*/

		int Call_InProgress;		/* is Call in progress now? */

		int GPRS_CoverageAvailable;	/* 1 - GPRS coverage available, 0 - no GPRS */
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
