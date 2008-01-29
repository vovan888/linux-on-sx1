/*
 * Shared memory segment macros
 *
 * Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
 *
 * Licensed under GPLv2 
*/

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 

#include <debug.h>
#include <ipc/shareddata.h>

#define SHMID		0x0888
#define SEMID		0x8888
#define SHMSIZE		4096

/* arg for semctl system calls. */
union semun {
        int val;                        /* value for SETVAL */
        struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
        unsigned short *array;  /* array for GETALL & SETALL */
        struct seminfo *__buf;  /* buffer for IPC_INFO */
        void *__pad;
};

static int	shmid;
static int	shmids[SHARED_NUMSEGMENTS]; /* Shared memory segment IDs */
/* semaphores stuff */
static int	semid;
static int	semids[SHARED_NUMSEGMENTS];
const static struct sembuf oper_lock[1] = {{
	.sem_num	= 0,
	.sem_op		= -1,
	.sem_flg	= SEM_UNDO,
	}};
const static struct sembuf oper_unlock[1] = {{
	.sem_num	= 0,
	.sem_op		= 1,
	.sem_flg	= SEM_UNDO,
	}};

static int ShmInit(unsigned int shared_id)
{
	/* allocate shared memory segment */
	shmid = shmget(SHMID + shared_id, SHMSIZE, IPC_CREAT | S_IRUSR | S_IWUSR );
	if (shmid < 0) {
		ERRLOG("Cant create shm object.\n");
	}

	/* allocate semaphore */
	semid = semget(SEMID+shared_id, 1, IPC_CREAT | S_IRUSR | S_IWUSR );
	if (semid < 0) {
		ERRLOG("Cant create sem object.\n");
	} else {
		/* init semaphore */
		union semun	arg;
		unsigned short vals[1];
		vals[0] = 1;
		arg.array = vals;
		semctl(semid, 0, SETALL, arg);
	}
	return 0;
}

/* Map shared memory segment with shared_id
 * returns its adress
*/
struct SharedSystem *ShmMap(unsigned int shared_id)
{
	void * ptr = NULL;

	/* check shared_id */
	if (shared_id > SHARED_NUMSEGMENTS) {
		return NULL;
	}

	shmid = shmids[shared_id];
	semid = semids[shared_id];

	if (! shmid )
		ShmInit(shared_id);
	if ( shmid ) {
		ptr = shmat(shmid, NULL, 0);
		if (ptr == (void *) -1 ) {
			ERRLOG("Cant shmat segment\n");
		}
	}
	
	shmids[shared_id] = shmid;
	semids[shared_id] = semid;

	return (struct SharedSystem *) ptr;
}

/* UnMap shared memory segment
 * returns 
*/
int	ShmUnmap(struct SharedSystem *ptr)
{
	/*FIXME should we free segments and semaphores ?*/
	return shmdt(ptr);
}

/* blocks till the shared mem is busy then locks it */
int ShmLock (unsigned int shared_id)
{
	if(shmids[shared_id])
		return semop(semids[shared_id], oper_lock, 1);
	else
		return 0;
}

/* unlocks shared memory segment */
int ShmUnlock (unsigned int shared_id)
{
	if(shmids[shared_id])
		return semop(semids[shared_id], oper_unlock, 1);
	else
		return 0;
}