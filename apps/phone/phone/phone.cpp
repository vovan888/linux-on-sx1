/*
 * Phone application
 *
 * Copyright (c) 2008 Vladimir Ananiev <vovan888 at gmail com>
 *
 * Licensed under GPLv2, see file License in this tarball for details.
 */

// generated by Fast Light User Interface Designer (fluid) version 1.0107

#include "phone.h"
#include <stdlib.h>

#include <ipc/shareddata.h>

PhoneApp app;

//---------------------------------------------------------------------------
/**
 * Removes all illegal characters from the phone number
 * @param str string with number
 * @param len string length
*/
void PhoneApp::RemoveIllegalChars(char *str, int len)
{
	int i, j;
	char ch;
	for (i = j = 0; i < len; i++) {
		ch = str[i];
		if (((ch >= '0') && (ch <= '9')) || (ch == '+')) {
			str[j] = ch;
			j++;
		}
	}
	str[j] = 0;
}
//---------------------------------------------------------------------------

void PhoneApp::SetOperatorName()
{
	char *name = Shm->PhoneServer.Network_Operator;
	char decoded[32];
	char str[3];
	int i, len = strlen(name);

	if (len > 0) {
		for (i = 0; i < len / 4; i++) {
			//004D00540053002D005200550053
			str[0] = name[i * 4 + 2];
			str[1] = name[i * 4 + 3];
			str[2] = 0;
			decoded[i] = (char)strtol(str, NULL, 16);
		}
		decoded[i] = 0;
		OpName->value(decoded);
	}
}
//---------------------------------------------------------------------------
int PhoneApp::RegisteredToNetwork()
{
	if (Shm->PhoneServer.CREG_State == GSMD_NETREG_REG_HOME)
		return 1;
	if (Shm->PhoneServer.CREG_State == GSMD_NETREG_REG_ROAMING)
		return 1;
	return 0;
}
//---------------------------------------------------------------------------
void PhoneApp::cb_timeout_qregister(void *)
{
	tbus_call_method ("PhoneServer", "Network/QueryRegistration", "");
}
//---------------------------------------------------------------------------
void PhoneApp::ConnectSignals()
{
	tbus_connect_signal("PhoneServer", "CallProgress");
	tbus_connect_signal("PhoneServer", "CMECMSError");
	tbus_connect_signal("PhoneServer", "IncomingCall");
	tbus_connect_signal("PhoneServer", "IncomingCLIP");
	tbus_connect_signal("PhoneServer", "OutgoingCOLP");
	tbus_connect_signal("PhoneServer", "NetworkReg");

	tbus_call_method("PhoneServer", "Network/RegisterAuto", "");
	// start timer to query network registration
	Fl::add_timeout(TIMEOUT_QNETWORK, cb_timeout_qregister);

	Shm = (struct SharedSystem *)ShmMap(SHARED_SYSTEM);
}
//---------------------------------------------------------------------------
void PhoneApp::handle_method_return(struct tbus_message *msg)
{
	DPRINT("(%s/%s)\n", msg->service_sender, msg->object);
	if (!strcmp("PhoneServer", msg->service_sender))
		if (!strcmp("Network/QueryRegistration", msg->object)) {
			if(RegisteredToNetwork()) {
				tbus_call_method ("PhoneServer", "Network/OperatorGet", "");
				Fl::remove_timeout(cb_timeout_qregister);
			} else
				Fl::add_timeout(TIMEOUT_QNETWORK, cb_timeout_qregister);

		} else if (!strcmp("Network/OperatorGet", msg->object)) {
			SetOperatorName();
		}

}
//---------------------------------------------------------------------------
void PhoneApp::handle_signal(struct tbus_message *msg)
{
	DPRINT("(%s/%s)\n", msg->service_sender, msg->object);

	if (!strcmp("PhoneServer", msg->service_sender))
		if (!strcmp("CallProgress", msg->object)) {
			int ret, progress, call_id;
			ret = tbus_get_message_args(msg, "ii", &progress, &call_id);
			if (ret < 0)
				return;
			if ((AppState == STATE_DIALING) && (progress == GSMD_CALL_CONNECTED)) {
				GotoState(STATE_CALL_ACTIVE);
			} else if (progress == GSMD_CALL_IDLE) {
				GotoState(STATE_END_CALL);
			}
		} else if (!strcmp("PINneeded", msg->object)) {
			// show some message here !
			GotoState(STATE_END_CALL);
		} else if (!strcmp("CMECMSError", msg->object)) {
			// TODO - check error number
			if (AppState != STATE_END_CALL)
				GotoState(STATE_END_CALL);
		} else if (!strcmp("IncomingCall", msg->object)) {
			GotoState(STATE_INCOMING);
		} else if (!strcmp("IncomingCLIP", msg->object)) {
			char *clip;
			int ret;
			ret = tbus_get_message_args(msg, "s", &clip);
			Number->value(clip);
			free(clip);
		} else if (!strcmp("OutgoingCOLP", msg->object)) {
			char *colp;
			int ret;
			ret = tbus_get_message_args(msg, "s", &colp);
			Number->value(colp);
			free(colp);
		}
}

