/* gsm.c
*
*  GSMD related utilites
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <libgsmd/libgsmd.h>
#include <libgsmd/voicecall.h>
#include <libgsmd/misc.h>
#include <libgsmd/phonebook.h>
#include <libgsmd/sms.h>
#include <libgsmd/event.h>
#include <libgsmd/pin.h>
#include <gsmd/usock.h>
#include <gsmd/ts0705.h>

#include "gsm.h"

int pending_responses = 0;

/* this is the handler for receiving passthrough responses */
static int pt_msghandler(struct lgsm_handle *lh, struct gsmd_msg_hdr *gmh)
{
	char *payload = (char *)gmh + sizeof(*gmh);
	printf("RSTR=`%s'\n", payload);
	return 0;
}


/* this is the handler for responses to network/operator commands */
static int net_msghandler(struct lgsm_handle *lh, struct gsmd_msg_hdr *gmh)
{
	const struct gsmd_signal_quality *sq = (struct gsmd_signal_quality *)
		((void *) gmh + sizeof(*gmh));
	const char *oper = (char *) gmh + sizeof(*gmh);
	const struct gsmd_msg_oper *opers = (struct gsmd_msg_oper *)
		((void *) gmh + sizeof(*gmh));
	const struct gsmd_own_number *num = (struct gsmd_own_number *)
		((void *) gmh + sizeof(*gmh));
	const struct gsmd_voicemail *vmail = (struct gsmd_voicemail *)
		((void *) gmh + sizeof(*gmh));
	enum gsmd_netreg_state state = *(enum gsmd_netreg_state *) gmh->data;
	int result = *(int *) gmh->data;
	static const char *oper_stat[] = {
		[GSMD_OPER_UNKNOWN] = "of unknown status",
		[GSMD_OPER_AVAILABLE] = "available",
		[GSMD_OPER_CURRENT] = "our current operator",
		[GSMD_OPER_FORBIDDEN] = "forbidden",
	};
	static const char *srvname[] = {
		[GSMD_SERVICE_ASYNC_MODEM] = "asynchronous modem",
		[GSMD_SERVICE_SYNC_MODEM] = "synchronous modem",
		[GSMD_SERVICE_PAD_ACCESS] = "PAD Access (asynchronous)",
		[GSMD_SERVICE_PACKET_ACCESS] = "Packet Access (synchronous)",
		[GSMD_SERVICE_VOICE] = "voice",
		[GSMD_SERVICE_FAX] = "fax",
	};

	switch (gmh->msg_subtype) {
	case GSMD_NETWORK_SIGQ_GET:
		if (sq->rssi == 99)
			printf("Signal undetectable\n");
		else
			printf("Signal quality %i dBm\n", -113 + sq->rssi * 2);
		if (sq->ber == 99)
			printf("Error rate undetectable\n");
		else
			printf("Bit error rate %i\n", sq->ber);
		pending_responses --;
		break;
	case GSMD_NETWORK_OPER_GET:
	case GSMD_NETWORK_OPER_N_GET:
		if (oper[0])
			printf("Our current operator is %s\n", oper);
		else
			printf("No current operator\n");
		pending_responses --;
		break;
	case GSMD_NETWORK_OPER_LIST:
		for (; !opers->is_last; opers ++)
			printf("%8.*s   %16.*s,   %.*s for short, is %s\n",
					sizeof(opers->opname_num),
					opers->opname_num,
					sizeof(opers->opname_longalpha),
					opers->opname_longalpha,
					sizeof(opers->opname_shortalpha),
					opers->opname_shortalpha,
					oper_stat[opers->stat]);
		pending_responses --;
		break;
	case GSMD_NETWORK_GET_NUMBER:
		printf("\t%s\t%10s%s%s%s\n", num->addr.number, num->name,
				(num->service == GSMD_SERVICE_UNKNOWN) ?
				"" : " related to ",
				(num->service == GSMD_SERVICE_UNKNOWN) ?
				"" : srvname[num->service],
				(num->service == GSMD_SERVICE_UNKNOWN) ?
				"" : " services");
		pending_responses --;
		break;
	case GSMD_NETWORK_VMAIL_SET:
		if (result)
			printf("Set voicemail error %i\n", result);
		else
			printf("Set voicemail OK \n");
		pending_responses --;
		break;
	case GSMD_NETWORK_VMAIL_GET:
		if(vmail->addr.number)
			printf ("voicemail number is %s \n",vmail->addr.number);
		pending_responses --;
		break;
	case GSMD_NETWORK_QUERY_REG:
		switch (state) {
			case GSMD_NETREG_UNREG:
				printf("not searching for network \n");
				break;
			case GSMD_NETREG_REG_HOME:
				printf("registered (home network) \n");
				break;
			case GSMD_NETREG_UNREG_BUSY:
				printf("searching for network \n");
				break;
			case GSMD_NETREG_DENIED:
				printf("registration denied \n");
				break;
			case GSMD_NETREG_REG_ROAMING:
				printf("registered (roaming) \n");
				break;
			default:
				break;
		}
		pending_responses --;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int phone_msghandler(struct lgsm_handle *lh, struct gsmd_msg_hdr *gmh)
{
	char *payload  = (char *)gmh + sizeof(*gmh);
	int *intresult = (void *)gmh + sizeof(*gmh);
	const struct gsmd_battery_charge *bc = (struct gsmd_battery_charge *)
		((void *) gmh + sizeof(*gmh));

	switch (gmh->msg_subtype) {
	case GSMD_PHONE_GET_IMSI:
		printf("imsi <%s>\n", payload);
		break;
	case GSMD_PHONE_GET_MANUF:
		printf("manufacturer: %s\n", payload);
		break;
	case GSMD_PHONE_GET_MODEL:
		printf("model: %s\n", payload);
		break;
	case GSMD_PHONE_GET_REVISION:
		printf("revision: %s\n", payload);
		break;
	case GSMD_PHONE_GET_SERIAL:
		printf("serial: %s\n", payload);
		break;
	case GSMD_PHONE_POWERUP:
		if (*intresult)
			printf("Modem power-up failed: %i\n", *intresult);
		else
			printf("Modem powered-up okay\n");
		break;
	case GSMD_PHONE_POWERDOWN:
		if (*intresult)
			printf("Modem power-down failed: %i\n", *intresult);
		else
			printf("Modem down\n");
		break;
	case GSMD_PHONE_POWER_STATUS:
		printf("Antenna Status: %s\n", payload);
		break;
	case GSMD_PHONE_GET_BATTERY:
		printf("<BCS>: %d <BCL>: %d \n", bc->bcs, bc->bcl);
		break;		
	case GSMD_PHONE_VIB_ENABLE:
		if(*intresult)
			printf("Vibrator enable failed: %i\n", *intresult);
		else
			printf("Vibrator enabled\n");
		break;
	case GSMD_PHONE_VIB_DISABLE:
		if(*intresult)
			printf("Vibrator disable failed: %i\n", *intresult);
		else
			printf("VIbrator disabled\n");
		break;
	default:
		return -EINVAL;
	}
	pending_responses --;
	return 0;
}

static int pin_msghandler(struct lgsm_handle *lh, struct gsmd_msg_hdr *gmh)
{
	int result = *(int *) gmh->data;
	switch (gmh->msg_subtype) {
		case GSMD_PIN_GET_STATUS:
			printf("PIN STATUS: %i\n", result);
			break;
		case GSMD_PIN_INPUT:
			if (result)
				printf("PIN error %i\n", result);
			else
				printf("PIN accepted!\n");
			break;
		default:
			return -EINVAL;	
	}
	pending_responses --;
	return 0;
}

static int call_msghandler(struct lgsm_handle *lh, struct gsmd_msg_hdr *gmh)
{
	struct gsmd_call_status *gcs;
	struct gsmd_call_fwd_stat *gcfs;
	int *ret;

	switch (gmh->msg_subtype) {
	case GSMD_VOICECALL_GET_STAT:
		gcs = (struct  gsmd_call_status*) ((char *)gmh + sizeof(*gmh));
		
		if (gcs->idx > 0)
			printf("%d, %d, %d, %d, %d, %s, %d\n",
					gcs->idx, gcs->dir,
					gcs->stat, gcs->mode,
					gcs->mpty, gcs->number,
					gcs->type);
		else if (gcs->idx < 0)
			/* If index < 0, error happens */
			printf("+CME ERROR %d\n", (0-(gcs->idx)));
		else
			/* No existing call */
			printf("Doesn't exist\n");

		if (gcs->is_last)
			pending_responses --;
		break;
	case GSMD_VOICECALL_CTRL:
		ret = (int*)((char *)gmh + sizeof(*gmh));
		 (*ret)? printf("+CME ERROR %d\n", *ret) : printf("OK\n");
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_DIS:
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_EN:
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_STAT:
		gcfs = (struct  gsmd_call_fwd_stat*) ((char *)gmh + sizeof(*gmh));
		
		printf("+CCFC:%d, %d, %s\n",gcfs->status, gcfs->classx, gcfs->addr.number);

		if (gcfs->is_last)
			pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_REG:
		pending_responses --;
		break;
	case GSMD_VOICECALL_FWD_ERAS:
		pending_responses --;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#define PIN_SIZE 8

static const char *pin;
static char pinbuf[PIN_SIZE+1];
static char pinbuf2[PIN_SIZE+1];

static int pin_handler(struct lgsm_handle *lh,
		int evt, struct gsmd_evt_auxdata *aux)
{
	int rc;
	int type = aux->u.pin.type;
	char *newpin = NULL;

	printf("EVENT: PIN request (type='%s') ", lgsm_pin_name(type));

	/* FIXME: read pin from STDIN and send it back via lgsm_pin */
	if (type == 1 && pin) {
		printf("Auto-responding with pin `%s'\n", pin);
		pending_responses ++;
		return lgsm_pin(lh, type, pin, NULL);
	} else {
		do {
			printf("Please enter %s: ", lgsm_pin_name(type));
			rc = fscanf(stdin, "%8s", pinbuf);
		} while (rc < 1);

		switch (type) {
		case GSMD_PIN_SIM_PUK:
		case GSMD_PIN_SIM_PUK2:
			do {
				printf("Please enter new PIN: ");
				rc = fscanf(stdin, "%8s", pinbuf2);
				newpin = pinbuf2;
			} while (rc < 1);
			break;
		default:
			break;
		}

		pending_responses ++;
		return lgsm_pin(lh, type, pinbuf, newpin);
	}
}

int pin_init(struct lgsm_handle *lh, const char *pin_preset)
{
	pin = pin_preset;
	return lgsm_evt_handler_register(lh, GSMD_EVT_PIN, &pin_handler);
}

static int incall_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	printf("EVENT: Incoming call type = %u\n", aux->u.call.type);

	return 0;
}

static int clip_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	printf("EVENT: Incoming call clip = %s\n", aux->u.clip.addr.number);

	return 0;
}

static int colp_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	printf("EVENT: Outgoing call colp = %s\n", aux->u.colp.addr.number);

	return 0;
}

static int netreg_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	printf("EVENT: Netreg ");

	switch (aux->u.netreg.state) {
	case GSMD_NETREG_UNREG:
		printf("not searching for network ");
		break;
	case GSMD_NETREG_REG_HOME:
		printf("registered (home network) ");
		break;
	case GSMD_NETREG_UNREG_BUSY:
		printf("searching for network ");
		break;
	case GSMD_NETREG_DENIED:
		printf("registration denied ");
		break;
	case GSMD_NETREG_REG_ROAMING:
		printf("registered (roaming) ");
		break;
	default:
		break;
	}

	if (aux->u.netreg.lac)
		printf("LocationAreaCode = 0x%04X ", aux->u.netreg.lac);
	if (aux->u.netreg.ci)
		printf("CellID = 0x%04X ", aux->u.netreg.ci);
	
	printf("\n");

	return 0;
}

