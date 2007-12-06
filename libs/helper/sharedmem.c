/*
 * Shared memory segment macros
 *
 * Licensed under GPLv2 
*/

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 

#include <debug.h>
#include <ipc/shareddata.h>

#define SHMID		0x0888
#define SHMSIZE		4096

static int	shmid = 0; /* Shared memory segment ID */

static int ShmInit()
{
	if(!shmid) {
		shmid = shmget(SHMID, SHMSIZE, IPC_CREAT | IPC_EXCL | );
		if(shmid < 0) {
			ERRLOG("Cant create shm object.\n");
		}
	}
	return shmid;
}

struct SharedData * ShmMap()
{
	void * ptr;
	if(!shmid)
		ShmInit();
	if(shmid) {
		ptr = shmat(SHMID, NULL, );
		if(ptr == (void *) -1 ) {
			ERRLOG("Cant shmat segment");
		}
	}
	return (struct SharedData *) ptr;
}
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