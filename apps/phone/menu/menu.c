/***************************************************************************
 *   Copyright (C) 2008 by Denys Gatsenko   *
 *   orb1t@orb1t   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <dirent.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <nano-X.h>
#include <theme.h>

#include "menu.h"

struct add_nfo
{
	int num; //number of fields
	char *nfo[MAX_ARGUMENTS];
};
typedef struct add_nfo AINFO;

void *my_malloc(size_t size)
{
	void *ret;

	if(!(ret = malloc(size))) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	return ret;
}

//---testing purpose only!!! should be removed once debuged!!!

#ifdef CONFIG_PLATFORM_X86DEMO
#define ACTIVETHEME_PATH	"share/activetheme/"
//const char *active_theme_path = "/usr/local/flphone/share/activetheme";
#else
#define ACTIVETHEME_PATH	"share/activetheme/"
//const char *active_theme_path = "/home/flphone/.config/activetheme";
#endif

//#define ACTIVETHEME_PATH	"/home/orb1t/work/linux-on-sx1/host/flphone/apps/phone/menu/share/activetheme/"

#define GROUP_MENU	0

#define GROUP_MAIN_MENU	0
#define GROUP_TEST_MENU	1

#define MENU_ITEM_TYPE_PROG 0
#define MENU_ITEM_TYPE_MENU 1

#define MAX_MENU_ITEMS 32

#include <nxcolors.h>

const char menu_groups[] = "MainMenu";
const char *menu_groups_items[] = {"LandMine","Screen saver","Tetris","nxTerm","Screenshot","Tux","World"};

static ini_fd_t menu_config_fd = NULL;	/* theme config file descr */

int ttheme_load_item(int menu_group_index, int item_index, MITEM *item ){
	char buffer[300], *p = buffer;

	if (!menu_config_fd) {
		/*Theme config is not loaded yet*/
		/* Open config file */
		char * filename = cfg_findfile ("etc/nanowm.cfg");
		if (!filename) {
			fprintf (stderr, "theme_get: nanowm.cfg file not found!\n");
			return -1;
		}
		ini_fd_t fd = ini_open (filename, "r", "#;");
		free (filename);
		
		if (fd == NULL) {
			fprintf (stderr, "theme_get: Unable to open nanowm.cfg file\n");
			return -1;
		} else {
			int ret = ini_locateHeading (fd, "Main");
			ret = ini_locateKey (fd, "Menu");
			ret = ini_readString (fd, p, sizeof (buffer));
		}
		ini_close(fd);

		/* open theme config file */
		char path[256] =  ACTIVETHEME_PATH;

		strncat(path, p, 255);
		strncat(path, ".cfg", 255);
		filename = cfg_findfile (path);
		menu_config_fd = ini_open (filename, "r", "#;");
		free (filename);
		if (menu_config_fd == NULL) {
			fprintf (stderr, "theme_get: Unable to open theme config file %s\n", path);
			return -1;
		}

	}
	/* load needed image */
	const char ** theme_group; /*  group name */
	const char ** theme_images;	/* pointer to the theme_*_images array */

	theme_group 	= menu_groups;
	theme_images	= menu_groups_items;

	if (ini_locateHeading (menu_config_fd, theme_group) ) {
		fprintf (stderr, "theme_get: Unable to find group %s\n",menu_groups);
		return -1;
	}
	if (ini_locateKey (menu_config_fd, theme_images[item_index] ) < 0 ) {
		fprintf (stderr, "theme_get: Unable to find key %s\n",menu_groups_items[item_index]);
		return -1;
	}
	if (ini_readString (menu_config_fd, p, sizeof (buffer))  < 0 ) {
		fprintf (stderr, "theme_get: Unable to get string from key %s\n",theme_images[item_index]);
		return -1;
	}

	item->name = strdup ( theme_images[item_index] );

	/* now parse key value */
	char * token;
	token = strtok(p, ", "); //printf ( "token = %s\n", token );
	/*token = type */
	item->type= atoi(token);

	token = strtok(NULL, ", ");// printf ( "token = %s\n", token );
	/*token = imagename */	

	if (token) {
		char path[256] =  ACTIVETHEME_PATH;
		strncat(path, token, 255);
		//char *filename = cfg_findfile (path);
		item->icon = cfg_findfile (path);
		//printf ( "item_icon= %s\n", item->icon );
		if(!item->icon) {
			fprintf (stderr, "theme_get: Unable to find image %s\n",path);
			return -1;
		} else {
			item->icon_id = GrLoadImageFromFile(item->icon, 0);
			if (item->icon_id == 0){
				fprintf(stderr, "theme_get: Failed to load image %s\n", item->icon);
				return -1;
			}
			//free(item->icon);
		}
	} else {
		item->icon_id = 0;
		return -1;
	}

	token = strtok(NULL, ", "); //printf ( "token = %s\n", token );
	switch ( item->type ){
		case MENU_ITEM_TYPE_PROG:
			item->prog = my_malloc ( sizeof ( PROG_ITEM ) );
			item->prog->command = strdup ( token );
			int n;
			for ( n = 0; n < MAX_ARGUMENTS; n++ ) item->prog->argv[n] = NULL;
			for ( n = 0; n < MAX_ARGUMENTS; n++ ) {
				token = strtok(NULL, ", "); //printf ( "token = %s\n", token );
				if ( token != NULL) item->prog->argv[n] = strdup ( token );
				else break;
			}
			break;
		case MENU_ITEM_TYPE_MENU:
			return 1;
			break;
		default:
			return -1;
	}

	return 1;
}