static int sigq_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	printf("EVENT: Signal Quality: %u\n", aux->u.signal.sigq.rssi);
	return 0;
}

static int ccwa_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	printf("EVENT: Call Waiting: %s,%d\n", aux->u.ccwa.addr.number, aux->u.ccwa.addr.type);
	return 0;
}

static const char *cprog_names[] = {
	[GSMD_CALLPROG_SETUP]		= "SETUP",
	[GSMD_CALLPROG_DISCONNECT]	= "DISCONNECT",
	[GSMD_CALLPROG_ALERT]		= "ALERT",
	[GSMD_CALLPROG_CALL_PROCEED]	= "PROCEED",
	[GSMD_CALLPROG_SYNC]		= "SYNC",
	[GSMD_CALLPROG_PROGRESS]	= "PROGRESS",
	[GSMD_CALLPROG_CONNECTED]	= "CONNECTED",
	[GSMD_CALLPROG_RELEASE]		= "RELEASE",
	[GSMD_CALLPROG_REJECT]		= "REJECT",
	[GSMD_CALLPROG_UNKNOWN]		= "UNKNOWN",
};

static const char *cdir_names[] = {
	[GSMD_CALL_DIR_MO]		= "Outgoing",
	[GSMD_CALL_DIR_MT]		= "Incoming",
	[GSMD_CALL_DIR_CCBS]		= "CCBS",
	[GSMD_CALL_DIR_MO_REDIAL]	= "Outgoing Redial",
};

