/********************************************************************
* Description:
* Author:  Vladimir Ananiev
*
* Copyright (c) 2008 Vladimir Ananiev  All rights reserved.
*
********************************************************************/
/*
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
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _libpim_h_
#define _libpim_h_

#ifdef __cplusplus
extern "C" {
#endif

/** Open connection to PIM database*/
DLLEXPORT int pim_init();
/** Close connection to PIM database*/
DLLEXPORT int pim_close();
/** Close connection to PIM database*/
DLLEXPORT int pim_get_contacts(const char *filter, char ***pazResult, int *pnRow);
/** Free result of Get methods */
void pim_free_result(char **azResult);

#ifdef __cplusplus
}
#endif

#endif
