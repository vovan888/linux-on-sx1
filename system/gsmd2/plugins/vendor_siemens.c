/* Siemens x55 modems
 *
 * (C) 2008 by Vladimir Ananiev <vovan888 at gmail com>
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
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "../gsmd.h"

#include <gsmd/gsmd.h>
#include <gsmd/usock.h>
#include <gsmd/event.h>
#include <gsmd/talloc.h>
#include <gsmd/extrsp.h>
#include <gsmd/atcmd.h>
#include <gsmd/vendorplugin.h>
#include <gsmd/unsolicited.h>

static int scii_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	char *tok1, *tok2;
	char tx_buf[20];

	strlcpy(tx_buf, buf, sizeof(tx_buf));
	tok1 = strtok(tx_buf, ",");
	if (!tok1)
		return -EIO;

	tok2 = strtok(NULL, ",");
	if (!tok2) {
		switch (atoi(tok1)) {
		case 0:
			gsmd->dev_state.ciph_ind.flags &= ~GSMD_CIPHIND_ACTIVE;
			break;
		case 1:
			gsmd->dev_state.ciph_ind.flags |= GSMD_CIPHIND_ACTIVE;
			break;
		case 2:
			gsmd->dev_state.ciph_ind.flags |= GSMD_CIPHIND_DISABLED_SIM;
			break;
		}
	} else {
// 		struct gsmd_evt_auxdata *aux;
//
// 		aux->u.cipher.net_state_gsm = atoi(tok1);
// 		aux->u.cipher.net_state_gsm = atoi(tok2);

//		usock_evt_send(gsmd, ucmd, GSMD_EVT_CIPHER);
	}

	return 0;
}

static int scbi_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
//	char *tok1, *tok2;
//	char tx_buf[20];

// 	strlcpy(tx_buf, param, sizeof(tx_buf));
// 	tok1 = strtok(tx_buf, ",");
// 	if (!tok1)
// 		return -EIO;
//
// 	tok2 = strtok(NULL, ",");
// 	if (!tok2) {
// 		switch (atoi(tok2)) {
// 		}
// 	}

	return 0;
}

static int sacd_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	char *tok, *name;
	char tx_buf[20];

	strlcpy(tx_buf, param, sizeof(tx_buf));
	tok = strtok(tx_buf, ",");
	if (!tok)
		return -EIO;
	tok = strtok(NULL, ",");
	if (!tok)
		return -EIO;
	int device = atoi(tok);

	tok = strtok(NULL, ",");
	if (!tok)
		return -EIO;
	int event = atoi(tok);

	switch (device) {
		case 1:	/* COM cable (x55 serial cable)*/
			name = "COM";
			break;
		case 15: /* USB cable */
			name = "USB";
			break;
		case 31: /* Stereo Headset */
			name = "Headset";
			break;
	}

	tbus_emit_signal("AccessoryConnected","si", &name, &event);

	return 0;
}

static int to_gsmd_call_progress[] = {
	GSMD_CALL_IDLE,		/* 10 */
	GSMD_CALL_DIALING,	/* 11 */
	GSMD_CALL_RINGING,	/* 12 */
	GSMD_CALL_RINGING,	/* 13 */
	GSMD_CALL_CONNECTED,	/* 14 */
	-1,	/* 15 FIXME*/
	-1,	/* 16 */
	-1,	/* 17 */
	-1,	/* 18 */
	-1,	/* 19 */
	-1,	/* 20 */
	-1,	/* 21 */
	-1,	/* 22 */
	-1,	/* 23 */
};

