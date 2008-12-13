/* Fl_SoftKey_Menu.cpp
*
*  Applications management
*
* Copyright 2008 by Vladimir Ananiev (Vovan888 at gmail com )
*
* Licensed under GPLv2, see LICENSE
*/

#include "flgui/Fl_SoftButton.h"

/** Create softkey menu button
 * @param leftsoft if true - this is leftsoftkey, false - right
 * @param label caption
 */
Fl_SoftButton::Fl_SoftButton(bool leftsoft,const char *label)
	:Fl_Menu_Button((leftsoft == true)?0 : APPVIEW_WIDTH / 2, APPVIEW_AREA_HEIGHT,
			APPVIEW_WIDTH / 2, APPVIEW_CONTROL_HEIGHT, label)
{
	box(FL_FLAT_BOX);
	shortcut = ((leftsoft == true) ? Key_LeftSoft : Key_RightSoft);
}

int Fl_SoftButton::handle(int event)
{
	switch (event) {
		case FL_SHORTCUT:
			if (!box()) return 0;
//			if (Fl::test_shortcut(shortcut) ) {
			if(shortcut == Fl::event_key()) {
				if (Fl::visible_focus()) Fl::focus(this);
				popup();
				return 1;
			}
	}
	return Fl_Menu_Button::handle(event);
}