static int cprog_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	const char *name, *dir;

	if (aux->u.call_status.prog >= ARRAY_SIZE(cprog_names))
		name = "UNDEFINED";
	else
		name = cprog_names[aux->u.call_status.prog];

	if (aux->u.call_status.dir >= ARRAY_SIZE(cdir_names))
		dir = "";
	else
		dir = cdir_names[aux->u.call_status.dir];

	printf("EVENT: %s Call Progress: %s\n", dir, name);

	return 0;
}

static int error_handler(struct lgsm_handle *lh, int evt, struct gsmd_evt_auxdata *aux)
{
	if(aux->u.cme_err.number)
		printf("cme error: %u\n", aux->u.cme_err.number);
	else if(aux->u.cms_err.number)
		printf("cms error: %u\n", aux->u.cms_err.number);
		
	return 0;
}

int event_init(struct lgsm_handle *lh)
{
	int rc;

	rc  = lgsm_evt_handler_register(lh, GSMD_EVT_IN_CALL, &incall_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_IN_CLIP, &clip_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_OUT_COLP, &colp_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_NETREG, &netreg_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_SIGNAL, &sigq_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_OUT_STATUS, &cprog_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_IN_ERROR, &error_handler);
	rc |= lgsm_evt_handler_register(lh, GSMD_EVT_CALL_WAIT, &ccwa_handler);
	return rc;
}

