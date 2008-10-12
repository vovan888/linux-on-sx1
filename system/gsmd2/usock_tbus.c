/*
 * usock_tbus.c - TBUS interface for the gsmd
 * (c) 2008 by Vladimir Ananiev <vovan888 at gmail com>
 * based on usock.c
 */

/* gsmd unix domain socket handling
 *
 * (C) 2006-2007 by OpenMoko, Inc.
 * Written by Harald Welte <laforge@openmoko.org>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "gsmd.h"

#include <gsmd/gsmd.h>
#include <gsmd/usock.h>
#include <gsmd/select.h>
#include <gsmd/atcmd.h>
#include <gsmd/usock.h>
#include <gsmd/talloc.h>
#include <gsmd/extrsp.h>
#include <gsmd/ts0707.h>
#include <gsmd/sms.h>

#include <ipc/tbus.h>

static void *__ucmd_ctx, *__gu_ctx;

/* callback for completed passthrough gsmd_atcmd's */
static int usock_passthru_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;

	return tbus_method_return(gu->service_sender, "Passthrough", "s", &resp);
}

/* callback for completed gsmd_atcmd's */
static int usock_cmd_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
//      struct gsmd_user *gu = ctx;

	return 0;
}

typedef int usock_method_handler(struct gsmd_user *gu, struct tbus_message *msg);

/* Connect method
 */
static int usock_rcv_connect(struct gsmd_user *gu, struct tbus_message *msg)
{
//      if(!strcmp("0", msg->data)) {
	/* disconnect client from gsmd */
	DEBUGP("EOF, this client has just vanished\n");
	/* EOF, this client has just vanished */
	gsmd_unregister_fd(&gu->gfd);
	close(gu->gfd.fd);
	/* finish pending atcmd's from this client thus
	 * destroying references to the user structure.  */
	atcmd_terminate_matching(gu->gsmd, gu);
	/* destroy whole user structure */
	llist_del(&gu->list);
	talloc_free(gu);
//      }

	return 0;
}

/* Passthrough method
 */
static int usock_rcv_passthrough(struct gsmd_user *gu, struct tbus_message *msg)
{
	struct gsmd_atcmd *cmd;
	cmd = atcmd_fill(msg->data, 0, &usock_passthru_cb, gu, 0, NULL);
	if (!cmd)
		return -ENOMEM;

	DEBUGP("submitting cmd=%p, gu=%p\n", cmd, gu);

	return atcmd_submit(gu->gsmd, cmd);
}

static int voicecall_get_stat_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
//      struct gsmd_user *gu = ctx;
	struct gsmd_call_status gcs;
	struct gsm_extrsp *er;

	DEBUGP("resp: %s\n", resp);

	er = extrsp_parse(cmd, resp);

	if (!er)
		return -ENOMEM;

	gcs.is_last = (cmd->ret == 0 || cmd->ret == 4) ? 1 : 0;

	if (!strncmp(resp, "OK", 2)) {
		/* No existing call */
		gcs.idx = 0;
	} else if (!strncmp(resp, "+CME", 4)) {
		/* +CME ERROR: <err> */
		DEBUGP("+CME error\n");
		gcs.idx = 0 - atoi(strpbrk(resp, "0123456789"));
	} else if (er->num_tokens == 7 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[3].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[4].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[5].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[6].type == GSMD_ECMD_RTT_NUMERIC) {
		/*
		 * [+CLCC: <id1>,<dir>,<stat>,<mode>,<mpty>[,
		 * <number>,<type>[,<alpha>]]
		 * [<CR><LF>+CLCC: <id2>,<dir>,<stat>,<mode>,<mpty>[,
		 * <number>,<type>[,<alpha>]]
		 * [...]]]
		 */

		gcs.idx = er->tokens[0].u.numeric;
		gcs.dir = er->tokens[1].u.numeric;
		gcs.stat = er->tokens[2].u.numeric;
		gcs.mode = er->tokens[3].u.numeric;
		gcs.mpty = er->tokens[4].u.numeric;
		strlcpy(gcs.number, er->tokens[5].u.string, GSMD_ADDR_MAXLEN + 1);
		gcs.type = er->tokens[6].u.numeric;
	} else if (er->num_tokens == 8 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[3].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[4].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[5].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[6].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[7].type == GSMD_ECMD_RTT_STRING) {

		/*
		 * [+CLCC: <id1>,<dir>,<stat>,<mode>,<mpty>[,
		 * <number>,<type>[,<alpha>]]
		 * [<CR><LF>+CLCC: <id2>,<dir>,<stat>,<mode>,<mpty>[,
		 * <number>,<type>[,<alpha>]]
		 * [...]]]
		 */

		gcs.idx = er->tokens[0].u.numeric;
		gcs.dir = er->tokens[1].u.numeric;
		gcs.stat = er->tokens[2].u.numeric;
		gcs.mode = er->tokens[3].u.numeric;
		gcs.mpty = er->tokens[4].u.numeric;
		strlcpy(gcs.number, er->tokens[5].u.string, GSMD_ADDR_MAXLEN + 1);
		gcs.type = er->tokens[6].u.numeric;
		strlcpy(gcs.alpha, er->tokens[7].u.string, GSMD_ALPHA_MAXLEN + 1);
	} else {
		DEBUGP("Invalid Input : Parse error\n");
		return -EINVAL;
	}

	talloc_free(er);

	 /*TODO*/ return 0;
//      return gsmd_ucmd_submit(gu, GSMD_MSG_VOICECALL, GSMD_VOICECALL_GET_STAT,
//                      cmd->id, sizeof(gcs), &gcs);
}

static int voicecall_ctrl_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
//      struct gsmd_user *gu = ctx;
	int ret = 0;

	DEBUGP("resp: %s\n", resp);

	if (!strncmp(resp, "+CME", 4)) {
		/* +CME ERROR: <err> */
		DEBUGP("+CME error\n");
		ret = atoi(strpbrk(resp, "0123456789"));
	}

	/*TODO*/ return 0;
//      return gsmd_ucmd_submit(gu, GSMD_MSG_VOICECALL, GSMD_VOICECALL_CTRL,
//                      cmd->id, sizeof(ret), &ret);
}

