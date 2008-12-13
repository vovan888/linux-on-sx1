/* Fl_SoftButton.h
*
*  Applications management
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#ifndef Fl_SoftButton_h
#define Fl_SoftButton_h

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Button.H>

#include <flphone/theme.h>
#include <flphone/keys_flnx.h>

class Fl_SoftButton : public Fl_Menu_Button {
private:
	int shortcut;
public:
// values for type:
	int handle(int);
	Fl_SoftButton(bool,const char * =0);
};

#endif