//--------------


pid_t launch_program(PROG_ITEM *prog){
	pid_t pid;

	if((pid = fork()) == -1){
		perror("Couldn't fork");
	} else if(!pid) {
		if(execvp(prog->command, prog->argv) == -1)
			fprintf(stderr, "Couldn't start \"%s\": %s\n", prog->command, strerror(errno));
		exit(7);
	}
	return pid;
}


void show_item ( MSTATE *state, MITEM *item ){

	GR_SIZE width, height, base, x, len;

	GrDrawImageToFit(item->wid, state->gc, ICON_X_POSITION, ICON_Y_POSITION, ICON_WIDTH, ICON_HEIGHT, item->icon_id);

	len = strlen(item->name);
	GrGetGCTextSize(state->gc, item->name, len, 0, &width, &height, &base);
	if(width >= ITEM_WIDTH) x = 0;
	else x = (ITEM_WIDTH - width) / 2;

	GrText(item->wid, state->gc, x, TEXT_Y_POSITION, item->name, len, 0);

/*	printf ( "---item information:\n.name = %s\n.type = %i\n.icon = %s\n", item->name, item->type, item->icon );
	if ( item->type == 0 ){
		printf ( "command: %s ", item->prog->command );
		int n;
		for ( n = 0; n < MAX_ARGUMENTS; n++ ) 
			if ( item->prog->argv[n] != NULL ) 
				printf ( "argv: %s ", item->prog->argv[n] );
	} else {
		printf ( "sub-menu name = %s\nsub-menu content:\n", item->name );
//		item->sub_menu_base = load_menu( item->type );
//		show_menu ( item->sub_menu_base );
	}
	printf ( "\n" );*/
}

/*void show_menu ( MSTATE *state, MITEM *menu ){
	MITEM *q = menu;
	while ( q != NULL ) {
		show_item ( state, q );
		q = q->next;
	}
}*/


MSTATE * load_menu ( int menu_group_index ){

	MITEM *ttt, *tmp, *m_base;
	m_base = NULL;
	MSTATE *state;
	state = my_malloc ( sizeof ( MSTATE ) );
	state->numitems = 0;

	//printf ( "\nstate->num_items = %i\n", state->numitems );

	int z, t;
	for ( z=0; z <= MAX_MENU_ITEMS; z++){
		ttt = my_malloc( sizeof( MITEM ) );
		ttt->name = ttt->icon = ttt->sub_menu_base = ttt->prog = ttt->next = NULL;
		t = ttheme_load_item( menu_group_index, z, ttt );
		if ( t == -1 ) {
			if ( m_base == NULL ){
				printf ( "Error!, no items in menu.cfg, or items are in incorrect format. Exiting!\n" );
				exit ( -1 );
			}
			free ( ttt );
			state->numitems--;
			state->main_menu = m_base;
			return state;
		}
		if ( m_base == NULL ){
			tmp = m_base = ttt;
		} else {
			tmp->next = ttt;
			tmp = ttt;
		}
		state->numitems++;
	}
	state->numitems--;
	state->main_menu = m_base;
	return state;
}

void handle_exposure_event(MSTATE *state){
	GR_EVENT_EXPOSURE *event = &state->event.exposure;
	MITEM *i = state->main_menu;

	if(event->wid == state->menu_window) 
		return;
	while(i) {
		if(event->wid == i->wid) {
			show_item(state, i);
			return;
		}
		i = i->next;
	}
	fprintf(stderr, "Got exposure event for unknown window %d\n", event->wid);
}

void handle_mouse_event(MSTATE *state){
	GR_EVENT_MOUSE *event = &state->event.mouse;
	MITEM *i = state->main_menu;

	if(event->wid == state->menu_window) return;

	while(i) {
		if(event->wid == i->wid) {
			launch_program(i->prog);
			return;
		}
		i = i->next;
	}

	fprintf(stderr, "Got mouse event for unknown window %d\n", event->wid);
}

