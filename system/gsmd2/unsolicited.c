/* gsmd unsolicited message handling
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "gsmd.h"

#include <gsmd/usock.h>
#include <gsmd/event.h>
#include <gsmd/extrsp.h>
#include <gsmd/ts0707.h>
#include <gsmd/unsolicited.h>
#include <gsmd/talloc.h>
#include <gsmd/sms.h>

#include <ipc/tbus.h>

static void *__us_ctx;

static void state_ringing_timeout(struct gsmd_timer *timer, void *opaque)
{
	struct gsmd *gsmd = (struct gsmd *)opaque;
	int ret, type;

	gsmd->dev_state.ring_check = 0;
	gsmd_timer_free(timer);

	/* Update state */
	if (!gsmd->dev_state.ringing)
		return;
	gsmd->dev_state.ringing = 0;

	gsmd_log(GSMD_INFO, "an incoming call timed out\n");

	/* Generate a timeout event */
	type = GSMD_CALL_TIMEOUT;
	ret = tbus_emit_signal("IncomingCall", "i", &type);
	if (ret < 0)
		goto err;
	return;
      err:
	gsmd_log(GSMD_ERROR, "event generation failed\n");
}

#define GSMD_RING_MAX_JITTER (200 * 1000)	/* 0.2 s */

static void state_ringing_update(struct gsmd *gsmd)
{
	struct timeval tv;

	if (gsmd->dev_state.ring_check)
		gsmd_timer_unregister(gsmd->dev_state.ring_check);

	/* Update state */
	gsmd->dev_state.ringing = 1;

	tv.tv_sec = 1;
	tv.tv_usec = GSMD_RING_MAX_JITTER;

	if (gsmd->dev_state.ring_check) {
		gsmd->dev_state.ring_check->expires = tv;
		gsmd_timer_register(gsmd->dev_state.ring_check);
	} else
		gsmd->dev_state.ring_check = gsmd_timer_create(&tv, &state_ringing_timeout, gsmd);
}

static int ring_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	int ret, type;

	state_ringing_update(gsmd);
	/* FIXME: generate ring event */
	type = GSMD_CALL_UNSPEC;
	ret = tbus_emit_signal("IncomingCall", "i", &type);

	return ret;
}

static int cring_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	int ret, type;

	if (!strcmp(param, "VOICE")) {
		/* incoming voice call */
		type = GSMD_CALL_VOICE;
	} else if (!strcmp(param, "SYNC")) {
		type = GSMD_CALL_DATA_SYNC;
	} else if (!strcmp(param, "REL ASYNC")) {
		type = GSMD_CALL_DATA_REL_ASYNC;
	} else if (!strcmp(param, "REL SYNC")) {
		type = GSMD_CALL_DATA_REL_SYNC;
	} else if (!strcmp(param, "FAX")) {
		type = GSMD_CALL_FAX;
	} else if (!strncmp(param, "GPRS ", 5)) {
		/* FIXME: change event type to GPRS */
		return 0;
	} else {
		type = GSMD_CALL_UNSPEC;
	}
	/* FIXME: parse all the ALT* profiles, Chapter 6.11 */
	ret = tbus_emit_signal("IncomingCall", "i", &type);

	return ret;
}

/* Chapter 7.2, network registration */
static int creg_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	const char *comma = strchr(param, ',');
	int prev_registered = gsmd->dev_state.registered;
	int state;
	char *end;
	int ret;

	state = strtol(param, &end, 10);
	if (!(end > param)) {
		gsmd_log(GSMD_ERROR, "Bad +CREG format, not updating state\n");
		return -EINVAL;
	}

	/* Update our knowledge about our state */
	gsmd->dev_state.registered =
	    (state == GSMD_NETREG_REG_HOME || state == GSMD_NETREG_REG_ROAMING);

	/* Initialize things that depend on network registration */
	if (gsmd->dev_state.registered && !prev_registered) {
		sms_cb_network_init(gsmd);
	}

	/* Notify clients */
	int lac, ci;
	state = atoi(param);
	if (comma) {
		/* we also have location area code and cell id to parse (hex) */
		lac = strtoul(comma + 2, NULL, 16);
		comma = strchr(comma + 1, ',');
		if (!comma)
			return -EINVAL;
		ci = strtoul(comma + 2, NULL, 16);
	} else
		lac = ci = 0;

	if (gsmd->shmem) {
		gsmd->shmem->PhoneServer.CREG_State = state;
		gsmd->shmem->PhoneServer.CREG_Lac = lac;
		gsmd->shmem->PhoneServer.CREG_Ci = ci;
	}

	ret = tbus_emit_signal("NetworkReg", "iii", &state, &lac, &ci);

	return ret;

}

