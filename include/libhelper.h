/* libhelper.h
*
* Helper library
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/

#ifndef _libhelper_h_
#define _libhelper_h_

/* config functions */
/* find file on MMC then in local dir */
char * cfg_findfile(char * filename);

/* sharedmem functions */
/* create sharedmem segment */
int ShmInit();
/* map sharedmem segment and return pointer to structure */
shm_segment_t * ShmMap ();
/* unmap shredmem segment */
int ShmUnmap(shm_segment_t * shm_seg)
/* destroy shared segment */
int ShmDestroy();

#endif