struct lgsm_handle *lgsmh;

static const struct msghandler_s {
	int type;
	lgsm_msg_handler *fn;
} msghandlers[] = {
	{ GSMD_MSG_PASSTHROUGH,	pt_msghandler },
	{ GSMD_MSG_NETWORK,	net_msghandler },
	{ GSMD_MSG_PHONE,	phone_msghandler },
	{ GSMD_MSG_PIN,		pin_msghandler },
	{ GSMD_MSG_VOICECALL,	call_msghandler },
	{ 0, 0 }
};

void lgsm_handle(int gsm_fd, void * data)
{
	int rc;
	char buf[1025];

	rc = read(gsm_fd, buf, sizeof(buf));
	if (rc <= 0) {
		printf("ERROR reading from gsm_fd\n");
		return;
	}
	rc = lgsm_handle_packet(lgsmh, buf, rc);
	if (rc < 0)
		printf("ERROR processing packet: %d(%s)\n", rc, strerror(-rc));

}

int init_lgsm(void)
{
	lgsmh = lgsm_init(LGSMD_DEVICE_GSMD);

	if (!lgsmh) {
		fprintf(stderr, "Can't connect to gsmd\n");
//		exit(1);
		/*FIXME*/
		return -1;
	}

	int gsm_fd = lgsm_fd(lgsmh);

	pin_init(lgsmh, "0000"); /*FIXME*/
	event_init(lgsmh);

	const struct msghandler_s *hndl;

	for (hndl = msghandlers; hndl->fn; hndl ++)
		lgsm_register_handler(lgsmh, hndl->type, hndl->fn);
	
	/* register to the network */
	lgsm_netreg_register(lgsmh, "\0     ");

	return gsm_fd;
}
