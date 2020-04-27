//****************************************************************************************
//
//	File:		DeskbarPulseView.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef DESKBARPULSEVIEW_H
#define DESKBARPULSEVIEW_H

#include "MiniPulseView.h"
#include "PrefsWindow.h"
#include <app/MessageRunner.h>


class DeskbarPulseView : public MiniPulseView 
{
	public:
		DeskbarPulseView(BRect rect);
		DeskbarPulseView(BMessage *message);
		~DeskbarPulseView();
		void MouseDown(BPoint point);
		void AttachedToWindow();
		void Pulse();

		void MessageReceived(BMessage *message);
		static DeskbarPulseView *Instantiate(BMessage *data);
		virtual	status_t Archive(BMessage *data, bool deep = true) const;

	private:
		void Remove();
		void SetMode(bool normal);

		Prefs *prefs;
		BMessageRunner *messagerunner;
};

#endif
