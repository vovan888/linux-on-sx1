/*
/* indicators.h
*
*  main module utils
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <nano-X.h>

#define INDICATOR_NETWORK	0
#define INDICATOR_BATTERY	1
#define INDICATOR_CLOCK		2
#define INDICATOR_ICONS		3
#define INDICATOR_OPNAME	4


struct indicatord {
	GR_WINDOW_ID wids[5];
};