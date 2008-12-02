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

#include <stdio.h>
#include <sqlite3.h>
#include <flphone/libflphone.h>
#include <flphone/libpim.h>

static sqlite3 *pim_db = NULL;

/** Open connection to PIM database */
int pim_init()
{
	int rc;

	if(pim_db != NULL)
		return 0;

	char *pim_db_filename = cfg_findfile("share/pim/pim.sqlite"); /*FIXME*/
	if(pim_db_filename == NULL) {
		// there is no database, we should create a new one
		//pim_create_default_db();	/*TODO*/
	} else {
		rc = sqlite3_open(pim_db_filename, &pim_db);
		if(rc) {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(pim_db));
			sqlite3_close(pim_db);
			return -1;
		}
	}
	return 0;
}

/** Close connection to PIM database */
int pim_close()
{
	return sqlite3_close(pim_db);
}

/** Get contact list, names filtered with filter */
int pim_get_contacts(const char *filter, char ***azResult, int *pnRow)
{
	char *zErrMsg = 0;
	char str_sql[256];
	int rc;

	snprintf(str_sql, 255, "SELECT contact_id, name "
				 "FROM contacts "
				 "WHERE name like \"%%%s%%\"", filter);

	rc = sqlite3_get_table(pim_db, str_sql, azResult, pnRow, 0, &zErrMsg);

	return rc;
}

/** Free result of Get methods */
void pim_free_result(char **azResult)
{
	sqlite3_free_table(azResult);
}
