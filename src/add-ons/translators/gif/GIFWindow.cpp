////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFWindow.cpp
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

#include "GIFWindow.h"
#include "GIFView.h"
#include <Application.h>
#include <LayoutBuilder.h>

GIFWindow::GIFWindow(BRect rect, const char *name) :
	BWindow(rect, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
	| B_AUTO_UPDATE_SIZE_LIMITS, B_CURRENT_WORKSPACE) {

	gifview = new GIFView("GIFView");
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.Add(gifview);
}

bool GIFWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