void set_item_selection ( MSTATE *state, int num ){
	int i = 0;
	MITEM *t;
	GR_WM_PROPERTIES props;

	t = state->main_menu;
printf ( "Selecting Item #%i\n", num );

	if ( num <= state->numitems ) {
		if ( num >= 0 ){
			while ( i != num ) {
				t = t->next;
				i++;
			}
		}
	}
	/* set current item background */
	GrSetWindowBackgroundColor ( t->wid, CURRENT_ITEM_BACKGROUND_COLOUR );

	if ( state->cur_sel_item != NULL ) {
		GrSetWindowBackgroundColor ( state->cur_sel_item->wid, ITEM_BACKGROUND_COLOUR );
	}
	state->cur_sel_item = t;
	state->cur_sel_item_num = i;
}

void handle_key_event ( MSTATE *state ){
	printf ( "!got keyboard event, state->event.keystroke.ch = %i\n", state->event.keystroke.ch ); 
	int k;
	switch(state->event.keystroke.ch) {
		case MWKEY_LEFT:
			if ( state->cur_sel_item_num == 0 )
				set_item_selection ( state, state->numitems );
			else
				set_item_selection ( state, state->cur_sel_item_num - 1 );
                        break;
                case MWKEY_RIGHT:
			if ( state->cur_sel_item_num == state->numitems )
				set_item_selection ( state, 0 );
			else
				set_item_selection ( state, state->cur_sel_item_num + 1 );
                	break;
                case MWKEY_UP:
			if ( state->cur_sel_item_num < 3 )
				set_item_selection ( state, state->numitems );
			else
				set_item_selection ( state, state->cur_sel_item_num - 3 );
                	break;
                case MWKEY_DOWN:
				set_item_selection ( state, state->cur_sel_item_num + 3 );
                	break;
		case  MWKEY_ENTER:
			if ( state->cur_sel_item->type == 0 )
				launch_program(state->cur_sel_item->prog);
			break;
                default:
                	break;
	}
}

void handle_event(MSTATE *state){
	switch(state->event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			handle_exposure_event(state);
			break;
		case GR_EVENT_TYPE_BUTTON_DOWN:
			handle_mouse_event(state);
			break;
		case GR_EVENT_TYPE_KEY_DOWN:
                    	handle_key_event ( state );
			break;
		case GR_EVENT_TYPE_CLOSE_REQ:
		case GR_EVENT_TYPE_SCREENSAVER:
		case GR_EVENT_TYPE_TIMER:
		case GR_EVENT_TYPE_NONE:
			break;
		default:
			fprintf(stderr, "Got unknown event type %d\n", state->event.type);
			break;
	}
}

void do_event_loop(MSTATE *state){
	do {
		GrGetNextEvent(&state->event);
		handle_event(state);
	} while(state->event.type != GR_EVENT_TYPE_CLOSE_REQ);
}

int main(int argc, char *argv[]){	

	if ( GrOpen() < 0){
		error("Couldn't connect to Nano-X server!\n");
		exit(-1);
	}

	MSTATE *state;
	GR_WM_PROPERTIES props;
	
	state = load_menu ( GROUP_MAIN_MENU ); printf ( "state.numitems = %i", state->numitems );
	//show_menu ( state, state->main_menu );

	state->gc = GrNewGC();

	GrSetGCForeground(state->gc, ITEM_TEXT_COLOUR);
	GrSetGCBackground(state->gc, ITEM_BACKGROUND_COLOUR);

/* vovan888: why clear root window ? */
//	GrClearWindow(GR_ROOT_WINDOW_ID, GR_TRUE);

	//state->menu_window = GrNewWindowWithTitle(GR_ROOT_WINDOW_ID, 0, 20, 176, 220, 0, ITEM_BACKGROUND_COLOUR, 0,"launcher");
	state->menu_window = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 20, 176, 200, 0, MWNOCOLOR, 0);

	GrSelectEvents(state->menu_window, GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_KEY_DOWN );
	props.flags = GR_WM_FLAGS_PROPS;
	props.props = GR_WM_PROPS_NOMOVE | GR_WM_PROPS_NODECORATE | GR_WM_PROPS_NOAUTOMOVE | GR_WM_PROPS_NOAUTORESIZE | GR_WM_PROPS_NOBACKGROUND;
	GrSetWMProperties(state->menu_window, &props);

	GrMapWindow(state->menu_window);
	GrSetFocus(state->menu_window);

	MITEM *xxx;
	xxx = state->main_menu;
	int x = 0, y = 0, cnt = 1;
	while ( xxx != NULL ){
		xxx->wid = GrNewWindow(state->menu_window, x + 5, y + 5, ITEM_WIDTH, ITEM_HEIGHT, 0,  ITEM_BACKGROUND_COLOUR, ITEM_BORDER_COLOUR);
		GrSelectEvents(xxx->wid, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_BUTTON_DOWN );
		GrMapWindow(xxx->wid);
		xxx = xxx->next;
		cnt++;
		x = x + ITEM_WIDTH;
		if ( cnt == 4 ){
			cnt = 1;
			x = 0;
			y = y + ITEM_HEIGHT;
		}
	}
	GrMapWindow(state->menu_window);

	set_item_selection ( state, 0 );

	do_event_loop( state );
}
