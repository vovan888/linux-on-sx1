/* apps.c
*
*  Applications management
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <flphone/libflphone.h>

#define PATHS_NUM	3
extern char *paths[];

/** Run specified application
 * @param appname application internal name (name of its folder)
 * @param arg arguments string for the applications
 * @return pid of the running program, or -1 if error
 */
int app_run(const char *appname, const char *arg)
{
	char str[256];
	char *app_dir;
	char *app_exec;
	int ret = 0;

	str[0] = 0;
	strcpy(str, "apps/");
	strncat(str, appname, 255);

	app_dir = cfg_findfile(str);
	if (app_dir) {
		strcpy(str, app_dir);
		strncat(str, "/", 255);
		strncat(str, appname, 255);
		strncat(str, ".desktop", 255);

		app_exec = cfg_readString(str, "Desktop Entry", "Exec");
		strcpy(str, app_dir);
		strncat(str, "/", 255);
		strncat(str, app_exec, 255);

		ret = fl_spawn(str, arg);

		free(app_dir);
	}

	return ret;
}

static void add_to_database(const char *path, const char *itemname)
{
	char str[256];	/*FIXME - constant ?*/
	struct stat file_attr;

	str[0] = 0;
	strcpy(str, path);
	strncat(str, itemname, 255);

	stat(str, &file_attr);
	if(!S_ISDIR(file_attr.st_mode))
		return; /* not a directory */
	/*TODO*/
}

/** refresh apps database
 is called when SD card inserted, app installed, dnotify on app dir
 * @param hint tells what part of filesystem changed
*/
int app_refresh_database(enum refresh_hint hint)
{
	int i;
	DIR *dp;
	struct dirent *ep;
	int num_applications = 0;

	for(i = 0; i < PATHS_NUM; i++) {
		if (paths[i] == NULL)
			continue;

		dp = opendir(paths[i]);
		if (dp != NULL) {
			while (ep = readdir (dp)) {
				add_to_database(paths[i], ep->d_name);
				num_applications++;
			};
			closedir(dp);
		}
	}

	return 0;
}