/* Chapter 7.11, call waiting */
static int ccwa_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	struct gsm_extrsp *er;
	int ret;
	char *number = talloc_size(__us_ctx, GSMD_ALPHA_MAXLEN + 1);
	int type, classx, cli;
	char *alpha = talloc_size(__us_ctx, GSMD_ALPHA_MAXLEN + 1);

	er = extrsp_parse(gsmd_tallocs, param);
	if (!er)
		return -ENOMEM;

	if (er->num_tokens == 5 &&
	    er->tokens[0].type == GSMD_ECMD_RTT_STRING &&
	    er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC &&
	    er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC &&
	    er->tokens[3].type == GSMD_ECMD_RTT_EMPTY &&
	    er->tokens[4].type == GSMD_ECMD_RTT_NUMERIC) {
		/*
		 * <number>,<type>,<class>,[<alpha>][,<CLI validity>]
		 */

		strcpy(number, er->tokens[0].u.string);
		type = er->tokens[1].u.numeric;
		classx = er->tokens[2].u.numeric;
		alpha[0] = '\0';
		cli = er->tokens[4].u.numeric;
	} else if (er->num_tokens == 5 &&
		   er->tokens[0].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[1].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[2].type == GSMD_ECMD_RTT_NUMERIC &&
		   er->tokens[3].type == GSMD_ECMD_RTT_STRING &&
		   er->tokens[4].type == GSMD_ECMD_RTT_NUMERIC) {
		/*
		 * <number>,<type>,<class>,[<alpha>][,<CLI validity>]
		 */

		strcpy(number, er->tokens[0].u.string);
		type = er->tokens[1].u.numeric;
		classx = er->tokens[2].u.numeric;
		strcpy(alpha, er->tokens[3].u.string);
		cli = er->tokens[4].u.numeric;
	} else {
		DEBUGP("Invalid Input : Parse error\n");
		return -EINVAL;
	}

	ret = tbus_emit_signal("CallWaiting", "siisi", &number, &type, &classx, &alpha, &cli);
	talloc_free(er);
	talloc_free(number);
	talloc_free(alpha);

	return ret;
}

/* Chapter 7.14, unstructured supplementary service data */
static int cusd_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 7.15, advise of charge */
static int cccm_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 10.1.13, GPRS event reporting */
static int cgev_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 10.1.14, GPRS network registration status */
static int cgreg_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 7.6, calling line identification presentation
+CLIP: "8926xxxxxxx",129,,,,0
*/
static int clip_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	int ret;
	const char *comma = strchr(param, ',');
	char *number = talloc_size(__us_ctx, GSMD_ALPHA_MAXLEN + 1);

	if (!comma)
		return -EINVAL;

	if (comma - param > GSMD_ADDR_MAXLEN)
		return -EINVAL;

	number[0] = '\0';
	strncat(number, param, comma - param);
	/* FIXME: parse of subaddr, etc. */

	ret = tbus_emit_signal("IncomingCLIP", "s", &number);
	talloc_free(number);

	return ret;
}

/* Chapter 7.9, calling line identification presentation */
static int colp_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	int ret;
	const char *comma = strchr(param, ',');
	char *number = talloc_size(__us_ctx, GSMD_ALPHA_MAXLEN + 1);

	if (!comma)
		return -EINVAL;

	if (comma - param > GSMD_ADDR_MAXLEN)
		return -EINVAL;

	number[0] = '\0';
	strncat(number, param, comma - param);
	/* FIXME: parse of subaddr, etc. */

	ret = tbus_emit_signal("OutgoingCOLP", "s", &number);
	talloc_free(number);

	return ret;
}

static int ctzv_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	int tz;

	/* timezones are expressed in quarters of hours +/- GMT (-48...+48) */
	tz = atoi(param);

	if (tz < -48 || tz > 48)
		return -EINVAL;

	return tbus_emit_signal("TimezoneChange", "i", &tz);
}

static int copn_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	struct gsm_extrsp *er = extrsp_parse(gsmd_tallocs, param);
	int rc = 0;

	if (!er)
		return -ENOMEM;

	extrsp_dump(er);

	if (er->num_tokens == 2 &&
	    er->tokens[0].type == GSMD_ECMD_RTT_STRING &&
	    er->tokens[1].type == GSMD_ECMD_RTT_STRING)
		rc = gsmd_opname_add(gsmd, er->tokens[0].u.string, er->tokens[1].u.string);

	talloc_free(er);

	return rc;
}