static int voicecall_fwd_stat_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
//      struct gsmd_user *gu = ctx;
	struct gsm_extrsp *er;
	struct gsmd_call_fwd_stat gcfs;

	DEBUGP("resp: %s\n", resp);

	er = extrsp_parse(cmd, resp);

	if (!er)
		return -ENOMEM;

	gcfs.is_last = (cmd->ret == 0 || cmd->ret == 4) ? 1 : 0;

	if (er->num_tokens == 2 &&
	    er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
	    er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC) {

		/*
		 * +CCFC: <status>,<class1>[,<number>,<type>
		 * [,<subaddr>,<satype>[,<time>]]][
		 * <CR><LF>+CCFC: <status>,<class2>[,<number>,<type>
		 * [,<subaddr>,<satype>[,<time>]]]
		 * [...]]
		 */

		gcfs.status = er->tokens[0].u.numeric;
		gcfs.classx = er->tokens[1].u.numeric;
		gcfs.addr.number[0] = '\0';
	} else if (er->num_tokens == 4 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[2].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[3].type == GSMD_ECMD_RTT_NUMERIC) {

		gcfs.status = er->tokens[0].u.numeric;
		gcfs.classx = er->tokens[1].u.numeric;
		strlcpy(gcfs.addr.number, er->tokens[2].u.string, GSMD_ADDR_MAXLEN + 1);
		gcfs.addr.type = er->tokens[3].u.numeric;
	} else if (er->num_tokens == 7 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[2].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[3].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[4].type == GSMD_ECMD_RTT_EMPTY &&
		   er->tokens[5].type == GSMD_ECMD_RTT_EMPTY &&
		   er->tokens[6].type == GSMD_ECMD_RTT_NUMERIC) {

		gcfs.status = er->tokens[0].u.numeric;
		gcfs.classx = er->tokens[1].u.numeric;
		strlcpy(gcfs.addr.number, er->tokens[2].u.string, GSMD_ADDR_MAXLEN + 1);
		gcfs.addr.type = er->tokens[3].u.numeric;
		gcfs.time = er->tokens[6].u.numeric;
	} else {
		DEBUGP("Invalid Input : Parse error\n");
		return -EINVAL;
	}

	talloc_free(er);

	 /*TODO*/ return 0;
//      return gsmd_ucmd_submit(gu, GSMD_MSG_VOICECALL, GSMD_VOICECALL_FWD_STAT,
//                      cmd->id, sizeof(gcfs), &gcfs);
}

static int usock_ringing_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;

	/* If the incoming call answer/rejection succeeded then we
	 * know the modem isn't ringing and we update the state info.  */
	if (cmd->ret == 0)
		gu->gsmd->dev_state.ringing = 0;
	return usock_cmd_cb(cmd, ctx, resp);
}

/* VoiceCall method */
static int usock_rcv_voicecall(struct gsmd_user *gu, struct tbus_message *msg)
{
	struct gsmd_atcmd *cmd = NULL;
//      struct gsmd_addr *ga;
//      struct gsmd_dtmf *gd;
//      struct gsmd_call_ctrl *gcc;
//      struct gsmd_call_fwd_reg *gcfr;
	char buf[64];
	int atcmd_len, err, numlen;

	if (!strcmp("VoiceCall/Dial", msg->object)) {
		char *number;

		err = tbus_get_message_args(msg, "s", &number);
		if (err < 0)
			return err;
		numlen = strip_string(number, "1234567890+");
		if (numlen <= 0)
			return -1;

		cmd = atcmd_fill("ATD", 5 + numlen, &usock_cmd_cb, gu, 0, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "ATD%s;", number);
		free(number);
		/* FIXME: number type! */
	} else if (!strcmp("VoiceCall/HangUp", msg->object)) {
		/* ATH0 is not supported by QC, we hope ATH is supported by everone */
		cmd = atcmd_fill("ATH", 0,
				 gu->gsmd->dev_state.ringing ?
				 usock_ringing_cb : usock_cmd_cb, gu, 0, NULL);

		/* This command is special because it needs to be sent to
		 * the MS even if a command is currently executing.  */
		if (cmd) {
			return cancel_atcmd(gu->gsmd, cmd);
		}
	} else if (!strcmp("VoiceCall/Answer", msg->object)) {
		cmd = atcmd_fill("ATA", 0, &usock_ringing_cb, gu, 0, NULL);
	} else if (!strcmp("VoiceCall/DTMF", msg->object)) {
		char *str;
		tbus_get_message_args(msg, "s", &str);
		atcmd_len = 7 + strlen(str) + 1;
		cmd = atcmd_fill("AT+VTS=", atcmd_len, &usock_cmd_cb, gu, 0, NULL);
		if (!cmd)
			return -ENOMEM;

		sprintf(cmd->buf, "AT+VTS=%s;", str);
		free(str);
	} else if (!strcmp("VoiceCall/GetStatus", msg->object)) {
		cmd = atcmd_fill("AT+CLCC", 0, &voicecall_get_stat_cb, gu, 0, NULL);
		if (!cmd)
			return -ENOMEM;
	} else if (!strcmp("VoiceCall/Control", msg->object)) {
		int n, x;
		tbus_get_message_args(msg, "ii", &n, &x);
		atcmd_len = 8 + 2 + 1;
		cmd = atcmd_fill("AT+CHLD=", atcmd_len, &voicecall_ctrl_cb, gu, 0, NULL);
		if (!cmd)
			return -ENOMEM;

		sprintf(cmd->buf, "AT+CHLD=%d%d", n, x);
	} else if (!strcmp("VoiceCall/ForwardControl", msg->object)) {
		int reas, mode, number;
		tbus_get_message_args(msg, "iii", &reas, &mode, &number);
		atcmd_len = 8 + 6 + 1;
		cmd = atcmd_fill("AT+CCFC=", atcmd_len, &usock_cmd_cb, gu, 0, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CCFC=%d,%d,%d", reas, mode, number);
	} else if (!strcmp("VoiceCall/ForwardStatus", msg->object)) {
		int reas;
		tbus_get_message_args(msg, "i", &reas);
		sprintf(buf, "%d,2", reas);

		atcmd_len = 8 + strlen(buf) + 1;
		cmd = atcmd_fill("AT+CCFC=", atcmd_len, &voicecall_fwd_stat_cb, gu, 0, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CCFC=%s", buf);
	} else
		return -EINVAL;

	if (cmd)
		return atcmd_submit(gu->gsmd, cmd);
	else
		return -ENOMEM;
}

static int null_cmd_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	gsmd_log(GSMD_DEBUG, "null cmd cb\n");
	return 0;
}

/* PIN command callback. Gets called for response to AT+CPIN= cmcd */
static int pin_cmd_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd *g = gu->gsmd;

	int ret = cmd->ret;

	/* Siemens modems return `+CPIN: TRUE` after valid PIN entry
	   and CME 16 if PIN is incorrect */
	if (!strncmp(resp, "+CPIN: TRUE", 11)) {
		ret = 0;
	}

	if (ret == 0)	/*FIXME! call this only one time */
		gsmd_initsettings_after_pin(g);

	/* Pass a GSM07.07 CME code directly, don't issue a new PIN
	 * request because the client waits for a response to her
	 * PIN submission rather than an event.  */
	return tbus_method_return(gu->service_sender, "PIN/Input", "i", &ret);
}