//---------------------------------------------------------------------------
void PhoneApp::GreenButtonPressed()
{
//	if(!RegisteredToNetwork())
//		return;

	if (AppState == STATE_CALL_ACTIVE) {
		//TODO Hold call
		return;
	} else if (AppState == STATE_IDLE) {
		// check if phone number is valid
		PhoneNumber = strdup(NumberInput->value());
		int NumLen = strlen(PhoneNumber);
		RemoveIllegalChars(PhoneNumber, NumLen);
		NumLen = strlen(PhoneNumber);
		if (NumLen <= 0)
			return;

		// fill in call group Output`s
		ContactName->value("John Dow");
		Number->value(PhoneNumber);

		// activate call
		tbus_call_method("PhoneServer", "VoiceCall/Dial", "s", &PhoneNumber);

		// update application state
		GotoState(STATE_DIALING);
	} else if (AppState == STATE_INCOMING) {
		// answer an incoming call
		tbus_call_method("PhoneServer", "VoiceCall/Answer", "");
		GotoState(STATE_CALL_ACTIVE);
	}
}

//---------------------------------------------------------------------------
void PhoneApp::RedButtonPressed()
{
	if (AppState == STATE_IDLE) {
		// hide application
		iconize();
		return;
	} else if (AppState == STATE_END_CALL) {
		Fl::remove_timeout(cb_timeout_end_call);
		GotoState(STATE_IDLE);
	} else {
		tbus_call_method("PhoneServer", "VoiceCall/HangUp", "");
		//TODO start end_call timer
		GotoState(STATE_END_CALL);
	}
}

