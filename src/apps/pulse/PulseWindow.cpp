//****************************************************************************************
//
//	File:		PulseWindow.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "PulseWindow.h"
#include "PulseApp.h"
#include "Common.h"
#include "DeskbarPulseView.h"
#include <interface/Alert.h>
#include <interface/Deskbar.h>
#include <stdlib.h>
#include <string.h>

PulseWindow::PulseWindow(BRect rect) :
	BWindow(rect, "Pulse", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE) {

	SetPulseRate(200000);
	
	PulseApp *pulseapp = (PulseApp *)be_app;
	BRect bounds = Bounds();
	normalpulseview = new NormalPulseView(bounds);
	AddChild(normalpulseview);
	
	minipulseview = new MiniPulseView(bounds, "MiniPulseView", pulseapp->prefs);
	AddChild(minipulseview);
	
	mode = pulseapp->prefs->window_mode;
	if (mode == MINI_WINDOW_MODE) {
		SetLook(B_MODAL_WINDOW_LOOK);
		SetFeel(B_NORMAL_WINDOW_FEEL);
		SetFlags(B_NOT_ZOOMABLE);
		normalpulseview->Hide();
		SetSizeLimits(GetMinimumViewWidth() - 1, 4096, 2, 4096);
		ResizeTo(rect.Width(), rect.Height());
	} else minipulseview->Hide();
	
	prefswindow = NULL;
}

void PulseWindow::MessageReceived(BMessage *message) {
	switch(message->what) {
		case PV_NORMAL_MODE:
		case PV_MINI_MODE:
		case PV_DESKBAR_MODE:
			SetMode(message->what);
			break;
		case PRV_NORMAL_FADE_COLORS:
		case PRV_NORMAL_CHANGE_COLOR:
			normalpulseview->UpdateColors(message);
			break;
		case PRV_MINI_CHANGE_COLOR:
			minipulseview->UpdateColors(message);
			break;
		case PRV_QUIT:
			prefswindow = NULL;
			break;
		case PV_PREFERENCES: {
			// If the window is already open, bring it to the front
			if (prefswindow != NULL) {
				prefswindow->Activate(true);
				break;
			}
			// Otherwise launch a new preferences window
			PulseApp *pulseapp = (PulseApp *)be_app;
			prefswindow = new PrefsWindow(pulseapp->prefs->prefs_window_rect,
				"Pulse Preferences", new BMessenger(this), pulseapp->prefs);
			prefswindow->Show();
			break;
		}
		case PV_ABOUT: {
			BAlert *alert = new BAlert("Info", "Pulse\n\nBy David Ramsey and Arve Hjønnevåg\nRevised by Daniel Switkin", "OK");
			// Use the asynchronous version so we don't block the window's thread
			alert->Go(NULL);
			break;
		}
		case PV_QUIT:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case PV_CPU_MENU_ITEM:
			// Call the correct version based on whose menu sent the message
			if (minipulseview->IsHidden()) normalpulseview->ChangeCPUState(message);
			else minipulseview->ChangeCPUState(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

void PulseWindow::SetMode(int newmode) {
	PulseApp *pulseapp = (PulseApp *)be_app;
	switch (newmode) {
		case PV_NORMAL_MODE:
			if (mode == MINI_WINDOW_MODE) {
				pulseapp->prefs->mini_window_rect = Frame();
				pulseapp->prefs->window_mode = NORMAL_WINDOW_MODE;
				pulseapp->prefs->Save();
			}
			minipulseview->Hide();
			normalpulseview->Show();
			mode = NORMAL_WINDOW_MODE;
			SetType(B_TITLED_WINDOW);
			SetFlags(B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
			ResizeTo(pulseapp->prefs->normal_window_rect.IntegerWidth(),
				pulseapp->prefs->normal_window_rect.IntegerHeight());
			MoveTo(pulseapp->prefs->normal_window_rect.left,
				pulseapp->prefs->normal_window_rect.top);
			break;
		case PV_MINI_MODE:
			if (mode == NORMAL_WINDOW_MODE) {
				pulseapp->prefs->normal_window_rect = Frame();
				pulseapp->prefs->window_mode = MINI_WINDOW_MODE;
				pulseapp->prefs->Save();
			}
			normalpulseview->Hide();
			minipulseview->Show();
			mode = MINI_WINDOW_MODE;
			SetLook(B_MODAL_WINDOW_LOOK);
			SetFeel(B_NORMAL_WINDOW_FEEL);
			SetFlags(B_NOT_ZOOMABLE);
			SetSizeLimits(GetMinimumViewWidth() - 1, 4096, 2, 4096);
			ResizeTo(pulseapp->prefs->mini_window_rect.IntegerWidth(),
				pulseapp->prefs->mini_window_rect.IntegerHeight());
			MoveTo(pulseapp->prefs->mini_window_rect.left,
				pulseapp->prefs->mini_window_rect.top);
			break;
		case PV_DESKBAR_MODE:
			// Do not set window's mode to DESKBAR_MODE because the
			// destructor needs to save the correct BRect. ~PulseApp()
			// will handle launching the replicant after our prefs are saved.
			pulseapp->prefs->window_mode = DESKBAR_MODE;
			PostMessage(B_QUIT_REQUESTED);
			break;
	}
}

PulseWindow::~PulseWindow() {
	PulseApp *pulseapp = (PulseApp *)be_app;
	if (mode == NORMAL_WINDOW_MODE)	pulseapp->prefs->normal_window_rect = Frame();
	else if (mode == MINI_WINDOW_MODE) pulseapp->prefs->mini_window_rect = Frame();
}

bool PulseWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}