static const char *pin_type_names[__NUM_GSMD_PIN] = {
	[GSMD_PIN_READY] = "READY",
	[GSMD_PIN_SIM_PIN] = "SIM PIN",
	[GSMD_PIN_SIM_PUK] = "SIM PUK",
	[GSMD_PIN_PH_SIM_PIN] = "Phone-to-SIM PIN",
	[GSMD_PIN_PH_FSIM_PIN] = "Phone-to-very-first SIM PIN",
	[GSMD_PIN_PH_FSIM_PUK] = "Phone-to-very-first SIM PUK",
	[GSMD_PIN_SIM_PIN2] = "SIM PIN2",
	[GSMD_PIN_SIM_PUK2] = "SIM PUK2",
	[GSMD_PIN_PH_NET_PIN] = "Network personalization PIN",
	[GSMD_PIN_PH_NET_PUK] = "Network personalizaiton PUK",
	[GSMD_PIN_PH_NETSUB_PIN] = "Network subset personalisation PIN",
	[GSMD_PIN_PH_NETSUB_PUK] = "Network subset personalisation PUK",
	[GSMD_PIN_PH_SP_PIN] = "Service provider personalisation PIN",
	[GSMD_PIN_PH_SP_PUK] = "Service provider personalisation PUK",
	[GSMD_PIN_PH_CORP_PIN] = "Corporate personalisation PIN",
	[GSMD_PIN_PH_CORP_PUK] = "Corporate personalisation PUK",
};

int pin_name_to_type(char *pin_name)
{
	enum gsmd_pin_type type = 0;

	unsigned int i;
	for (i = 0; i < __NUM_GSMD_PIN; i++) {
		if (!strcmp(pin_name, pin_type_names[i])) {
			type = i;
			break;
		}
	}

	return type;
}

/* PIN read callback. Gets called for response to AT+CPIN? cmcd */
static int get_cpin_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	enum gsmd_pin_type type = 0;

	if (!strncmp(resp, "+CPIN: ", 7))
		type = pin_name_to_type(resp + 7);

	if (gu->gsmd->shmem) {
		gu->gsmd->shmem->PhoneServer.PIN_Type = type;
	}

	return tbus_method_return(gu->service_sender, "PIN/GetStatus", "i", &type);
}

/* PIN Method */
static int usock_rcv_pin(struct gsmd_user *gu, struct tbus_message *msg)
{
	struct gsmd_atcmd *cmd;

	if (!strcmp("PIN/Input", msg->object)) {
		char *oldpin, *newpin;

		tbus_get_message_args(msg, "ss", &oldpin, &newpin);

		int newpinlen = strlen(newpin);
		int atcmdlen = 8 + strlen(oldpin) + 1;
		if (newpinlen > 0)
			atcmdlen += 1 + newpinlen;
		cmd = atcmd_fill("AT+CPIN=", atcmdlen, &pin_cmd_cb, gu, 0, NULL);
		if (!cmd)
			return -ENOMEM;

		strlcat(cmd->buf, oldpin, cmd->buflen);

		if (newpinlen > 0) {
			strlcat(cmd->buf, ",", cmd->buflen);
			strlcat(cmd->buf, newpin, cmd->buflen);
		}
		//cmd->buflen = strlen(cmd->buf);
		/*strlcat(cmd->buf, "\"", cmd->buflen); */
		free(oldpin);
		free(newpin);
	} else if (!strcmp("PIN/GetStatus", msg->object)) {
		cmd = atcmd_fill("AT+CPIN?", 0, &get_cpin_cb, gu, 0, NULL);
	} else {
		return -EINVAL;
	}

	return atcmd_submit(gu->gsmd_slow, cmd);
}

static int phone_powerup_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	int ret = cmd->ret;

	/* We need to verify if there is some error */
	switch (ret) {
	case 0:
		gsmd_log(GSMD_DEBUG, "Radio powered-up\n");
		gu->gsmd->dev_state.on = 1;
		break;
	default:
		/* something went wrong */
		gsmd_log(GSMD_DEBUG, "Radio power-up failed\n");
		break;
	}

	return 0;
}

static int phone_powerdown_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	int ret = cmd->ret;
	return 0;
}

static int phone_power_status_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	DEBUGP("resp: %s\n", resp);
	if (!strncmp(resp, "+CFUN: ", 7))
		resp += 7;
	return tbus_method_return(gu->service_sender, "Phone/PowerStatus", "s", &resp);
}

static int phone_get_manuf_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;

	DEBUGP("cmd = '%s', resp: '%s'\n", cmd->buf, resp);
	if (!strncmp(resp, "+CGMI: ", 7))
		resp += 7;

	if (gu->gsmd->shmem) {
		strncpy(gu->gsmd->shmem->PhoneServer.Phone_Manuf, resp, 31);
		gu->gsmd->shmem->PhoneServer.Phone_Manuf[31] = 0;
	}

	return tbus_method_return(gu->service_sender, "Phone/GetManufacturer", "s", &resp);
}

static int phone_get_model_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;

	DEBUGP("cmd = '%s', resp: '%s'\n", cmd->buf, resp);
	if (!strncmp(resp, "+CGMM: ", 7))
		resp += 7;

	if (gu->gsmd->shmem) {
		strncpy(gu->gsmd->shmem->PhoneServer.Phone_Model, resp, 31);
		gu->gsmd->shmem->PhoneServer.Phone_Model[31] = 0;
	}

	return tbus_method_return(gu->service_sender, "Phone/GetModel", "s", &resp);
}

static int phone_get_revision_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;

	DEBUGP("cmd = '%s', resp: '%s'\n", cmd->buf, resp);
	if (!strncmp(resp, "+CGMR: ", 7))
		resp += 7;

	if (gu->gsmd->shmem) {
		strncpy(gu->gsmd->shmem->PhoneServer.Phone_Revision, resp, 31);
		gu->gsmd->shmem->PhoneServer.Phone_Revision[31] = 0;
	}

	return tbus_method_return(gu->service_sender, "Phone/GetRevision", "s", &resp);
}

static int phone_get_serial_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;

	DEBUGP("cmd = '%s', resp: '%s'\n", cmd->buf, resp);
	if (!strncmp(resp, "+CGSN: ", 7))
		resp += 7;

	if (gu->gsmd->shmem) {
		strncpy(gu->gsmd->shmem->PhoneServer.Phone_Serial, resp, 31);
		gu->gsmd->shmem->PhoneServer.Phone_Serial[31] = 0;
	}

	return tbus_method_return(gu->service_sender, "Phone/GetSerialNum", "s", &resp);
}

static int phone_get_battery_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_battery_charge gbs;
	struct gsm_extrsp *er;

	DEBUGP("cmd = '%s', resp: '%s'\n", cmd->buf, resp);
	er = extrsp_parse(gsmd_tallocs, resp);
	if (!er)
		return -ENOMEM;
	/* +CBC: 0,0 */
	if ((er->num_tokens == 2) &&
	    er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
	    er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC) {
		gbs.bcs = er->tokens[0].u.numeric;
		gbs.bcl = er->tokens[1].u.numeric;
	}
	talloc_free(er);

	int chargelevel = (gbs.bcl <= 80)?((gbs.bcl + 19) / 20):(5);

	if (gu->gsmd->shmem) {
		gu->gsmd->shmem->Battery.Status = gbs.bcs;
		gu->gsmd->shmem->Battery.ChargeLevel = chargelevel;
	}

	return tbus_method_return(gu->service_sender, "Phone/GetBattery", "ii", &gbs.bcs, &gbs.bcl);
}

