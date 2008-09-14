#ifndef _GSMD_EVENT_H
#define _GSMD_EVENT_H

/* Signals */
enum gsmd_events {
//	GSMD_EVT_NONE		= 0,
	GSMD_EVT_IN_CALL	= 1,	/* IncomingCall	- Incoming call */
	GSMD_EVT_IN_SMS		= 2,	/* IncomingSMS	- Incoming SMS */
	GSMD_EVT_IN_GPRS	= 3,	/* IncomingGPRS	- Network initiated GPRS */
	GSMD_EVT_IN_CLIP	= 4,	/* IncomingCLIP	- Incoming CLIP */
	GSMD_EVT_NETREG		= 5,	/* NetworkReg	- Network (un)registration event */
	GSMD_EVT_SIGNAL		= 6,	/* Signal	- Signal quality event */
	GSMD_EVT_PIN		= 7,	/* PINneeded	- Modem is waiting for some PIN/PUK */
	GSMD_EVT_OUT_STATUS	= 8,	/* OutgoingCall	- Outgoing call status */
	GSMD_EVT_OUT_COLP	= 9,	/* OutgoingCOLP	- Outgoing COLP */
	GSMD_EVT_CALL_WAIT	= 10,	/* CallWaiting	- Call Waiting */
	GSMD_EVT_TIMEZONE	= 11,	/* TimezoneChange - Timezone change */
//	GSMD_EVT_SUBSCRIPTIONS	= 12,	/* To which events are we subscribed to */
	GSMD_EVT_CIPHER		= 13,	/* CipheringInfo - Ciphering Information */
	GSMD_EVT_IN_CBM		= 14,	/* IncomingCBM	- Incoming Cell Broadcast message */
	GSMD_EVT_IN_DS		= 15,	/* SMSDeliveryStatus - SMS Status Report */
	GSMD_EVT_IN_ERROR	= 16,	/* CME_CMS_Error - CME/CMS error */
	__NUM_GSMD_EVT
};

/* Chapter 8.3 */
enum gsmd_pin_type {			/* waiting for ... */
	GSMD_PIN_READY		= 0,	/* not pending for any password */
	GSMD_PIN_SIM_PIN	= 1,	/* SIM PIN */
	GSMD_PIN_SIM_PUK	= 2,	/* SIM PUK */
	GSMD_PIN_PH_SIM_PIN	= 3,	/* phone-to-SIM passowrd */
	GSMD_PIN_PH_FSIM_PIN	= 4,	/* phone-to-very-first SIM password */
	GSMD_PIN_PH_FSIM_PUK	= 5,	/* phone-to-very-first SIM PUK password */
	GSMD_PIN_SIM_PIN2	= 6,	/* SIM PIN2 */
	GSMD_PIN_SIM_PUK2	= 7,	/* SIM PUK2 */
	GSMD_PIN_PH_NET_PIN	= 8,	/* netwokr personalisation password */
	GSMD_PIN_PH_NET_PUK	= 9,	/* network personalisation PUK */
	GSMD_PIN_PH_NETSUB_PIN	= 10, 	/* network subset personalisation PIN */
	GSMD_PIN_PH_NETSUB_PUK	= 11,	/* network subset personalisation PUK */
	GSMD_PIN_PH_SP_PIN	= 12,	/* service provider personalisation PIN */
	GSMD_PIN_PH_SP_PUK	= 13,	/* service provider personalisation PUK */
	GSMD_PIN_PH_CORP_PIN	= 14,	/* corporate personalisation PIN */
	GSMD_PIN_PH_CORP_PUK	= 15,	/* corporate personalisation PUK */
	__NUM_GSMD_PIN
};

enum gsmd_call_type {
	GSMD_CALL_NONE		= 0,
	GSMD_CALL_UNSPEC	= 1,
	GSMD_CALL_VOICE		= 2,
	GSMD_CALL_FAX		= 4,
	GSMD_CALL_DATA_SYNC	= 5,
	GSMD_CALL_DATA_REL_ASYNC= 6,
	GSMD_CALL_DATA_REL_SYNC	= 7,
	GSMD_CALL_TIMEOUT       = 8,
	__NUM_GSMD_CALL
};

/* Chapter 7.2 */
enum gsmd_netreg_state {
	GSMD_NETREG_UNREG	= 0,
	GSMD_NETREG_REG_HOME,
	GSMD_NETREG_UNREG_BUSY,
	GSMD_NETREG_DENIED,
	GSMD_NETREG_UNKNOWN,
	GSMD_NETREG_REG_ROAMING,
	__NUM_GSMD_NETREG
};
/*
enum gsmd_call_progress {
	GSMD_CALLPROG_SETUP		= 0,
	GSMD_CALLPROG_DISCONNECT	= 1,
	GSMD_CALLPROG_ALERT		= 2,
	GSMD_CALLPROG_CALL_PROCEED	= 3,
	GSMD_CALLPROG_SYNC		= 4,
	GSMD_CALLPROG_PROGRESS		= 5,
	GSMD_CALLPROG_CONNECTED		= 6,
	GSMD_CALLPROG_RELEASE		= 7,
	GSMD_CALLPROG_REJECT		= 8,
	GSMD_CALLPROG_UNKNOWN		= 9,
	__NUM_GSMD_CALLPROG
};
*/
enum gsmd_call_progress {
	GSMD_CALL_IDLE		= 0,
	GSMD_CALL_INCOMING	= 1,
	GSMD_CALL_DIALING	= 2,
	GSMD_CALL_RINGING	= 3,
	GSMD_CALL_CONNECTED	= 4,
	GSMD_CALL_HOLD		= 5,
	GSMD_CALL_HANGUP	= 6,
	GSMD_CALL_FAILURE	= 7,
	__NUM_GSMD_CALL_STATUS
};

enum gsmd_call_direction {
	GSMD_CALL_DIR_MO		= 0,	/* Mobile Originated (Outgoing) */
	GSMD_CALL_DIR_MT		= 1,	/* Mobile Terminated (Incoming) */
	GSMD_CALL_DIR_CCBS		= 2,	/* network initiated MO */
	GSMD_CALL_DIR_MO_REDIAL		= 3,	/* Mobile Originated Redial */
};

#endif
