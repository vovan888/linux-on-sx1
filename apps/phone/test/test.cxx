// generated by Fast Light User Interface Designer (fluid) version 1.0107

#include "test.h"
#include "stdio.h"

void TestProgramUI::cb_Select_i(Fl_Menu_*, void*) {
  label1->value("selected 1");
}
void TestProgramUI::cb_Select(Fl_Menu_* o, void* v) {
  ((TestProgramUI*)(o->parent()->user_data()))->cb_Select_i(o,v);
}

void TestProgramUI::cb_Select1_i(Fl_Menu_*, void*) {
  label1->value("selected 2");
}
void TestProgramUI::cb_Select3_i(Fl_Menu_*, void*) {

  label1->value("selected 3");
}
void TestProgramUI::cb_Select1(Fl_Menu_* o, void* v) {
  ((TestProgramUI*)(o->parent()->user_data()))->cb_Select1_i(o,v);
}
void TestProgramUI::cb_Select3(Fl_Menu_* o, void* v) {
  ((TestProgramUI*)(o->parent()->user_data()))->cb_Select3_i(o,v);
}

Fl_Menu_Item TestProgramUI::menu_menu_left[] = {
 {"Select 1", 0,  (Fl_Callback*)TestProgramUI::cb_Select, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Select 2", 0,  (Fl_Callback*)TestProgramUI::cb_Select1, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Select 3", 0,  (Fl_Callback*)TestProgramUI::cb_Select3, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

void TestProgramUI::cb_Close_i(Fl_Menu_*, void*) {
  Fl::delete_widget(win);
}
void TestProgramUI::cb_Close(Fl_Menu_* o, void* v) {
  ((TestProgramUI*)(o->parent()->user_data()))->cb_Close_i(o,v);
}

Fl_Menu_Item TestProgramUI::menu_menu_right[] = {
 {"Close", 0,  (Fl_Callback*)TestProgramUI::cb_Close, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Hide", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

TestProgramUI::TestProgramUI() {
  Fl_Window* w;
  { Fl_Window* o = win = new Fl_Window(175, 200);
    w = o;
    o->user_data((void*)(this));
    label1 = new Fl_Output(10, 32, 150, 18);
    { Fl_Menu_Button* o = menu_left = new Fl_Menu_Button(0, 180, 85, 20, "Options");
      o->box(FL_FLAT_BOX);
      o->menu(menu_menu_left);
    }
    { Fl_Menu_Button* o = menu_right = new Fl_Menu_Button(85, 180, 89, 20, "Close");
      o->box(FL_FLAT_BOX);
      o->menu(menu_menu_right);
    }
    o->size_range(0, 0, 176, 204);
    o->end();
  }
}

void TestProgramUI::show(int argc, char **argv) {
  win->show(argc,argv);
}

int main(int argc, char **argv) {
  TestProgramUI ui;
	ui.show(argc,argv);
  return Fl::run();
}
