/* theme.c
*
* Theme handling functions
*
* Copyright 2007 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <common/libini.h>
#include <flphone/theme.h>
#include <flphone/libflphone.h>
#include <flphone_config.h>
#include <nano-X.h>

#ifdef CONFIG_PLATFORM_X86DEMO
#define ACTIVETHEME_PATH	"share/activetheme/"
#else
#define ACTIVETHEME_PATH	"share/activetheme/"
#endif

const char theme_title[] = "Title";
const char *theme_title_images[] = {"Battery","Signal","WindowCaption","Time","Indicators"};

const char theme_mainscreen[] = "MainScreen";
const char *theme_mainscreen_images[] = {"MainBattery","MainSignal", "Wallpaper","MissedCalls",
		"Messages","DateTime", "Operator", "Profile","Information","StatusIndicators"};

const char theme_dialer[] = "Dialer";
const char *theme_dialer_images[] = {"Background"};

const char theme_callscreen[] = "Callscreen";
const char *theme_callscreen_images[] = {"Background"};

static ini_fd_t theme_config_fd = NULL;	/* theme config file descr */

const char *theme_font_sizes[] = {"Normal", "Big", "Small", "Tiny"};
static int theme_font_sizes_cache[4];


static int load_theme_config(void)
{
	char buffer[300], *p = buffer;
	int ret;

	if (!theme_config_fd) {
		/*Theme config is not loaded yet*/
		memset(theme_font_sizes_cache, 0, sizeof(theme_font_sizes_cache));
		/* Open config file */
		char *filename = cfg_findfile ("etc/nanowm.cfg");
		if (!filename)
			return -1;

		ini_fd_t fd = ini_open (filename, "r", "#;");
		free (filename);

		if (fd == NULL) {
			fprintf (stderr, "theme_get: Unable to open nanowm.cfg file\n");
			return -1;
		} else {
			ret = ini_locateHeading (fd, "Main");
			ret = ini_locateKey (fd, "Theme");
			ret = ini_readString (fd, p, sizeof (buffer));
		}
		ini_close(fd);
		if (ret < 0)
			p = "theme";
		/* open theme config file */
		char path[256] =  ACTIVETHEME_PATH;

		strncat(path, p, 255);
		strncat(path, ".cfg", 255);
		filename = cfg_findfile (path);
		theme_config_fd = ini_open (filename, "r", "#;");
		free (filename);
		if (theme_config_fd == NULL) {
			fprintf (stderr, "theme_get: Unable to open theme config file\n");
			return -1;
		}

	}
	return 0;
}

/* example of theme config file string:
 * Name = <xposition>,<yposition>,<image filename>
 * Wallpaper=0,0,wallpaper.png
 */

/**
 * Load image from filename and return its ID
*/
GR_WINDOW_ID theme_load_image(char * filename)
{
	GR_GC_ID gc;
	GR_IMAGE_ID iid;
	GR_WINDOW_ID pid;
	GR_IMAGE_INFO iif;

	gc = GrNewGC();
	if(!(iid = GrLoadImageFromFile(filename, 0))) {
		fprintf(stderr, "Failed to load image file \"%s\"\n", filename);
		return 0;
	}
	GrGetImageInfo(iid, &iif);
	pid = GrNewPixmap(iif.width, iif.height, NULL);

	GrDrawImageToFit(pid, gc, 0, 0, iif.width, iif.height, iid);
	GrDestroyGC(gc);
	GrFreeImage(iid);

	return pid;
}

/**
 * Load specified theme file and unpack it to the active theme folder
 * theme file is ZIP packed
*/
int theme_load(char * theme_file)
{
	if (load_theme_config() < 0)
		return -1;
	return 0;
}

/**
 * Get specified image from the active theme
 *
*/
int theme_get_image(int group_index, int image_index, int * xcoord, int * ycoord, GR_IMAGE_ID * pict_id)
{
	char buffer[300], *p = buffer;

	if (load_theme_config() < 0)
		return -1;
	/* load needed image */
	const char * theme_group; /*  group name */
	const char ** theme_images;	/* pointer to the theme_*_images array */
	switch(group_index) {
		case THEME_GROUP_TITLE:
			theme_group 	= theme_title;
			theme_images	= theme_title_images;
			break;
		case THEME_GROUP_MAINSCREEN:
			theme_group 	= theme_mainscreen;
			theme_images	= theme_mainscreen_images;
			break;
		default:
			return -1;
	}
	if (ini_locateHeading (theme_config_fd, theme_group) ) {
		fprintf (stderr, "theme_get: Unable to find group %s\n",theme_group);
		return -1;
	}
	if (ini_locateKey (theme_config_fd, theme_images[image_index] ) < 0 ) {
		fprintf (stderr, "theme_get: Unable to find key %s\n",theme_images[image_index]);
		return -1;
	}
	if (ini_readString (theme_config_fd, p, sizeof (buffer))  < 0 ) {
		fprintf (stderr, "theme_get: Unable to get string from key %s\n",theme_images[image_index]);
		return -1;
	}
	/* now parse key value */
	char * token;
	token = strtok(p, ", ");
	/*token = xcoord */
	*xcoord = atoi(token);

	token = strtok(NULL, ", ");
	/*token = ycoord */
	*ycoord = atoi(token);

	token = strtok(NULL, ", ");
	/*token = imagename */

	if (token) {
		/* we have found image name, now load it */
		char path[256] =  ACTIVETHEME_PATH;
		strncat(path, token, 255);
		char *filename = cfg_findfile (path);
		if(!filename) {
			fprintf (stderr, "theme_get: Unable to find image %s\n",path);
		} else {
	//		*wid = theme_load_image(filename);
			*pict_id = GrLoadImageFromFile(filename, 0);
			if (*pict_id == 0)
				fprintf(stderr, "theme_get: Failed to load image %s\n", filename);
			free(filename);
		}
	} else {
		*pict_id = 0;
	}
	return (*pict_id == 0)?-2:0;
}

/**
 * Get font size from theme config
 */
int theme_fontsize(int index)
{
	int ret, fontsize;

	if (index < 0 || index > 3)
		return -1;
	if (load_theme_config() < 0)
		return -1;

	if (theme_font_sizes_cache[index])
		return theme_font_sizes_cache[index];

	ret = ini_locateHeading (theme_config_fd, "FontSize");
	ret |= ini_locateKey (theme_config_fd, theme_font_sizes[index]);
	ret |= ini_readInt(theme_config_fd, &fontsize);
	if (ret < 0)
		return -1;
	else {
		theme_font_sizes_cache[index] = fontsize;
		return fontsize;
	}
}
