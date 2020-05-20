//****************************************************************************************
//
//	File:		DeskbarPulseView.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************


#include "DeskbarPulseView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Roster.h>

#include "Common.h"
#include "Prefs.h"
#include "PulseApp.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeskbarPulseView"


DeskbarPulseView::DeskbarPulseView(BRect rect)
	: MiniPulseView(rect, "DeskbarPulseView")
{
	messagerunner = NULL;
	prefs = NULL;
}


DeskbarPulseView::DeskbarPulseView(BMessage *message)
	: MiniPulseView(message)
{
	mode1->SetLabel(B_TRANSLATE("Normal mode"));
	mode1->SetMessage(new BMessage(PV_NORMAL_MODE));
	mode2->SetLabel(B_TRANSLATE("Mini mode"));
	mode2->SetMessage(new BMessage(PV_MINI_MODE));
	quit = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(PV_QUIT), 0, 0);
	popupmenu->AddSeparatorItem();
	popupmenu->AddItem(quit);

	SetViewColor(B_TRANSPARENT_COLOR);

	prefs = new Prefs();
	active_color.red = (prefs->deskbar_active_color & 0xff000000) >> 24;
	active_color.green = (prefs->deskbar_active_color & 0x00ff0000) >> 16;
	active_color.blue = (prefs->deskbar_active_color & 0x0000ff00) >> 8;

	idle_color.red = (prefs->deskbar_idle_color & 0xff000000) >> 24;
	idle_color.green = (prefs->deskbar_idle_color & 0x00ff0000) >> 16;
	idle_color.blue = (prefs->deskbar_idle_color & 0x0000ff00) >> 8;

	frame_color.red = (prefs->deskbar_frame_color & 0xff000000) >> 24;
	frame_color.green = (prefs->deskbar_frame_color & 0x00ff0000) >> 16;
	frame_color.blue = (prefs->deskbar_frame_color & 0x0000ff00) >> 8;
	SetViewColor(idle_color);

	messagerunner = NULL;
}


void
DeskbarPulseView::AttachedToWindow()
{
	BMessenger messenger(this);
	mode1->SetTarget(messenger);
	mode2->SetTarget(messenger);
	preferences->SetTarget(messenger);
	about->SetTarget(messenger);
	quit->SetTarget(messenger);

	system_info sys_info;
	get_system_info(&sys_info);
	if (sys_info.cpu_count >= 2) {
		for (unsigned int x = 0; x < sys_info.cpu_count; x++)
			cpu_menu_items[x]->SetTarget(messenger);
	}

	// Use a BMessageRunner to deliver periodic messsages instead
	// of Pulse() events from the Deskbar - this is to avoid changing
	// the current pulse rate and affecting other replicants
	messagerunner = new BMessageRunner(messenger, new BMessage(PV_REPLICANT_PULSE),
		200000, -1);
}


void
DeskbarPulseView::MouseDown(BPoint point)
{
	BPoint cursor;
	uint32 buttons;
	MakeFocus(true);
	GetMouse(&cursor, &buttons, true);

	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		BMessage *message = Window()->CurrentMessage();
		int32 clicks = message->FindInt32("clicks");
		if (clicks >= 2) {
			BMessenger messenger(this);
			BMessage *m = new BMessage(PV_NORMAL_MODE);
			messenger.SendMessage(m);
		}
	} else MiniPulseView::MouseDown(point);
}


void
DeskbarPulseView::Pulse()
{
	// Override and do nothing here
}


void
DeskbarPulseView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case PV_NORMAL_MODE:
			SetMode(true);
			Remove();
			break;
		case PV_MINI_MODE:
			SetMode(false);
			Remove();
			break;
		case PV_PREFERENCES:
		{
			message->AddMessenger("settingsListener", this);
			// Spawn the app and open the window there, not in DeskBar process.
			be_roster->Launch(APP_SIGNATURE, message);
			break;
		}
		case PV_ABOUT:
		{
			BMessage aboutRequest(B_ABOUT_REQUESTED);
			be_roster->Launch(APP_SIGNATURE, &aboutRequest);
			break;
		}
		case PV_QUIT:
			Remove();
			break;
		case PRV_DESKBAR_CHANGE_COLOR:
			UpdateColors(message);
			break;
		case PRV_DESKBAR_ICON_WIDTH: {
			int width = message->FindInt32("width");
			ResizeTo(width - 1, Bounds().Height());
			Draw(Bounds());
			break;
		}
		case PV_REPLICANT_PULSE:
			Update();
			Draw(Bounds());
			break;
		case PV_CPU_MENU_ITEM:
			ChangeCPUState(message);
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


DeskbarPulseView *
DeskbarPulseView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "DeskbarPulseView"))
		return NULL;
	return new DeskbarPulseView(data);
}

status_t
DeskbarPulseView::Archive(BMessage *data, bool deep) const
{
	PulseView::Archive(data, deep);
	data->AddString("add_on", APP_SIGNATURE);
	data->AddString("class", "DeskbarPulseView");
	return B_OK;
}


void
DeskbarPulseView::Remove()
{
	// Remove ourselves from the deskbar by name
	BDeskbar *deskbar = new BDeskbar();
	status_t err = deskbar->RemoveItem("DeskbarPulseView");
	if (err != B_OK) {
		BString str;
		snprintf(str.LockBuffer(512), 512,
			B_TRANSLATE("Removing from Deskbar failed.\n%s"), strerror(err));
		str.UnlockBuffer();
		BAlert *alert = new BAlert(B_TRANSLATE("Info"), str.String(),
			B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	}
	delete deskbar;
}


void
DeskbarPulseView::SetMode(bool normal)
{
	if (normal) prefs->window_mode = NORMAL_WINDOW_MODE;
	else prefs->window_mode = MINI_WINDOW_MODE;
	prefs->Save();
	be_roster->Launch(APP_SIGNATURE);
}


DeskbarPulseView::~DeskbarPulseView()
{
	if (messagerunner != NULL) delete messagerunner;
	if (prefs != NULL) delete prefs;
}
