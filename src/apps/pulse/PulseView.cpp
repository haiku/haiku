//****************************************************************************************
//
//	File:		PulseView.cpp
//
//	Written by:	David Ramsey and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "PulseView.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Catalog.h>

#include <syscalls.h>

#include "Common.h"
#include "PulseApp.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PulseView"


PulseView::PulseView(BRect rect, const char *name) :
	BView(rect, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS) {

	popupmenu = NULL;
	cpu_menu_items = NULL;

	// Don't init the menus for the DeskbarPulseView, because this instance
	// will only be used to archive the replicant
	if (strcmp(name, "DeskbarPulseView") != 0) {
		Init();
	}
}

// This version will be used by the instantiated replicant
PulseView::PulseView(BMessage *message) : BView(message) {
	SetResizingMode(B_FOLLOW_ALL_SIDES);
	SetFlags(B_WILL_DRAW | B_PULSE_NEEDED);

	popupmenu = NULL;
	cpu_menu_items = NULL;
	Init();
}

void PulseView::Init() {
	popupmenu = new BPopUpMenu("PopUpMenu", false, false, B_ITEMS_IN_COLUMN);
	popupmenu->SetFont(be_plain_font);
	mode1 = new BMenuItem("", NULL, 0, 0);
	mode2 = new BMenuItem("", NULL, 0, 0);
	preferences = new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS), 
		new BMessage(PV_PREFERENCES), 0, 0);
	about = new BMenuItem(B_TRANSLATE("About Pulse" B_UTF8_ELLIPSIS), 
		new BMessage(PV_ABOUT), 0, 0);

	popupmenu->AddItem(mode1);
	popupmenu->AddItem(mode2);
	popupmenu->AddSeparatorItem();

	system_info sys_info;
	get_system_info(&sys_info);

	// Only add menu items to control CPUs on an SMP machine
	if (sys_info.cpu_count >= 2) {
		cpu_menu_items = new BMenuItem *[sys_info.cpu_count];
		char temp[20];
		for (int x = 0; x < sys_info.cpu_count; x++) {
			sprintf(temp, "CPU %d", x + 1);
			BMessage *message = new BMessage(PV_CPU_MENU_ITEM);
			message->AddInt32("which", x);
			cpu_menu_items[x] = new BMenuItem(temp, message, 0, 0);
			popupmenu->AddItem(cpu_menu_items[x]);
		}
		popupmenu->AddSeparatorItem();
	}

	popupmenu->AddItem(preferences);
	popupmenu->AddItem(about);
}

void PulseView::MouseDown(BPoint point) {
	BPoint cursor;
	uint32 buttons;
	MakeFocus(true);
	GetMouse(&cursor, &buttons, true);

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		ConvertToScreen(&point);
		// Use the asynchronous version so we don't interfere with
		// the window responding to Pulse() events
		popupmenu->Go(point, true, false, true);
	}
}

void PulseView::Update() {
	system_info sys_info;
	get_system_info(&sys_info);
	bigtime_t now = system_time();

	// Calculate work done since last call to Update() for each CPU
	for (int x = 0; x < sys_info.cpu_count; x++) {
		double cpu_time = (double)(sys_info.cpu_infos[x].active_time - prev_active[x]) / (now - prev_time);
		prev_active[x] = sys_info.cpu_infos[x].active_time;
		if (cpu_time < 0) cpu_time = 0;
		if (cpu_time > 1) cpu_time = 1;
		cpu_times[x] = cpu_time;

		if (sys_info.cpu_count >= 2) {
			if (!_kern_cpu_enabled(x) && cpu_menu_items[x]->IsMarked())
				cpu_menu_items[x]->SetMarked(false);
			if (_kern_cpu_enabled(x) && !cpu_menu_items[x]->IsMarked())
				cpu_menu_items[x]->SetMarked(true);
		}
	}
	prev_time = now;
}

void PulseView::ChangeCPUState(BMessage *message) {
	int which = message->FindInt32("which");

	if (!LastEnabledCPU(which)) {
		_kern_set_cpu_enabled(which, (int)!cpu_menu_items[which]->IsMarked());
	} else {
		BAlert *alert = new BAlert(B_TRANSLATE("Info"),
			B_TRANSLATE("You can't disable the last active CPU."),
			B_TRANSLATE("OK"));
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go(NULL);
	}
}

PulseView::~PulseView() {
	if (popupmenu != NULL) delete popupmenu;
	if (cpu_menu_items != NULL) delete cpu_menu_items;
}