static int phone_vibrator_enable_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	int ret = cmd->ret;

	switch (ret) {
	case 0:
		gsmd_log(GSMD_DEBUG, "Vibrator enabled\n");
		gu->gsmd->dev_state.vibrator = 1;
		break;
	default:
		gsmd_log(GSMD_DEBUG, "AT+CVIB=1 operation failed\n");
		break;
	}

	return 0;
}

static int phone_vibrator_disable_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	int ret = cmd->ret;

	if (ret == 0)
		gu->gsmd->dev_state.vibrator = 0;

	return 0;
}

/* Phone methods */
static int usock_rcv_phone(struct gsmd_user *gu, struct tbus_message *msg)
{
	struct gsmd_atcmd *cmd;

	if (!strcmp("Phone/PowerUp", msg->object)) {
		cmd = atcmd_fill("AT+CFUN=1", 0, &phone_powerup_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/PowerDown", msg->object)) {
		cmd = atcmd_fill("AT+CFUN=0", 0, &phone_powerdown_cb, gu, 0, NULL);
		gu->gsmd->dev_state.on = 0;
	} else if (!strcmp("Phone/PowerStatus", msg->object)) {
		cmd = atcmd_fill("AT+CFUN?", 0, &phone_power_status_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/GetIMSI", msg->object)) {
		return tbus_method_return(gu->service_sender, "Phone/GetIMSI", "s",
					  &gu->gsmd->imsi);
	} else if (!strcmp("Phone/GetManufacturer", msg->object)) {
		cmd = atcmd_fill("AT+CGMI", 0, &phone_get_manuf_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/GetModel", msg->object)) {
		cmd = atcmd_fill("AT+CGMM", 0, &phone_get_model_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/GetRevision", msg->object)) {
		cmd = atcmd_fill("AT+CGMR", 0, &phone_get_revision_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/GetSerialNum", msg->object)) {
		cmd = atcmd_fill("AT+CGSN", 0, &phone_get_serial_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/GetBattery", msg->object)) {
		cmd = atcmd_fill("AT+CBC", 0, &phone_get_battery_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/VibratorOn", msg->object)) {
		cmd = atcmd_fill("AT+CVIB=1", 0, &phone_vibrator_enable_cb, gu, 0, NULL);
	} else if (!strcmp("Phone/VibratorOff", msg->object)) {
		cmd = atcmd_fill("AT+CVIB=0", 0, &phone_vibrator_disable_cb, gu, 0, NULL);
		gu->gsmd->dev_state.vibrator = 0;
	} else
		return -EINVAL;

	if (!cmd)
		return -ENOMEM;

	return atcmd_submit(gu->gsmd, cmd);
}

static int usock_rcv_modem(struct gsmd_user *gu, struct tbus_message *msg)
{
	struct gsmd *g = gu->gsmd;

	if (!strcmp("Modem/Power", msg->object)) {
		int power;
		tbus_get_message_args(msg, "i", &power);
		if (g->machinepl->power) {
			g->machinepl->power(g, power);
		}
	} else if (!strcmp("Modem/Init", msg->object)) {
		gsmd_initsettings2(g);
	} else
		return -EINVAL;

	return 0;
}

static int network_query_reg_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsm_extrsp *er;
	enum gsmd_netreg_state state;

	DEBUGP("cmd = '%s', resp: '%s'\n", cmd->buf, resp);

	if (strncmp(resp, "+CREG: ", 7))
		return -EINVAL;

	er = extrsp_parse(gsmd_tallocs, resp);
	if (!er)
		return -ENOMEM;
	//extrsp_dump(er);
	/*TODO lac, ci, AcT */
	/* +CREG: <n>,<stat>[,<lac>,<ci>, [AcT]] */
	if ((er->num_tokens == 4 || er->num_tokens == 2) &&
	    er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
	    er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC) {
		state = er->tokens[1].u.numeric;
	}

	talloc_free(er);

	if (gu->gsmd->shmem) {
		gu->gsmd->shmem->PhoneServer.CREG_State = state;
	}

	return tbus_method_return(gu->service_sender, "Network/QueryRegistration", "i", &state);
}

static int network_vmail_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_voicemail vmail;
	struct gsm_extrsp *er;
	int rc;
	int ret = cmd->ret;

	DEBUGP("cmd = '%s', resp: '%s'\n", cmd->buf, resp);

	if (cmd->buf[7] == '=') {
		/* response to set command */
		rc = 0;
	} else {
		/* response to get command */
		if (strncmp(resp, "+CSVM: ", 7))
			return -EINVAL;
		resp += 7;
		er = extrsp_parse(gsmd_tallocs, resp);
		if (!er)
			return -ENOMEM;
		if (er->num_tokens == 3 &&
		    er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		    er->tokens[1].type == GSMD_ECMD_RTT_STRING &&
		    er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC) {
			vmail.enable = er->tokens[0].u.numeric;
			strlcpy(vmail.addr.number, er->tokens[1].u.string, GSMD_ADDR_MAXLEN + 1);
			vmail.addr.type = er->tokens[2].u.numeric;
		}
		rc = tbus_method_return(gu->service_sender, "Network/VoiceMailGet", "isi",
					&vmail.enable, &vmail.addr.number, &vmail.addr.type);
		talloc_free(er);
	}
	return rc;
}

static int network_sigq_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_signal_quality gsq;
	char *comma;

	gsq.rssi = atoi(resp + 6);
	comma = strchr(resp, ',');
	if (!comma++)
		return -EIO;
	gsq.ber = atoi(comma);

// 	if (gu->gsmd->shmem) {
// 		gu->gsmd->shmem->PhoneServer.Network_Signal.rssi = gsq.rssi;
// 		gu->gsmd->shmem->PhoneServer.Network_Signal.ber = gsq.ber;
// 	}

	return tbus_method_return(gu->service_sender, "Network/SignalGet", "ii", &gsq.rssi,
				  &gsq.ber);
}

static int network_oper_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	const char *end, *opname;
	int format, s, ret;
	char *buf;

	/* Format: <mode>[,<format>,<oper>] */
	/* In case we're not registered, return an empty string.  */
	if (sscanf(resp, "+COPS: %*i,%i,\"%n", &format, &s) <= 0)
		end = opname = resp;
	else {
		/* If the phone returned the opname in a short or numeric
		 * format, then it probably doesn't know the operator's full
		 * name or doesn't support it.  Return any information we
		 * have in this case.  */
		if (format != 0)
			gsmd_log(GSMD_NOTICE, "+COPS response in a format "
				 " different than long alphanumeric - " " returning as is!\n");
		opname = resp + s;
		end = strchr(opname, '"');
		if (!end)
			return -EINVAL;
	}

	buf = strndup(opname, end - opname);

	if (gu->gsmd->shmem) {
		strncpy(gu->gsmd->shmem->PhoneServer.Network_Operator, buf, 64+8);
	}

	ret = tbus_method_return(gu->service_sender, "Network/OperatorGet", "s", &buf);
	free(buf);

	return ret;
}

static int network_oper_n_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	char buf[16 + 1] = { '\0' };
	struct gsm_extrsp *er;

	er = extrsp_parse(cmd, resp);

	if (!er)
		return -ENOMEM;

	//extrsp_dump(er);

	/* Format: <mode>[,<format>,<oper>] */
	if (er->num_tokens == 1 && er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC) {

		/* In case we're not registered, return an empty string */
		buf[0] = '\0';
	} else if (er->num_tokens >= 3 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[2].type == GSMD_ECMD_RTT_STRING) {

		strlcpy(buf, er->tokens[2].u.string, sizeof(buf));
	} else {
		DEBUGP("Invalid Input : Parse error\n");
		return -EINVAL;
	}

	talloc_free(er);

	if (gu->gsmd->shmem) {
		strncpy(gu->gsmd->shmem->PhoneServer.Network_OperatorNum, buf, 7);
		gu->gsmd->shmem->PhoneServer.Network_OperatorNum[7] = 0;
	}

	return tbus_method_return(gu->service_sender, "Network/OperatorGetNum", "s", &buf);
}

static int network_opers_parse(const char *str, struct gsmd_msg_oper **out)
{
	int len = 0;
	struct gsm_extrsp *er;
	char buf[64];
	char *head, *tail, *ptr;
	struct gsmd_msg_oper *out2;

	if (strncmp(str, "+COPS: ", 7))
		return -EINVAL;
	/*
	 * string ",," means the begginig of extended parameters and we
	 * don't want to scan them for operators.
	 */
	ptr = strstr(str, ",,");
	if (ptr)
		ptr[0] = '\0';

	ptr = (char *)str;
	while (*str) {
		if (*str == '(' && isdigit(*(str + 1))) {
			len++;
			str += 2;
		} else
			str++;
	}

	*out = talloc_size(__gu_ctx, sizeof(struct gsmd_msg_oper) * (len + 1));

	if (!out)
		return -ENOMEM;

	out2 = *out;
	str = ptr;

	while (*str) {
		if (*str == '(')
			head = (char *)str;
		else if (*str == ')') {
			tail = (char *)str;

			memset(buf, '\0', sizeof(buf));
			strncpy(buf, head + 1, (tail - head - 1));

			DEBUGP("buf: %s\n", buf);

			er = extrsp_parse(gsmd_tallocs, buf);

			if (!er)
				return -ENOMEM;

			//extrsp_dump(er);

			if (er->num_tokens >= 4 &&
			    er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
			    er->tokens[1].type == GSMD_ECMD_RTT_STRING &&
			    er->tokens[2].type == GSMD_ECMD_RTT_STRING &&
			    er->tokens[3].type == GSMD_ECMD_RTT_STRING) {

				/*
				 * +COPS=? +COPS: [list of supported (<stat>,long alphanumeric <oper>
				 *       ,short alphanumeric <oper>,numeric <oper>)s]
				 */

				out2->stat = er->tokens[0].u.numeric;
				strlcpy(out2->opname_longalpha, er->tokens[1].u.string,
					sizeof(out2->opname_longalpha));
				strlcpy(out2->opname_shortalpha, er->tokens[2].u.string,
					sizeof(out2->opname_shortalpha));
				strlcpy(out2->opname_num, er->tokens[3].u.string,
					sizeof(out2->opname_num));
			} else {
				DEBUGP("Invalid Input : Parse error\n");
				talloc_free(*out);
				return -EINVAL;
			}

			talloc_free(er);
			out2->is_last = 0;
			out2++;
		}

		str++;
	}

	out2->is_last = 1;
	return len;
}

static int network_opers_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_msg_oper *buf = NULL;
	int len, ret;

	len = network_opers_parse(resp, &buf);
	if (len < 0)
		return len;	/* error we got from network_opers_parse */

/*	ret = gsmd_ucmd_submit(gu, GSMD_MSG_NETWORK, GSMD_NETWORK_OPER_LIST,
			cmd->id, sizeof(*buf) * (len + 1), buf);*/
	/*TODO*/
	talloc_free(buf);
	return ret;
}

static int network_pref_opers_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = (struct gsmd_user *)ctx;
	struct gsmd_msg_prefoper entry;
	int index;
	char opname[17];

	if (cmd->ret && cmd->ret != -255)
		return 0;	/* TODO: Send a response */

	if (sscanf(resp, "+CPOL: %i,0,\"%16[^\"]\"", &index, opname) < 2)
		return -EINVAL;	/* TODO: Send a response */

	entry.index = index;
	entry.is_last = (cmd->ret == 0);
	memcpy(entry.opname_longalpha, opname, sizeof(entry.opname_longalpha));

/*	return gsmd_ucmd_submit(gu, GSMD_MSG_NETWORK, GSMD_NETWORK_PREF_LIST,
			cmd->id, sizeof(entry), &entry);*/
}

