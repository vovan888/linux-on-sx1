/* config.c
*
* Config files handling functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libini.h>
#include <flphone_config.h>
#include <libflphone.h>

#define PATHS_NUM	2
#ifdef CONFIG_PLATFORM_X86DEMO
const char *paths[PATHS_NUM] = {"/usr/local/flphone/", NULL };
#else
const char *paths[PATHS_NUM] = {"/usr/flphone/","/mnt/mmc1/" };
#endif

/*
 * Find specified file - on MMC then in local folder
 * return: NULL - not found, or  pointer to full path string
 * do not forget to dealloc it after use
*/
char * cfg_findfile(char * filename)
{
	int i = PATHS_NUM;
	char str[256];	/*FIXME - constant ?*/
	
	while ( i-- >= 0) {
		if (paths[i] == NULL)
			continue;
		strcpy(str, paths[i]);
		strncat(str, filename, 255);
		if (access(str, F_OK))
			continue; /* file not found */
		/* file exists, return pointer to its full path*/
		return strdup(str);
	}
	return NULL;
}
