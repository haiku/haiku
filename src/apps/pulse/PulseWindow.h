//****************************************************************************************
//
//	File:		PulseWindow.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef PULSEWINDOW_H
#define PULSEWINDOW_H

#include <interface/Window.h>
#include "NormalPulseView.h"
#include "MiniPulseView.h"
#include "PrefsWindow.h"

class PulseWindow : public BWindow {
	public:
		PulseWindow(BRect rect);
		~PulseWindow();
		bool QuitRequested();
		void MessageReceived(BMessage *message);
		void SetMode(int newmode);

	private:
		NormalPulseView *normalpulseview;
		MiniPulseView *minipulseview;
		PrefsWindow *prefswindow;
		int mode;
};

#endif
