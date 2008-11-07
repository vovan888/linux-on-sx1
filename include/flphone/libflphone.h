/* libflphone.h
*
* libflphone library
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/

#ifndef _libflphone_h_
#define _libflphone_h_

#ifdef __cplusplus
extern "C" {
#endif
#include <flphone_config.h>


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
DLLEXPORT char *cfg_findfile(const char *filename);

DLLEXPORT char *cfg_readString(const char *config, const char *head, const char *key);

DLLEXPORT int cfg_readInt(const char *config, const char *head, const char *key);

/* flphone.c */
DLLEXPORT int fl_spawn(const char *filename, const char *arg);

#ifdef __cplusplus
}
#endif
#endif
