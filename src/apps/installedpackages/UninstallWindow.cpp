/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#include "UninstallWindow.h"
#include <Application.h>


// Macro reserved for later localization
#define T(x) x


UninstallWindow::UninstallWindow()
	:	BWindow(BRect(100, 100, 600, 300), T("Installed packages"), 
			B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	fBackground = new UninstallView(Bounds());
	AddChild(fBackground);

	ResizeTo(Bounds().Width(), fBackground->Bounds().Height());
}


UninstallWindow::~UninstallWindow()
{
	RemoveChild(fBackground);
	
	delete fBackground;
}


bool
UninstallWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

