//****************************************************************************************
//
//	File:		CPUButton.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef CPUBUTTON_H
#define CPUBUTTON_H

#include <interface/Control.h>
#include <app/MessageRunner.h>

class CPUButton : public BControl {
	public:
		CPUButton(BRect rect, const char *name, const char *label, BMessage *message);
		CPUButton(BMessage *message);
		void Draw(BRect rect);
		void MouseDown(BPoint point);
		void MouseUp(BPoint point);
		void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		~CPUButton();
		
		status_t Invoke(BMessage *message = NULL);
		static CPUButton *Instantiate(BMessage *data);
		status_t Archive(BMessage *data, bool deep = true) const;
		void MessageReceived(BMessage *message);
		
		void UpdateColors(int32 color);
		void AttachedToWindow();

	private:
		rgb_color on_color, off_color;
		bool replicant;
		BMessageRunner *messagerunner;
};

#endif
		