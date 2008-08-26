/* config.c
*
* Config files handling functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <common/libini.h>
#include <flphone_config.h>
#include <flphone/libflphone.h>

#define PATHS_NUM	2
#ifdef CONFIG_PLATFORM_X86DEMO
const char *paths[PATHS_NUM] = {"./", NULL };
#else
const char *paths[PATHS_NUM] = {"/mnt/mmc1/","/usr/flphone/" };
#endif

/*
 * Find specified file - on MMC then in local folder
 * return: NULL - not found, or  pointer to full path string
 * do not forget to dealloc it after use
*/
char * cfg_findfile(char * filename)
{
	int i;
	char str[256];	/*FIXME - constant ?*/
	
	for(i = 0; i < PATHS_NUM; i++) {
		if (paths[i] == NULL)
			continue;
		str[0] = 0;	/*FIXME empty string*/
		strcpy(str, paths[i]);
		strncat(str, filename, 255);
		if (access(str, F_OK))
			continue; /* file not found */
		/* file exists, return pointer to its full path*/
		return strdup(str);
	}
	return NULL;
}
