/* gsmd vendor plugin core
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

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <common/linux_list.h>

#include "gsmd.h"

#include <gsmd/gsmd.h>
#include <gsmd/vendorplugin.h>

int gsmd_vendor_plugin_find(struct gsmd *g)
{
	g->vendorpl = &gsmd_vendor_plugin;
	g_slow->vendorpl = &gsmd_vendor_plugin;

	return 0;
}