static const struct gsmd_unsolicit gsm0707_unsolicit[] = {
	{"RING", &ring_parse},
	{"+CRING", &cring_parse},
	{"+CREG", &creg_parse},	/* Network registration */
	{"+CCWA", &ccwa_parse},	/* Call waiting */
	{"+CUSD", &cusd_parse},	/* Unstructured supplementary data */
	{"+CCCM", &cccm_parse},	/* Advice of Charge */
	{"+CGEV", &cgev_parse},	/* GPRS Event */
	{"+CGREG", &cgreg_parse},	/* GPRS Registration */
	{"+CLIP", &clip_parse},
	{"+COLP", &colp_parse},
	{"+CTZV", &ctzv_parse},	/* Timezone */
	{"+COPN", &copn_parse},	/* operator names, treat as unsolicited */
	/*
	   { "+CKEV",   &ckev_parse },
	   { "+CDEV",   &cdev_parse },
	   { "+CIEV",   &ciev_parse },
	   { "+CLAV",   &clav_parse },
	   { "+CCWV",   &ccwv_parse },
	   { "+CLAV",   &clav_parse },
	   { "+CSSU",   &cssu_parse },
	 */
};

static struct gsmd_unsolicit unsolicit[256] = { {0, 0} };

/* called by midlevel parser if a response seems unsolicited */
int unsolicited_parse(struct gsmd *g, const char *buf, int len, const char *param)
{
	struct gsmd_unsolicit *i;
	int rc;

	/* call unsolicited code parser */
	for (i = unsolicit; i->prefix; i++) {
		const char *colon;
		if (strncmp(buf, i->prefix, strlen(i->prefix)))
			continue;

		colon = strchr(buf, ':') + 2;
		if (colon > buf + len)
			colon = NULL;

		rc = i->parse(buf, len, colon, g);
		if (rc == -EAGAIN)
			return rc;
		if (rc < 0)
			gsmd_log(GSMD_ERROR, "error %d during parsing of "
				 "an unsolicied response `%s'\n", rc, buf);
		return rc;
	}

	gsmd_log(GSMD_NOTICE, "no parser for unsolicited response `%s'\n", buf);

	return -ENOENT;
}

int unsolicited_register_array(const struct gsmd_unsolicit *arr, int len)
{
	int curlen = 0;

	while (unsolicit[curlen++].prefix) ;
	if (len + curlen > ARRAY_SIZE(unsolicit))
		return -ENOMEM;

	/* Add at the beginning for overriding to be possible */
	memmove(&unsolicit[len], unsolicit, sizeof(struct gsmd_unsolicit) * curlen);
	memcpy(unsolicit, arr, sizeof(struct gsmd_unsolicit) * len);

	return 0;
}

void unsolicited_init(struct gsmd *g)
{
	struct gsmd_vendor_plugin *vpl = g->vendorpl;

	__us_ctx = talloc_named_const(gsmd_tallocs, 1, "gsmd_unsol");

	/* register generic unsolicited code parser */
	unsolicited_register_array(gsm0707_unsolicit, ARRAY_SIZE(gsm0707_unsolicit));

	/* register vendor-specific unsolicited code parser */
	if (vpl && vpl->num_unsolicit)
		if (unsolicited_register_array(vpl->unsolicit, vpl->num_unsolicit))
			gsmd_log(GSMD_ERROR, "registering vendor-specific "
				 "unsolicited responses failed\n");
}