/*
+CIND:
("battchg",(0-5)),("signal",(0-5)),("service",(0,1)),("message",(0,1)),
("call",(0,1)),("roam",(0,1)),("smsfull",(0,1)),
("call status",(10x-20x,31x,33x,34x,51x,53x,54x)),
("GPRS coverage",(0,1))
*/
static int ciev_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	char *tok1, *tok2;
	char tx_buf[20];
	int value, ret, call_status, call_id;

	strlcpy(tx_buf, param, sizeof(tx_buf));
	tok1 = strtok(tx_buf, ",");
	if (!tok1)
		return -EIO;

	tok2 = strtok(NULL, ",");
	value = atoi(tok2);
	switch (atoi(tok1)) {
		case 1: /* battery charge level 0..5*/
			if (gsmd->shmem)
				gsmd->shmem->Battery.ChargeLevel = value;
			ret = tbus_emit_signal("BatteryCharge","i", &value);
			break;
		case 2: /* signal quality  0..5*/
			if (gsmd->shmem)
				gsmd->shmem->PhoneServer.Network_Signal = value;
			ret = tbus_emit_signal("Signal","i", &value);
			break;
		case 3: /* service 1-available, 0-not available*/
			if (gsmd->shmem)
				gsmd->shmem->PhoneServer.Network_Service_Avail = value;
			ret = tbus_emit_signal("ServiceAvailable","i", &value);
			break;
		case 4: /*  1- message received, 0 - not received */
			/*TODO*/
			break;
		case 5: /*  1 - call in progress, 0 - call not in progress */
			if (gsmd->shmem)
				gsmd->shmem->PhoneServer.Call_InProgress = value;
			ret = tbus_emit_signal("CallInProgess","i", &value);
			break;
		case 6: /*  1 - roaming, 0 - home network */
			/*TODO*/
			break;
		case 7: /*  1 - SMS storage memory is full, 0 - OK */
			/*TODO*/
			break;
		case 8: /*  call status ...*/
			call_status = value / 10;
			call_id = value - call_status*10;
			// convert to enum gsmd_call_progress
			call_status = to_gsmd_call_progress[call_status - 10];

			ret = tbus_emit_signal("CallProgress","ii", &call_status, &call_id);
			break;
		case 9: /* 1 - GPRS coverage available, 0 - no GPRS*/
			if (gsmd->shmem)
				gsmd->shmem->PhoneServer.GPRS_CoverageAvailable = value;
			ret = tbus_emit_signal("GPRScoverage","i", &value);
			break;
		case 10: /*  call setup */
			/*TODO*/
			break;
	}

	return 0;
}

static const struct gsmd_unsolicit siemens_unsolicit[] = {
	{ "+CIEV",	&ciev_parse },	/* Indicators event reporting */
	{ "^SACD",	&sacd_parse },	/* Accessory Indication */
	{ "^SCBI",	&scbi_parse },	/* CCBS feature available */
	{ "^SCII",	&scii_parse },	/* Ciphering Indication */
};

static int siemens_detect(struct gsmd *g)
{
	/* FIXME: do actual detection of vendor if we have multiple vendors */
	return 1;
}

static int siemens_initsettings(struct gsmd *g)
{
	int rc = 0;
	/* reset the second mux channel to factory defaults */
	rc |= gsmd_simplecmd(g, "AT&F2");
	/* setup Mobile error reporting, mode=3 - buffer unsolic messages in TE
	 1 - indicator event reporting using result code +CIEV: */
	rc |= gsmd_simplecmd(g, "AT+CMER=3,0,0,1,0");
	/* ignore DTR line (on mux0! - channel 1)*/
//	rc |= gsmd_simplecmd(g, "AT&D0");
	/* enable ^SACD: Accessory Indicators */
	rc |= gsmd_simplecmd(g, "AT^SACD=3");

	return rc;
}

static int siemens_initsettings_slow(struct gsmd *g)
{
	int rc = 0;
	/* reset the mux 3 channel to factory defaults */
	rc |= gsmd_simplecmd(g, "AT&F3");

	return rc;
}

static int siemens_initsettings_after_pin(struct gsmd *g)
{
	/* AT^SCII=  enable ^SCII: ciphering reporting */

	/*TODO - channel 2 - fast commands
	AT+CUSD=1	-> OK	(USSD?)
	AT+COPS=3,0;+COPS? -> +COPS: 0,0,"004D0065006700610046006F006E"
	AT+COPS=3,2;+COPS? -> +COPS: 0,2,"25002"
	AT+CSCB=0,"",""	-> OK (disable CB Msgs)
	*/

	/*TODO - channel 3 - slow commands
	AT^SPIC="PS"	-> ^SPIC: 3,10	- PIN counter
	AT+CRSM=192,28599 -> +CRSM: 148,4
	AT^SSPI -> ^SSPI: "004D0065006700610046006F006E","25002"	(MegaFon, 250 02)

	*/

	return 0;
}

struct gsmd_vendor_plugin gsmd_vendor_plugin = {
	.name = "Siemens x55",
	.ext_chars = "^",
	.num_unsolicit = ARRAY_SIZE(siemens_unsolicit),
	.unsolicit = siemens_unsolicit,
	.detect = &siemens_detect,
	.initsettings = &siemens_initsettings,
	.initsettings_slow = &siemens_initsettings_slow,
	.initsettings_after_pin = &siemens_initsettings_after_pin,
};
