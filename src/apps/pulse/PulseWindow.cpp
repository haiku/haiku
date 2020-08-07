//*****************************************************************************
//
//	File:		PulseWindow.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//*****************************************************************************


#include "PulseWindow.h"
#include "PulseApp.h"
#include "Common.h"
#include "DeskbarPulseView.h"

#include <Alert.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Screen.h>
#include <TextView.h>

#include <stdlib.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PulseWindow"


PulseWindow::PulseWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE_SYSTEM_NAME("Pulse"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_QUIT_ON_WINDOW_CLOSE)
{
	SetPulseRate(200000);

	PulseApp *pulseapp = (PulseApp *)be_app;
	BRect bounds = Bounds();
	fNormalPulseView = new NormalPulseView(bounds);
	AddChild(fNormalPulseView);

	fMiniPulseView = new MiniPulseView(bounds, "MiniPulseView",
		pulseapp->fPrefs);
	AddChild(fMiniPulseView);

	fMode = pulseapp->fPrefs->window_mode;
	if (fMode == MINI_WINDOW_MODE) {
		SetLook(B_MODAL_WINDOW_LOOK);
		SetFeel(B_NORMAL_WINDOW_FEEL);
		SetFlags(B_NOT_ZOOMABLE);
		fNormalPulseView->Hide();
		SetSizeLimits(GetMinimumViewWidth() - 1, 4096, 2, 4096);
		ResizeTo(rect.Width(), rect.Height());
	} else {
		fMiniPulseView->Hide();
		BRect r = fNormalPulseView->Bounds();
		ResizeTo(r.Width(), r.Height());
	}
}


PulseWindow::~PulseWindow()
{
	PulseApp *pulseapp = (PulseApp *)be_app;

	if (fMode == NORMAL_WINDOW_MODE)
		pulseapp->fPrefs->normal_window_rect = Frame();
	else if (fMode == MINI_WINDOW_MODE)
		pulseapp->fPrefs->mini_window_rect = Frame();
}


void
PulseWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case PV_NORMAL_MODE:
		case PV_MINI_MODE:
		case PV_DESKBAR_MODE:
			SetMode(message->what);
			break;
		case PRV_NORMAL_FADE_COLORS:
		case PRV_NORMAL_CHANGE_COLOR:
			fNormalPulseView->UpdateColors(message);
			break;
		case PRV_MINI_CHANGE_COLOR:
			fMiniPulseView->UpdateColors(message);
			break;
		case PV_PREFERENCES: {
			DetachCurrentMessage();
			message->AddMessenger("settingsListener", this);
			be_app->PostMessage(message);
			break;
		}
		case PV_ABOUT: {
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
		}
		case PV_QUIT:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case PV_CPU_MENU_ITEM:
			// Call the correct version based on whose menu sent the message
			if (fMiniPulseView->IsHidden())
				fNormalPulseView->ChangeCPUState(message);
			else
				fMiniPulseView->ChangeCPUState(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
PulseWindow::SetMode(int newmode)
{
	PulseApp *pulseapp = (PulseApp *)be_app;

	switch (newmode) {
		case PV_NORMAL_MODE:
		{
			if (fMode == MINI_WINDOW_MODE) {
				pulseapp->fPrefs->mini_window_rect = Frame();
				pulseapp->fPrefs->window_mode = NORMAL_WINDOW_MODE;
				pulseapp->fPrefs->Save();
			}
			fMiniPulseView->Hide();
			fNormalPulseView->Show();
			fMode = NORMAL_WINDOW_MODE;
			SetType(B_TITLED_WINDOW);
			SetFlags(B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
			BRect r = fNormalPulseView->Bounds();
			ResizeTo(r.Width(), r.Height());
			MoveTo(pulseapp->fPrefs->normal_window_rect.left,
				pulseapp->fPrefs->normal_window_rect.top);
			MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
			break;
		}

		case PV_MINI_MODE:
			if (fMode == NORMAL_WINDOW_MODE) {
				pulseapp->fPrefs->normal_window_rect = Frame();
				pulseapp->fPrefs->window_mode = MINI_WINDOW_MODE;
				pulseapp->fPrefs->Save();
			}
			fNormalPulseView->Hide();
			fMiniPulseView->Show();
			fMode = MINI_WINDOW_MODE;
			SetLook(B_MODAL_WINDOW_LOOK);
			SetFeel(B_NORMAL_WINDOW_FEEL);
			SetFlags(B_NOT_ZOOMABLE);
			SetSizeLimits(GetMinimumViewWidth() - 1, 4096, 2, 4096);
			ResizeTo(pulseapp->fPrefs->mini_window_rect.IntegerWidth(),
				pulseapp->fPrefs->mini_window_rect.IntegerHeight());
			MoveTo(pulseapp->fPrefs->mini_window_rect.left,
				pulseapp->fPrefs->mini_window_rect.top);
			MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
			break;

		case PV_DESKBAR_MODE:
			// Do not set window's mode to DESKBAR_MODE because the
			// destructor needs to save the correct BRect. ~PulseApp()
			// will handle launching the replicant after our prefs are saved.
			pulseapp->fPrefs->window_mode = DESKBAR_MODE;
			LoadInDeskbar();
			break;
	}
}


bool
PulseWindow::QuitRequested()
{
	return true;
}