static unsigned int errors_creating_events[] = {
	GSM0707_CME_PHONE_FAILURE,
	GSM0707_CME_PHONE_NOCONNECT,
	GSM0707_CME_PHONE_ADAPT_RESERVED,
	GSM0707_CME_PH_SIM_PIN_REQUIRED,
	GSM0707_CME_PH_FSIM_PIN_REQUIRED,
	GSM0707_CME_PH_FSIM_PUK_REQUIRED,
	GSM0707_CME_SIM_NOT_INSERTED,
	GSM0707_CME_SIM_PIN_REQUIRED,
	GSM0707_CME_SIM_PUK_REQUIRED,
/*	GSM0707_CME_SIM_FAILURE,
	GSM0707_CME_SIM_BUSY,
	GSM0707_CME_SIM_WRONG,*/
	GSM0707_CME_SIM_PIN2_REQUIRED,
	GSM0707_CME_SIM_PUK2_REQUIRED,
/*	GSM0707_CME_MEMORY_FULL,
	GSM0707_CME_MEMORY_FAILURE,*/
	GSM0707_CME_NETPERS_PIN_REQUIRED,
	GSM0707_CME_NETPERS_PUK_REQUIRED,
	GSM0707_CME_NETSUBSET_PIN_REQUIRED,
	GSM0707_CME_NETSUBSET_PUK_REQUIRED,
	GSM0707_CME_PROVIDER_PIN_REQUIRED,
	GSM0707_CME_PROVIDER_PUK_REQUIRED,
	GSM0707_CME_CORPORATE_PIN_REQUIRED,
	GSM0707_CME_CORPORATE_PUK_REQUIRED,
};

static int is_in_array(unsigned int val, unsigned int *arr, unsigned int arr_len)
{
	unsigned int i;

	for (i = 0; i < arr_len; i++) {
		if (arr[i] == val)
			return 1;
	}

	return 0;
}

const int pintype_from_cme[GSM0707_CME_UNKNOWN] = {
	[GSM0707_CME_PH_SIM_PIN_REQUIRED] = GSMD_PIN_PH_SIM_PIN,
	[GSM0707_CME_PH_FSIM_PIN_REQUIRED] = GSMD_PIN_PH_FSIM_PIN,
	[GSM0707_CME_PH_FSIM_PUK_REQUIRED] = GSMD_PIN_PH_FSIM_PUK,
	[GSM0707_CME_SIM_PIN_REQUIRED] = GSMD_PIN_SIM_PIN,
	[GSM0707_CME_SIM_PUK_REQUIRED] = GSMD_PIN_SIM_PUK,
	[GSM0707_CME_SIM_PIN2_REQUIRED] = GSMD_PIN_SIM_PIN2,
	[GSM0707_CME_SIM_PUK2_REQUIRED] = GSMD_PIN_SIM_PUK2,
	[GSM0707_CME_NETPERS_PIN_REQUIRED] = GSMD_PIN_PH_NET_PIN,
	[GSM0707_CME_NETPERS_PUK_REQUIRED] = GSMD_PIN_PH_NET_PUK,
	[GSM0707_CME_NETSUBSET_PIN_REQUIRED] = GSMD_PIN_PH_NETSUB_PIN,
	[GSM0707_CME_NETSUBSET_PUK_REQUIRED] = GSMD_PIN_PH_NETSUB_PUK,
	[GSM0707_CME_PROVIDER_PIN_REQUIRED] = GSMD_PIN_PH_SP_PIN,
	[GSM0707_CME_PROVIDER_PUK_REQUIRED] = GSMD_PIN_PH_SP_PUK,
	[GSM0707_CME_CORPORATE_PIN_REQUIRED] = GSMD_PIN_PH_CORP_PIN,
	[GSM0707_CME_CORPORATE_PUK_REQUIRED] = GSMD_PIN_PH_CORP_PUK,
	/* FIXME: */
	[GSM0707_CME_SIM_FAILURE] = 0,
	[GSM0707_CME_SIM_BUSY] = 0,
	[GSM0707_CME_SIM_WRONG] = 0,
	[GSM0707_CME_MEMORY_FULL] = 0,
	[GSM0707_CME_MEMORY_FAILURE] = 0,
	[GSM0707_CME_PHONE_FAILURE] = 0,
	[GSM0707_CME_PHONE_NOCONNECT] = 0,
	[GSM0707_CME_PHONE_ADAPT_RESERVED] = 0,
	[GSM0707_CME_SIM_NOT_INSERTED] = 0,
};

int generate_event_from_cme(struct gsmd *g, unsigned int cme_error)
{
	if (!is_in_array(cme_error, errors_creating_events, ARRAY_SIZE(errors_creating_events))) {

		int cme = cme_error, cms = -1;
		return tbus_emit_signal("CMECMSError", "ii", &cme, &cms);
	} else {
		int pin_type = pintype_from_cme[cme_error];
		if (cme_error >= GSM0707_CME_UNKNOWN || !pin_type)
			return 0;

		return tbus_emit_signal("PINneeded", "i", &pin_type);
	}
}

int generate_event_from_cms(struct gsmd *g, unsigned int cms_error)
{
	int cme = -1, cms = cms_error;
	return tbus_emit_signal("CMECMSError", "ii", &cme, &cms);
}
