/* gsm.h
*
*  GSMD connection functions
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*
*/

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif
extern struct lgsm_handle *lgsmh;

void lgsm_handle(int gsm_fd, void * data);

int init_lgsm(void);

#ifdef __cplusplus
 }
#endif
