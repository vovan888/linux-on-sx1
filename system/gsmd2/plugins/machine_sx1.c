/* Siemens SX1 machine plugin
 *
 * (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
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
#include <sys/ioctl.h>

#include "../gsmd.h"

#include <gsmd/gsmd.h>
#include <gsmd/usock.h>
#include <gsmd/event.h>
#include <gsmd/talloc.h>
#include <gsmd/extrsp.h>
#include <gsmd/machineplugin.h>
#include <gsmd/atcmd.h>

#define GSMD_MODEM_POWEROFF_TIMEOUT 6

static int sx1_detect(struct gsmd *g)
{
	return 1; /* not yet implemented */
}

static int sx1_init(struct gsmd *g, int fd)
{
    /* Modem is always powered on on SX1. Check EgoldWakeUp */
	g->interpreter_ready = 1;
	g_slow->interpreter_ready = 1;

	return 0;
}

static int sx1_power(struct gsmd *g, int power)
{
	switch (power) {
		case GSMD_MODEM_POWERUP:
			break;

		case GSMD_MODEM_POWERDOWN:
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

struct gsmd_machine_plugin gsmd_machine_plugin = {
	.name = "Siemens SX1",
	.power = sx1_power,
	.ex_submit = NULL,
	.detect = &sx1_detect,
	.init = &sx1_init,
};
