/* theme.h
*
*  Theme functions definion
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <flphone/libflphone.h>

#ifndef _theme_h_
#define _theme_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <nano-X.h>
#include <common/libini.h>
#include <flphone_config.h>

/* Applications layout 176x220 */
#if defined (CONFIG_176x220)
#define APPVIEW_STATUS_HEIGHT   20
#define APPVIEW_CONTROL_HEIGHT  20
#define CONFIG_SCREEN_WIDTH	176
#define CONFIG_SCREEN_HEIGHT	220
/* settings for the Icon grid */
#define GRID_ICON_WIDTH         48
#define GRID_ICON_HEIGHT        35

/* Applications layout 240x320 */
#elif defined (CONFIG_240x320)
#define APPVIEW_STATUS_HEIGHT   28
#define APPVIEW_CONTROL_HEIGHT  28
#define CONFIG_SCREEN_WIDTH	240
#define CONFIG_SCREEN_HEIGHT	320
/* settings for the Icon grid */
#define GRID_ICON_WIDTH         48
#define GRID_ICON_HEIGHT        35

/* Applications layout 480x640 */
#elif defined (CONFIG_480x640)
#define APPVIEW_STATUS_HEIGHT   48
#define APPVIEW_CONTROL_HEIGHT  48
#define CONFIG_SCREEN_WIDTH	480
#define CONFIG_SCREEN_HEIGHT	640
/* settings for the Icon grid */
#define GRID_ICON_WIDTH         48
#define GRID_ICON_HEIGHT        35

#endif

#define APPVIEW_WIDTH		CONFIG_SCREEN_WIDTH
#define APPVIEW_HEIGHT		( CONFIG_SCREEN_HEIGHT - APPVIEW_STATUS_HEIGHT )

#define APPVIEW_AREA_HEIGHT	( CONFIG_SCREEN_HEIGHT - APPVIEW_STATUS_HEIGHT - APPVIEW_CONTROL_HEIGHT )

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

/** Font size index */
#define THEME_FONT_NORMAL	0
#define THEME_FONT_BIG		1
#define THEME_FONT_SMALL	2
#define THEME_FONT_TINY		3

/** Load image from filename and return its ID */
DLLEXPORT GR_WINDOW_ID theme_load_image(char * filename);

/**
 * Load image defined with <group_index> and <image_index>
 * sets xcoord, ycoord and (GR_IMAGE_ID) pict_id (if contains image name)
 */
DLLEXPORT int theme_get_image(int group_index, int image_index, int * xcoord, int * ycoord, GR_IMAGE_ID * pict_id);

/** Get fontsize */
DLLEXPORT int theme_fontsize(int index);

#ifdef __cplusplus
}
#endif
#endif
