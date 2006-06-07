/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "ScreenSaverWindow.h"

#include <WindowPrivate.h>

#include <Application.h>
#include <View.h>


/*!
	This is the BDirectWindow subclass that rendering occurs in.
	A view is added to it so that BView based screensavers will work.
*/
ScreenSaverWindow::ScreenSaverWindow(BRect frame) 
	: BDirectWindow(frame, "ScreenSaver Window", 
		B_NO_BORDER_WINDOW_LOOK, kWindowScreenFeel, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fSaver(NULL) 
{
	frame.OffsetTo(0, 0);
	fTopView = new BView(frame, "ScreenSaver View", B_FOLLOW_ALL, B_WILL_DRAW);
	fTopView->SetViewColor(0, 0, 0);
	AddChild(fTopView);
}


ScreenSaverWindow::~ScreenSaverWindow() 
{
	Hide();
}


void
ScreenSaverWindow::SetSaver(BScreenSaver *saver)
{
	fSaver = saver;
}


bool 
ScreenSaverWindow::QuitRequested() 
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void 
ScreenSaverWindow::DirectConnected(direct_buffer_info *info) 
{
	if (fSaver)
		fSaver->DirectConnected(info);
}

