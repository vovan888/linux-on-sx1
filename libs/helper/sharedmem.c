/*
 * Shared memory segment macros
 *
 * Licensed under GPLv2 
*/

#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 

#if 0

typedef struct {
	
} shm_data_t;

int ShmInit()
{
	if((shm_fd = shm_open(tok, (O_CREAT|O_EXCL|O_RDWR), (S_IREAD|S_IWRITE))) > 0 )
	{
		*creator = 1;
	}
	else if((shm_fd = shm_open(tok, (O_CREAT|O_RDWR), (S_IREAD|S_IWRITE))) < 0) 
	{
		DBGLOG("Could not create shm object.\n");
		return NULL;
	}
}

#endif