//---------------------------------------------------------------------------
void PhoneApp::GotoState(int newAppState)
{
	DPRINT("GotoState (%d)\n", newAppState);

	show();	// just in case we are iconized

	switch (newAppState) {
	case STATE_IDLE:
		// switch to the dial group
		Number->value("");
		ContactName->value("");
		CallType->value("");
		grp_dial->show();
		grp_call->hide();
		break;
	case STATE_INCOMING:
		// switch to the call group
		CallType->value("Incoming call...");
		grp_dial->hide();
		grp_call->show();
		break;
	case STATE_DIALING:
		// switch to the call group
		CallType->value("Dialing...");
		grp_dial->hide();
		grp_call->show();
		// possible TODO :
		// play a dialing sound
		// start a dialing animation
		break;
	case STATE_CALL_ACTIVE:
		CallType->value("Talking...");
		// TODO start call duration timer
		break;
	case STATE_END_CALL:
		CallType->value("Call ended...");
		// start a timer, then go to IDLE
		Fl::add_timeout(TIMEOUT_END_CALL, cb_timeout_end_call, this);
		break;
	}

	AppState = newAppState;
}
//---------------------------------------------------------------------------
void PhoneApp::cb_timeout_end_call(void *w)
{
	PhoneApp *Phone = (PhoneApp *)w;
	Phone->GotoState(STATE_IDLE);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_NumberInput_i(Fl_Input *, void *)
{
	// check value to fit to phone number
// 0..9 , + , p , w;
}
void PhoneApp::cb_NumberInput(Fl_Input * o, void *v)
{
	((PhoneApp *) (o->parent()->parent()))->cb_NumberInput_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_search_i(Fl_Text_Display *, void *)
{
	// keypress on search results;
}
void PhoneApp::cb_search(Fl_Text_Display * o, void *v)
{
	((PhoneApp *) v)->cb_search_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_btn_end_call_i(Fl_Button *, void *)
{
	// end call;
	RedButtonPressed();
}
void PhoneApp::cb_btn_end_call(Fl_Button * o, void *v)
{
	((PhoneApp *) v)->cb_btn_end_call_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_Add_i(Fl_Menu_ *, void *)
{
	// add recipient option;
}
void PhoneApp::cb_Add(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_Add_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_Call_i(Fl_Menu_ *, void *)
{
	// dial a number;
	GreenButtonPressed();
}
void PhoneApp::cb_Call(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_Call_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_Copy_i(Fl_Menu_ *, void *)
{
	// copy option;
}
void PhoneApp::cb_Copy(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_Copy_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_Speaker_i(Fl_Menu_ *, void *)
{
	// turn Speaker on or off;
}
void PhoneApp::cb_Speaker(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_Speaker_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_Hold_i(Fl_Menu_ *, void *)
{
	//hold this call;
}
void PhoneApp::cb_Hold(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_Hold_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_Disable_i(Fl_Menu_ *, void *)
{
	//disable microphone;
}
void PhoneApp::cb_Disable(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_Disable_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_Send_i(Fl_Menu_ *, void *)
{
	//send a DTMF string;
}
void PhoneApp::cb_Send(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_Send_i(o, v);
}

//---------------------------------------------------------------------------
void PhoneApp::cb_End_i(Fl_Menu_ *, void *)
{
	// end current call;
	RedButtonPressed();
}
void PhoneApp::cb_End(Fl_Menu_ * o, void *v)
{
	((PhoneApp *) (o->parent()->user_data()))->cb_End_i(o, v);
}

//---------------------------------------------------------------------------
Fl_Menu_Item PhoneApp::menu_LeftSoft[] = {
	{"Add recipient", 0, (Fl_Callback *) PhoneApp::cb_Add, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Call", Key_Green, (Fl_Callback *) PhoneApp::cb_Call, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Copy", 0, (Fl_Callback *) PhoneApp::cb_Copy, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Speaker", 0, (Fl_Callback *) PhoneApp::cb_Speaker, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Hold", 0, (Fl_Callback *) PhoneApp::cb_Hold, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Disable Mic", 0, (Fl_Callback *) PhoneApp::cb_Disable, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Send DTMF", 0, (Fl_Callback *) PhoneApp::cb_Send, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{"Отбой", 0, (Fl_Callback *) PhoneApp::cb_End, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0}
};

//---------------------------------------------------------------------------
void PhoneApp::cb_RightSoft_i(Fl_Menu_Button *, void *)
{
	// Close program;
	exit(1);
}
void PhoneApp::cb_RightSoft(Fl_Menu_Button * o, void *v)
{
	((PhoneApp *) (o->parent()))->cb_RightSoft_i(o, v);
}

//---------------------------------------------------------------------------
PhoneApp::PhoneApp()
:  Fl_App("Phone", true)
{
	user_data(this);

	{
		Fl_Group *o = grp_dial = new Fl_Group(0, 0, 176, 174, "Dial");
		{
			Fl_Input *o = NumberInput = new Fl_Input(0, 0, 176, 32);
			o->box(FL_FLAT_BOX);
			o->textfont(1);
			o->textsize(20);
			o->callback((Fl_Callback *) cb_NumberInput);
			o->when(FL_WHEN_CHANGED);
		}
		{
			Fl_Text_Display *o = search = new Fl_Text_Display(0, 34, 176, 140);
			o->box(FL_FLAT_BOX);
			o->callback((Fl_Callback *) cb_search, (void *)this);
			o->when(FL_WHEN_CHANGED);
		}
		o->user_data(this);
		o->end();
	}
	{
		Fl_Group *o = grp_call = new Fl_Group(0, 0, 176, 174, "Call");
		o->hide();
		{
			Fl_Output *o = OpName = new Fl_Output(2, 2, 172, 25);
			o->box(FL_FLAT_BOX);
			o->textsize(12);
		}
		{
			Fl_Output *o = CallType = new Fl_Output(2, 24, 172, 25);
			o->box(FL_FLAT_BOX);
		}
		{
			Fl_Output *o = ContactName = new Fl_Output(2, 66, 172, 25);
			o->box(FL_FLAT_BOX);
		}
		{
			Fl_Output *o = Number = new Fl_Output(2, 91, 172, 25);
			o->box(FL_FLAT_BOX);
		}
		{
			Fl_Button *o = btn_end_call = new Fl_Button(4, 150, 166, 25, "End call");
			o->labelfont(1);
			o->shortcut(Key_Red);
			o->callback((Fl_Callback *) cb_btn_end_call, (void *)this);
		}
		o->user_data(this);
		o->end();
	}
	AppArea->add(grp_dial);
	AppArea->add(grp_call);
	LeftSoftMenu->menu(menu_LeftSoft);
	RightSoft->callback((Fl_Callback *) cb_RightSoft, (void *)this);

	AppState = STATE_IDLE;
	OpName->value("");
	PhoneNumber = "";

	ConnectSignals();
}

//---------------------------------------------------------------------------
PhoneApp::~PhoneApp()
{
	if (PhoneNumber)
		delete PhoneNumber;
}

//---------------------------------------------------------------------------
int main(int argc, char **argv)
{
	app.show();
	return Fl::run();
}
