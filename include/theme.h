/* theme.h
*
*  Theme functions definion
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <libflphone.h>

#ifndef _theme_h_
#define _theme_h_

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <nano-X.h>
#include <libini.h>
#include <flphone_config.h>

/* Applications layout */
#define APPVIEW_WIDTH           CONFIG_SCREEN_WIDTH

#define APPVIEW_STATUS_HEIGHT   20
#define APPVIEW_CONTROL_HEIGHT  20
#define APPVIEW_AREA_HEIGHT     ( CONFIG_SCREEN_HEIGHT - APPVIEW_STATUS_HEIGHT - APPVIEW_CONTROL_HEIGHT )
/* settings for the Icon grid */
#define GRID_ICON_WIDTH         48
#define GRID_ICON_HEIGHT        35

/* Theme groups */
#define THEME_GROUP_TITLE	0
#define THEME_GROUP_MAINSCREEN	1
#define THEME_GROUP_DIALER	2
#define THEME_GROUP_CALLSCREEN	3

/* Title group */
#define THEME_BATTERY		0
#define THEME_SIGNAL		1
#define THEME_WINDOWCAPTION	2
#define THEME_TIME		3
#define THEME_INDICATORS	4

/* MainScreen group */
#define THEME_MAINBATTERY	0
#define THEME_MAINSIGNAL	1
#define THEME_WALLPAPER		2
#define THEME_MISSEDCALLS	3
#define THEME_MESSAGES		4
#define THEME_DATETIME		5
#define THEME_OPERATOR		6
#define THEME_PROFILE		7
#define THEME_INFORMATION	8
#define THEME_STATUSINDICATORS	9

/* Dialer */

/* Callscreen */

/* overall number of images in theme */
#define THEME_MAXIMAGES		16

/* maximum images in multi image indicator */
#define N_MAX_IMAGES		8
/*
 * Load image from filename and return its ID
*/
DLLEXPORT GR_WINDOW_ID theme_load_image(char * filename);

/*
 * Load image defined with <group_index> and <image_index>
 * sets xcoord, ycoord and (GR_IMAGE_ID) pict_id (if contains image name)
 */
DLLEXPORT int theme_get_image(int group_index, int image_index, int * xcoord, int * ycoord, GR_IMAGE_ID * pict_id);

#endif