static int network_pref_num_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = (struct gsmd_user *)ctx;
	int min_index, max_index, size;

	if (cmd->ret)
		return 0;	/* TODO: Send a response */

	/* This is not a full general case, theoretically the range string
	 * can include commas and more dashes, but we have no full parser for
	 * ranges yet.  */
	if (sscanf(resp, "+CPOL: (%i-%i)", &min_index, &max_index) < 2)
		return -EINVAL;	/* TODO: Send a response */
	size = max_index - min_index + 1;

/*	return gsmd_ucmd_submit(gu, GSMD_MSG_NETWORK, GSMD_NETWORK_PREF_SPACE,
			cmd->id, sizeof(size), &size);*/
}

static int network_ownnumbers_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = (struct gsmd_user *)ctx;
	struct gsmd_own_number *num;
	int len, ret, type;
	char dummy;

	if (cmd->ret && cmd->ret != -255)
		return 0;	/* TODO: Send a response */

	if (sscanf(resp, "+CNUM: \"%*[^\"]\"%c%n", &dummy, &len) > 0)
		len -= strlen("+CNUM: \"\",");
	else
		len = 0;

	num = talloc_size(__gu_ctx, sizeof(*num) + len + 1);
	if (len)
		ret = sscanf(resp, "+CNUM: \"%[^\"]\",\"%32[^\"]\",%i,%*i,%i,",
			     num->name, num->addr.number, &type, &num->service) - 1;
	else
		ret = sscanf(resp, "+CNUM: ,\"%32[^\"]\",%i,%*i,%i,",
			     num->addr.number, &type, &num->service);
	if (ret < 2) {
		talloc_free(num);
		return -EINVAL;	/* TODO: Send a response */
	}
	if (ret < 3)
		num->service = GSMD_SERVICE_UNKNOWN;
	num->name[len] = 0;
	num->addr.type = type;
	num->is_last = (cmd->ret == 0);

