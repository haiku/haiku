////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFWindow.h
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef GIFWINDOW_H
#define GIFWINDOW_H

#include <Window.h>
class GIFView;

class GIFWindow : public BWindow {
	public:
		GIFWindow(BRect rect, const char *name);
		bool QuitRequested();
		GIFView *gifview;
};

#endif

