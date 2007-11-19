/* theme.c
*
* Theme handling functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libini.h>
#include <flphone_config.h>

#ifdef CONFIG_PLATFORM_X86DEMO
const char *active_theme_path = "/usr/local/flphone/data/activetheme";
#else
const char *active_theme_path = "/home/flphone/.config/activetheme";
#endif

const char theme_title[] = "Title";
const char *theme_title_images[] = {"Battery","Signal","WindowCaption","Time","Indicators"};

const char theme_mainscreen[] = "MainScreen";
const char *theme_mainscreen_images[] = {"Wallpaper","MissedCalls","Messages","DateTime","Operator",
		"Profile","Information","StatusIndicators"};

const char theme_dialer[] = "Dialer";
const char *theme_dialer_images[] = {};

const char theme_callscreen[] = "Callscreen";
const char *theme_callscreen_images[] = {};

/* example of theme config file string:
 * Name = <xposition>,<yposition>,<image filename>
 * Wallpaper=0,0,wallpaper.png
 */

/*
 * Load specified theme file and unpack it to the active theme folder
 * theme file is ZIP packed
*/
int theme_load(char * theme_file)
{
}

/*
 * Get specified image from the active theme
 *
*/
int theme_get(int image_index)
{
	
}