/*	ret = gsmd_ucmd_submit(gu, GSMD_MSG_NETWORK, GSMD_NETWORK_GET_NUMBER,
			cmd->id, sizeof(*num) + len + 1, num);*/
	/*TODO*/
	talloc_free(num);
	return ret;
}

/* Network methods */
static int usock_rcv_network(struct gsmd_user *gu, struct tbus_message *msg)
{
	struct gsmd_atcmd *cmd;
//      struct gsmd_voicemail *vmail = (struct gsmd_voicemail *) gph->data;
//      gsmd_oper_numeric *oper = (gsmd_oper_numeric *) gph->data;
	char buffer[15 + sizeof(gsmd_oper_numeric)];
	int cmdlen;

	if (!strcmp("Network/RegisterAuto", msg->object)) {
		cmdlen = sprintf(buffer, "AT+COPS=0");
		cmd = atcmd_fill(buffer, cmdlen + 1, &null_cmd_cb, gu, 0, NULL);
	} else if (!strcmp("Network/RegisterManual", msg->object)) {
		char *oper;
		int ret = tbus_get_message_args(msg, "s", &oper);
		if (!ret && strlen(oper) > 0) {
			cmdlen = sprintf(buffer, "AT+COPS=1,2,\"%s\"", oper);
			free(oper);
		} else {
			cmdlen = sprintf(buffer, "AT+COPS=0");
		}
		cmd = atcmd_fill(buffer, cmdlen + 1, &null_cmd_cb, gu, 0, NULL);
	} else if (!strcmp("Network/DeRegister", msg->object)) {
		cmd = atcmd_fill("AT+COPS=2", 0, &null_cmd_cb, gu, 0, NULL);
	} else if (!strcmp("Network/QueryRegistration", msg->object)) {
		cmd = atcmd_fill("AT+CREG?", 0, &network_query_reg_cb, gu, 0, NULL);
	} else if (!strcmp("Network/VoiceMailGet", msg->object)) {
		cmd = atcmd_fill("AT+CSVM?", 0, &network_vmail_cb, gu, 0, NULL);
	} else if (!strcmp("Network/VoiceMailSet", msg->object)) {
		char *number;
		int type;
		tbus_get_message_args(msg, "si", &number, &type);
		cmdlen = sprintf(buffer, "AT+CSVM=1,\"%s\",%d", number, type);
		cmd = atcmd_fill(buffer, cmdlen + 1, &network_vmail_cb, gu, 0, NULL);
		free(number);
	} else if (!strcmp("Network/SignalGet", msg->object)) {
		cmd = atcmd_fill("AT+CSQ", 0, &network_sigq_cb, gu, 0, NULL);
	} else if (!strcmp("Network/OperatorGet", msg->object)) {
		/* Set long alphanumeric format */
/*		atcmd_submit(gu->gsmd, atcmd_fill("AT+COPS=3,0", 11 + 1,
						  &null_cmd_cb, gu, 0, NULL));*/
		cmd = atcmd_fill("AT+COPS=3,0;+COPS?", 0, &network_oper_cb, gu, 0, NULL);
	} else if (!strcmp("Network/OperatorGetNum", msg->object)) {
		/* Set numeric format */
/*		atcmd_submit(gu->gsmd, atcmd_fill("AT+COPS=3,2", 11 + 1,
						  &null_cmd_cb, gu, 0, NULL));*/
		cmd = atcmd_fill("AT+COPS=3,2;+COPS?", 0, &network_oper_n_cb, gu, 0, NULL);
	} else if (!strcmp("Network/OperatorList", msg->object)) {
		cmd = atcmd_fill("AT+COPS=?", 0, &network_opers_cb, gu, 0, NULL);
	} else if (!strcmp("Network/PreferredList", msg->object)) {
		/* Set long alphanumeric format */
		atcmd_submit(gu->gsmd, atcmd_fill("AT+CPOL=,0", 0, &null_cmd_cb, gu, 0, NULL));
		cmd = atcmd_fill("AT+CPOL?", 0, &network_pref_opers_cb, gu, 0, NULL);
	} else if (!strcmp("Network/PreferredDel", msg->object)) {
		int tmp;
		tbus_get_message_args(msg, "i", &tmp);
		cmdlen = sprintf(buffer, "AT+CPOL=%i", tmp);
		cmd = atcmd_fill(buffer, cmdlen + 1, &null_cmd_cb, gu, 0, NULL);
	} else if (!strcmp("Network/PreferredAdd", msg->object)) {
		char *str;
		tbus_get_message_args(msg, "s", &str);
		cmdlen = sprintf(buffer, "AT+CPOL=,2,\"%.*s\"", sizeof(gsmd_oper_numeric), str);
		cmd = atcmd_fill(buffer, cmdlen + 1, &null_cmd_cb, gu, 0, NULL);
		free(str);
	} else if (!strcmp("Network/PreferredSpace", msg->object)) {
		cmd = atcmd_fill("AT+CPOL=?", 0, &network_pref_num_cb, gu, 0, NULL);
	} else if (!strcmp("Network/GetOwnNum", msg->object)) {
		cmd = atcmd_fill("AT+CNUM", 0, &network_ownnumbers_cb, gu, 0, NULL);
	} else
		return -EINVAL;
	if (!cmd)
		return -ENOMEM;

	return atcmd_submit(gu->gsmd, cmd);
}

static int phonebook_find_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_phonebooks gps;
	struct gsm_extrsp *er;

	DEBUGP("resp: %s\n", resp);

	er = extrsp_parse(cmd, resp);

	if (!er)
		return -ENOMEM;

	gps.is_last = (cmd->ret == 0 || cmd->ret == 4) ? 1 : 0;

	if (!strncmp(resp, "OK", 2)) {
		/* The record is empty or could not read yet */
		gps.pb.index = 0;
	} else if (!strncmp(resp, "+CME", 4)) {
		DEBUGP("== +CME error\n");
		/* +CME ERROR: 21 */
		gps.pb.index = 0 - atoi(strpbrk(resp, "0123456789"));
	} else if (er->num_tokens == 4 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[3].type == GSMD_ECMD_RTT_STRING) {

		/*
		 * [+CPBR: <index1>,<number>,<type>,<text>[[...]
		 * <CR><LF>+CPBR: <index2>,<unmber>,<type>,<text>]]
		 */

		gps.pb.index = er->tokens[0].u.numeric;
		strlcpy(gps.pb.numb, er->tokens[1].u.string, GSMD_PB_NUMB_MAXLEN + 1);
		gps.pb.type = er->tokens[2].u.numeric;
		strlcpy(gps.pb.text, er->tokens[3].u.string, GSMD_PB_TEXT_MAXLEN + 1);
	} else {
		DEBUGP("Invalid Input : Parse error\n");
		return -EINVAL;
	}

	talloc_free(er);

/*	return gsmd_ucmd_submit(gu, GSMD_MSG_PHONEBOOK, GSMD_PHONEBOOK_FIND,
			cmd->id, sizeof(gps), &gps);*/
}

