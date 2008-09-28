/* flphone.c
*
* Different functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <flphone/libflphone.h>

/**
 * Spawn an application
 * @param filename path to the program to spawn
 * @param arg string to pass to the program as argv[1]
 * @return pid of the new process or -1 if error
 */
int fl_spawn(const char *filename, const char *arg)
{

	int ret;
	const char *argv[3];

	if ( (filename == NULL) || (arg == NULL) )
		return -1;

	argv[0] = filename;
	argv[1] = arg;
	argv[2] = 0;

	ret = vfork();
	if (!ret) {
		execvp(filename, argv);
		exit(1);
	}

	return ret;
}
