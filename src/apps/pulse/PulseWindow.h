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


class PulseWindow : public BWindow {
	public:
		PulseWindow(BRect rect);
		virtual ~PulseWindow();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);

		void SetMode(int newmode);

	private:
		NormalPulseView*	fNormalPulseView;
		MiniPulseView*		fMiniPulseView;
		int32				fMode;
};

#endif	// PULSEWINDOW_H
