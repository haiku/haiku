//****************************************************************************************
//
//	File:		ConfigView.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef CONFIGVIEW_H
#define CONFIGVIEW_H

#include <interface/CheckBox.h>
#include <interface/RadioButton.h>
#include <interface/TextControl.h>
#include <interface/ColorControl.h>
#include "Prefs.h"

class RTColorControl : public BColorControl {
	public:
		RTColorControl(BPoint point, BMessage *message);
		void SetValue(int32 color);
};

class ConfigView : public BView {
	public:
		ConfigView(BRect rect, const char *name, int mode, Prefs *prefs);
		void AttachedToWindow();
		void MessageReceived(BMessage *message);
		void UpdateDeskbarIconWidth();
		
	private:
		void ResetDefaults();
		bool first_time_attached;
		int mode;
		
		RTColorControl *colorcontrol;
		// For Normal
		BCheckBox *fadecolors;
		// For Mini and Deskbar
		BRadioButton *active, *idle, *frame;
		// For Deskbar
		BTextControl *iconwidth;
};

#endif
