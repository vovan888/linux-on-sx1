/*
/* interface.c
*
*  Graphics interface utils
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/
#include <flphone_config.h>

#include <stdio.h>
#include <stdarg.h>

#include <nano-X.h>
#include "indocators.h"

int interface_create_netsignal(struct indicatord * ind)
{
	ind->wids[INDICATOR_BATTERY] = 
}
/* create interface view */
int interface_create( struct indicatord * ind)
{
	interface_create_netsignal(ind);
/*	interface_create_battery();
	interface_create_clock();
	interface_create_icons();
	interface_create_opname();*/
}