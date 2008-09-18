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

/**
 * Find specified file - in local folder then on MMC
 * @param filename filename with folder path like etc/nanowm.cfg
 * @return full path string in allocated buffer, remember to free it after use
 *  or NULL if some error happened
*/
char * cfg_findfile(const char * filename)
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

/**
 * Read string value from config file
 * @param config filename of config file
 * @param head Heading to search
 * @param key Key to read from
 * @return string value in allocated buffer, remember to free it after use
 *  or NULL if some error happened
*/
char *cfg_readString(const char *config, const char *head, const char *key)
{
	char str[129];
	char *cfg = cfg_findfile (config);
	if (cfg == NULL)
		return NULL;

	ini_fd_t fd = ini_open (cfg, "r", "#;");
	free(cfg);
	if (fd == NULL)
		return NULL;

	int ret = ini_locateHeading (fd, head);
	ret = ini_locateKey (fd, key);
	ret = ini_readString (fd, str, sizeof (str));
	ini_close(fd);
	
	if (ret < 0)
		return NULL;
	else
		return strdup(str);
}

/**
 * Read positive integer value from config file
 * @param config filename of config file
 * @param head Heading to search
 * @param key Key to read from
 * @return value, or -1 if error
 */
int cfg_readInt(const char *config, const char *head, const char *key)
{
	char *cfg = cfg_readString (config, head, key);

	if (cfg == NULL)
		return -1;

	return atoi(cfg);
}
