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
#define SHMSIZE		4096

/* arg for semctl system calls. */
union semun {
        int val;                        /* value for SETVAL */
        struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
        unsigned short *array;  /* array for GETALL & SETALL */
        struct seminfo *__buf;  /* buffer for IPC_INFO */
        void *__pad;
};

static int	use_count = 0;
static int	shmid = 0; /* Shared memory segment ID */
/* semaphores stuff */
static int	semid = 0;
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

static int ShmInit()
{
	if(!shmid) {
		/* allocate shared memory segment */
		shmid = shmget(SHMID, SHMSIZE, IPC_CREAT | S_IRUSR | S_IWUSR );
		if(shmid < 0) {
			ERRLOG("Cant create shm object.\n");
		}
	}
	if(!semid) {
		/* allocate semaphore */
		semid = semget(SHMID, 1, S_IRUSR | S_IWUSR );
		if(semid < 0) {
			ERRLOG("Cant create sem object.\n");
		} else {
			/* init semaphore */
			union semun	arg;
			unsigned short vals[1];
			vals[0] = 1;
			arg.array = vals;
			semctl(semid, 0, SETALL, arg);
		}
	}
	return shmid;
}

SharedData * ShmMap()
{
	void * ptr = NULL;
	if(!shmid)
		ShmInit();
	if(shmid) {
		ptr = shmat(SHMID, NULL, 0);
		if(ptr == (void *) -1 ) {
			ERRLOG("Cant shmat segment");
		} else {
			use_count++;
		}
	}
	return (SharedData *) ptr;
}

int	ShmUnmap(SharedData * ptr)
{
	if(shmid) {
		shmdt(ptr);
		use_count--;
		if (use_count == 0) {
			/* remove semaphore */
			union semun	arg;
			semctl(semid, 1, IPC_RMID, arg);
		}
	}
	return 0;
}

/* blocks till the shared mem is busy them locks it */
int ShmLock ()
{
	if(shmid)
		return semop(semid, oper_lock, 1);
	else
		return 0;
}

/* unlocks shared memory segment */
int ShmUnlock ()
{
	if(shmid)
		return semop(semid, oper_unlock, 1);
	else
		return 0;
}
