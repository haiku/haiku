//****************************************************************************************
//
//	File:		BottomPrefsView.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef BOTTOMPREFSVIEW_H
#define BOTTOMPREFSVIEW_H

#include <interface/View.h>

class BottomPrefsView : public BView {
	public:
		BottomPrefsView(BRect rect, const char *name);
		void Draw(BRect rect);
};

#endif