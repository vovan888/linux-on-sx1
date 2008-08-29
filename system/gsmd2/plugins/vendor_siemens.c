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
		struct gsmd_evt_auxdata *aux;
		struct gsmd_ucmd *ucmd = usock_build_event(GSMD_MSG_EVENT,
							   GSMD_EVT_CIPHER,
							   sizeof(*aux));
		if (!ucmd)
			return -ENOMEM;

		aux = (struct gsmd_evt_auxdata *) ucmd->buf;

		aux->u.cipher.net_state_gsm = atoi(tok1);
		aux->u.cipher.net_state_gsm = atoi(tok2);

		usock_evt_send(gsmd, ucmd, GSMD_EVT_CIPHER);
	}

	return 0;
}

static int sacd_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	char *tok1, *tok2;
	char tx_buf[20];
	
	strlcpy(tx_buf, buf, sizeof(tx_buf));
	tok1 = strtok(tx_buf, ",");
	if (!tok1)
		return -EIO;
	
	tok2 = strtok(NULL, ",");
	if (!tok2) {
		switch (atoi(tok2)) {
		case 1:	/* COM cable (x55 serial cable)*/
			/*TODO*/
			break;
		case 15: /* USB cable */
			/*TODO*/
			break;
		case 31: /* Headset */
			/*TODO*/
			break;
		}
	}

	return 0;
}

/*  +CIEV ind,value.  indicator:
1 = battery charge (0..10)
2 = signal quality (0 - good ..7 - bad, 99 - NA)
3 = service
4 = message
5 = call
6 = roam
7 = smsfull
*/
static int ciev_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	char *tok1, *tok2;
	char tx_buf[20];
	
	strlcpy(tx_buf, buf, sizeof(tx_buf));
	tok1 = strtok(tx_buf, ",");
	if (!tok1)
		return -EIO;
	
	switch (atoi(tok1)) {
		case 1: /* battery charge */
			/*TODO*/
			break;
	}

	return 0;
}

static const struct gsmd_unsolicit siemens_unsolicit[] = {
	{ "+CIEV",	&ciev_parse },	/* Indicators event reporting */
	{ "^SCII",	&scii_parse },	/* Ciphering Indication */
	{ "^SACD",	&sacd_parse },	/* Accessory Indication */
};

static int siemens_detect(struct gsmd *g)
{
	/* FIXME: do actual detection of vendor if we have multiple vendors */
	return 1;
}

static int siemens_initsettings(struct gsmd *g)
{
	int rc = 0;
	struct gsmd_atcmd *cmd;

	/* reset the second mux channel to factory defaults */
//	rc |= gsmd_simplecmd(g, "AT&F2");
	/* ignore DTR line (on mux1)*/
//	rc |= gsmd_simplecmd(g, "AT&D0");
	/* setup Mobile error reporting, mode=3,
	 indicator event reporting using result code +CIEV:
	*/
	rc |= gsmd_simplecmd(g, "AT+CMER=3,0,0,1,0");
	/* enable ^SACD: Accessory Indicators */
	rc |= gsmd_simplecmd(g, "AT^SACD=3");
	
	/* AT^SCII=  enable ^SCII: ciphering reporting */

	return rc;
}

struct gsmd_vendor_plugin gsmd_vendor_plugin = {
	.name = "Siemens x55",
	.ext_chars = "^",
	.num_unsolicit = ARRAY_SIZE(siemens_unsolicit),
	.unsolicit = siemens_unsolicit,
	.detect = &siemens_detect,
	.initsettings = &siemens_initsettings,
};
