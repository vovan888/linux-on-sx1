/* libflphone.h
*
* libflphone library
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/

#include <flphone_config.h>

#ifndef _libflphone_h_
#define _libflphone_h_


/* -fvisibility=hidden support macro */
#ifdef CONFIG_GCC_HIDDEN_VISIBILITY
    #define DLLEXPORT __attribute__ ((visibility("default")))
    #define DLLLOCAL __attribute__ ((visibility("hidden")))
#else
    #define DLLEXPORT
    #define DLLLOCAL
#endif

/* config functions */
/* find file on MMC then in local dir */
DLLEXPORT char * cfg_findfile(char * filename);

#endif