static int phonebook_read_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_phonebook gp;
	struct gsm_extrsp *er;

	DEBUGP("resp: %s\n", resp);

	er = extrsp_parse(cmd, resp);

	if (!er)
		return -ENOMEM;

	if (!strncmp(resp, "OK", 2)) {
		/* The record is empty or could not read yet */
		gp.index = 0;
	} else if (!strncmp(resp, "+CME", 4)) {
		DEBUGP("+CME error\n");
		/* +CME ERROR: 21 */
		gp.index = 0 - atoi(strpbrk(resp, "0123456789"));
	} else if (er->num_tokens == 4 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[3].type == GSMD_ECMD_RTT_STRING) {

		/*
		 * [+CPBR: <index1>,<number>,<type>,<text>[[...]
		 * <CR><LF>+CPBR: <index2>,<unmber>,<type>,<text>]]
		 */

		gp.index = er->tokens[0].u.numeric;
		strlcpy(gp.numb, er->tokens[1].u.string, GSMD_PB_NUMB_MAXLEN + 1);
		gp.type = er->tokens[2].u.numeric;
		strlcpy(gp.text, er->tokens[3].u.string, GSMD_PB_TEXT_MAXLEN + 1);
	} else {
		DEBUGP("Invalid Input : Parse error\n");
		return -EINVAL;
	}

	talloc_free(er);

/*	return gsmd_ucmd_submit(gu, GSMD_MSG_PHONEBOOK, GSMD_PHONEBOOK_READ,
			cmd->id, sizeof(gp), &gp);*/
}

static int phonebook_readrg_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_phonebooks gps;
	struct gsm_extrsp *er;

	DEBUGP("resp: %s\n", resp);

	er = extrsp_parse(cmd, resp);

	if (!er)
		return -ENOMEM;

	gps.is_last = (cmd->ret == 0 || cmd->ret == 4) ? 1 : 0;

	if (!strncmp(resp, "OK", 2)) {
		/* The record is empty or could not read yet */
		gps.pb.index = 0;
	} else if (!strncmp(resp, "+CME", 4)) {
		DEBUGP("+CME error\n");
		/* +CME ERROR: 21 */
		gps.pb.index = 0 - atoi(strpbrk(resp, "0123456789"));
	} else if (er->num_tokens == 4 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[1].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[3].type == GSMD_ECMD_RTT_STRING) {

		/*
		 * [+CPBR: <index1>,<number>,<type>,<text>[[...]
		 * <CR><LF>+CPBR: <index2>,<unmber>,<type>,<text>]]
		 */

		gps.pb.index = er->tokens[0].u.numeric;
		strlcpy(gps.pb.numb, er->tokens[1].u.string, GSMD_PB_NUMB_MAXLEN + 1);
		gps.pb.type = er->tokens[2].u.numeric;
		strlcpy(gps.pb.text, er->tokens[3].u.string, GSMD_PB_TEXT_MAXLEN + 1);
	} else {
		DEBUGP("Invalid Input : Parse error\n");
		return -EINVAL;
	}

	talloc_free(er);

/*	return gsmd_ucmd_submit(gu, GSMD_MSG_PHONEBOOK, GSMD_PHONEBOOK_READRG,
			cmd->id, sizeof(gps), &gps);*/
}

static int phonebook_write_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	DEBUGP("resp: %s\n", resp);

/*	return gsmd_ucmd_submit(ctx, GSMD_MSG_PHONEBOOK, GSMD_PHONEBOOK_WRITE,
			cmd->id, strlen(resp) + 1, resp);*/
}

static int phonebook_delete_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	DEBUGP("resp: %s\n", resp);

/*	return gsmd_ucmd_submit(ctx, GSMD_MSG_PHONEBOOK, GSMD_PHONEBOOK_DELETE,
			cmd->id, strlen(resp) + 1, resp);*/
}

static int phonebook_get_support_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	/* TODO: Need to handle command error */
	/* +CPBR: (1-100),44,16 */
	struct gsmd_user *gu = ctx;
	struct gsmd_phonebook_support gps;
	char *fcomma, *lcomma;
	char *dash;

	DEBUGP("resp: %s\n", resp);

	dash = strchr(resp, '-');
	if (!dash)
		return -EIO;	/* TODO: Send a response */
	gps.index = atoi(dash + 1);

	fcomma = strchr(resp, ',');
	if (!fcomma)
		return -EIO;	/* TODO: Send a response */
	gps.nlength = atoi(fcomma + 1);

	lcomma = strrchr(resp, ',');
	if (!lcomma)
		return -EIO;	/* TODO: Send a response */
	gps.tlength = atoi(lcomma + 1);

/*	return gsmd_ucmd_submit(gu,
			GSMD_MSG_PHONEBOOK, GSMD_PHONEBOOK_GET_SUPPORT,
			cmd->id, sizeof(gps), &gps);*/
}

static int phonebook_list_storage_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	/* TODO; using link list ; need to handle command error */
	struct gsmd_user *gu = ctx;
	struct gsmd_phonebook_storage gps;
	char *ptr;

	DEBUGP("resp: %s\n", resp);

	/*
	 * +CPBS: (<storage>s)
	 */
	gps.num = 0;

	if (!strncmp(resp, "+CPBS", 5)) {
		char *delim = "(,";
		ptr = strpbrk(resp, delim);
		while (ptr) {
			strncpy(gps.mem[gps.num].type, ptr + 2, 2);
			gps.mem[gps.num].type[2] = '\0';
			ptr = strpbrk(ptr + 2, delim);
			gps.num++;
		}
	}

/*	return gsmd_ucmd_submit(gu,
			GSMD_MSG_PHONEBOOK, GSMD_PHONEBOOK_LIST_STORAGE,
			cmd->id, sizeof(gps), &gps);*/
}

