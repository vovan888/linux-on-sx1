#define ITEM_WIDTH 55
#define ITEM_HEIGHT 60
#define ITEM_TEXT_COLOUR GR_COLOR_BLACK
#define ITEM_BORDER_COLOUR GR_COLOR_BLACK
#define ITEM_BACKGROUND_COLOUR GR_COLOR_GRAY80
#define CURRENT_ITEM_BACKGROUND_COLOUR GR_COLOR_SKYBLUE
#define ICON_WIDTH 32
#define ICON_HEIGHT 32
#define ICON_X_POSITION ((ITEM_WIDTH - ICON_WIDTH) / 2)
#define ICON_Y_POSITION 6
#define TEXT_Y_POSITION (ITEM_HEIGHT - 6)
#define MAX_ARGUMENTS 12

struct command_argv {
	char *command;
	char *argv[MAX_ARGUMENTS];
};
typedef struct command_argv PROG_ITEM;

struct menu_item {
	char *name;
	char *icon;
	int type; //should be 0 - for program item, 1 - fulder/sub-menu
	PROG_ITEM *prog; //used if type=0
	struct MITEM *sub_menu_base; // if type=1 accordingly
	struct MITEM *next;	
	GR_IMAGE_ID icon_id;
	GR_WINDOW_ID wid;
};
typedef struct menu_item MITEM;


struct menu_state {
	GR_WINDOW_ID menu_window;
	MITEM *main_menu;
	int numitems;
	MITEM *cur_sel_item;
	int cur_sel_item_num;
	GR_GC_ID gc;
	GR_EVENT event;
};
typedef struct menu_state MSTATE;

MSTATE * load_menu ( int menu_group_index );
void show_menu ( MSTATE *state, MITEM *menu );