static int usock_rcv_phonebook(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, int len)
{
	struct gsmd_atcmd *cmd = NULL;
	struct gsmd_phonebook_readrg *gpr;
	struct gsmd_phonebook *gp;
	struct gsmd_phonebook_find *gpf;
	int *index;
	int atcmd_len;
	char *storage;
	char buf[1024];

	switch (gph->msg_subtype) {
	case GSMD_PHONEBOOK_LIST_STORAGE:
		cmd = atcmd_fill("AT+CPBS=?", 0, &phonebook_list_storage_cb, gu, gph->id, NULL);
		break;
	case GSMD_PHONEBOOK_SET_STORAGE:
		if (len < sizeof(*gph) + 3)
			return -EINVAL;

		storage = (char *)((void *)gph + sizeof(*gph));

		/* ex. AT+CPBS="ME" */
		atcmd_len = 1 + strlen("AT+CPBS=\"") + 2 + strlen("\"");
		cmd = atcmd_fill("AT+CPBS=\"", atcmd_len, &usock_cmd_cb, gu, gph->id, NULL);

		if (!cmd)
			return -ENOMEM;

		sprintf(cmd->buf, "AT+CPBS=\"%s\"", storage);
		break;
	case GSMD_PHONEBOOK_FIND:
		if (len < sizeof(*gph) + sizeof(*gpf))
			return -EINVAL;
		gpf = (struct gsmd_phonebook_find *)((void *)gph + sizeof(*gph));

		atcmd_len = 1 + strlen("AT+CPBF=\"") + strlen(gpf->findtext) + strlen("\"");
		cmd = atcmd_fill("AT+CPBF=\"", atcmd_len, &phonebook_find_cb, gu, gph->id, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBF=\"%s\"", gpf->findtext);
		break;
	case GSMD_PHONEBOOK_READ:
		if (len < sizeof(*gph) + sizeof(int))
			return -EINVAL;

		index = (int *)((void *)gph + sizeof(*gph));

		sprintf(buf, "%d", *index);

		/* ex, AT+CPBR=23 */
		atcmd_len = 1 + strlen("AT+CPBR=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBR=", atcmd_len, &phonebook_read_cb, gu, gph->id, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBR=%d", *index);
		break;
	case GSMD_PHONEBOOK_READRG:
		if (len < sizeof(*gph) + sizeof(*gpr))
			return -EINVAL;
		gpr = (struct gsmd_phonebook_readrg *)((void *)gph + sizeof(*gph));

		sprintf(buf, "%d,%d", gpr->index1, gpr->index2);

		/* ex, AT+CPBR=1,100 */
		atcmd_len = 1 + strlen("AT+CPBR=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBR=", atcmd_len, &phonebook_readrg_cb, gu, gph->id, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBR=%s", buf);
		break;
	case GSMD_PHONEBOOK_WRITE:
		if (len < sizeof(*gph) + sizeof(*gp))
			return -EINVAL;
		gp = (struct gsmd_phonebook *)((void *)gph + sizeof(*gph));

		sprintf(buf, "%d,\"%s\",%d,\"%s\"", gp->index, gp->numb, gp->type, gp->text);

		atcmd_len = 1 + strlen("AT+CPBW=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBW=", atcmd_len, &phonebook_write_cb, gu, gph->id, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBW=%s", buf);
		break;
	case GSMD_PHONEBOOK_DELETE:
		if (len < sizeof(*gph) + sizeof(int))
			return -EINVAL;
		index = (int *)((void *)gph + sizeof(*gph));

		sprintf(buf, "%d", *index);

		/* ex, AT+CPBW=3 */
		atcmd_len = 1 + strlen("AT+CPBW=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBW=", atcmd_len, &phonebook_delete_cb, gu, gph->id, NULL);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBW=%s", buf);
		break;
	case GSMD_PHONEBOOK_GET_SUPPORT:
		cmd = atcmd_fill("AT+CPBR=?", 9 + 1, &phonebook_get_support_cb, gu, gph->id, NULL);
		break;
	default:
		return -EINVAL;
	}

	if (cmd)
		return atcmd_submit(gu->gsmd_slow, cmd);
	else
		return 0;
}

static struct usock_methods {
	char *method;
	usock_method_handler *handler;
} pcmd_type_handlers[] = {
	{
	"Passthrough",	&usock_rcv_passthrough}, {
//	"Event",	&usock_rcv_event}, {
	"VoiceCall",	&usock_rcv_voicecall}, {
	"PIN",		&usock_rcv_pin}, {
	"Phone",	&usock_rcv_phone}, {
	"Network",	&usock_rcv_network}, {
	"SMS",		&usock_rcv_sms}, {
	"CB",		&usock_rcv_cb}, {
	"PhoneBook",	&usock_rcv_phonebook}, {
	"Modem",	&usock_rcv_modem}, {
	"Connect",	&usock_rcv_connect}, {
	NULL, NULL},
};

static int usock_rcv_pcmd(struct gsmd_user *gu, struct tbus_message *msg)
{
	int i, len;
	char *str;
	usock_method_handler *umh = NULL;

	for (i = 0; i < __NUM_GSMD_MSGS; i++) {
		str = pcmd_type_handlers[i].method;
		len = strlen(str);
		if (!strncmp(str, msg->object, len)) {
			umh = pcmd_type_handlers[i].handler;
			break;
		}
	}

	DEBUGP("handler found = %d\n", umh != NULL);
	if (!umh)
		return -EINVAL;

	return umh(gu, msg);
}

static struct gsmd_user *usock_connect(struct gsmd *g, struct tbus_message *msg)
{
	struct gsmd_user *newuser;

	/* connect client to gsmd */
	newuser = talloc(__gu_ctx, struct gsmd_user);
	if (!newuser)
		return NULL;

	newuser->gsmd = g;
	newuser->gsmd_slow = g_slow;
	newuser->subscriptions = 0xffffffff;
	newuser->service_sender = strdup(msg->service_sender);
	INIT_LLIST_HEAD(&newuser->finished_ucmds);

	llist_add(&newuser->list, &g->users);

	DEBUGP("added to users = %s\n", newuser->service_sender);

	return newuser;
}

/* callback for read/write on client (libgsmd) socket */
static int gsmd_usock_cb(int fd, unsigned int what, void *data)
{
	struct gsmd *g = data;
	struct gsmd_user *gu = NULL;
	struct tbus_message msg;
	int type;

	if (what & GSMD_FD_READ) {
		type = tbus_get_message(&msg);
		if (type < 0)
			return type;

		DEBUGP("got tbus message %s => %s\n", msg.service_sender, msg.object);

		/* try to find client in users list */
		struct gsmd_user *usr, *usrtmp;
		llist_for_each_entry_safe(usr, usrtmp, &g->users, list) {
			if (!strcmp(msg.service_sender, usr->service_sender)) {
				gu = usr;
				break;
			}
		}

		if (gu == NULL) {
			/* we have new client */
			DEBUGP("new client - adding to llist\n");
			gu = usock_connect(g, &msg);
		}

		if (gu != NULL) {
			DEBUGP("processing message...\n");
			switch (type) {
				case TBUS_MSG_CALL_METHOD:
					return usock_rcv_pcmd(gu, &msg);
				break;
				case TBUS_MSG_SIGNAL:
				break;
			}
		}
		tbus_msg_free(&msg);

		return 0;
	}

	return 0;
}

/* callback for read on master-listen-socket */
int usock_init(struct gsmd *g)
//static int gsmd_usock_cb(int fd, unsigned int what, void *data)
{
	int fd;

	__ucmd_ctx = talloc_named_const(gsmd_tallocs, 1, "ucmd");
	__gu_ctx = talloc_named_const(gsmd_tallocs, 1, "gsmd_user");

	fd = tbus_register_service("PhoneServer");
	if (fd < 0)
		return fd;

	g->gfd_sock.fd = fd;
	g->gfd_sock.when = GSMD_FD_READ | GSMD_FD_EXCEPT;
	g->gfd_sock.data = g;
	g->gfd_sock.cb = &gsmd_usock_cb;

	DEBUGP("usock_init\n");
	return gsmd_register_fd(&g->gfd_sock);
}
