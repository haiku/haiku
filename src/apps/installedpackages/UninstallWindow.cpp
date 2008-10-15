/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#include "UninstallWindow.h"

#include <GroupLayout.h>


// Macro reserved for later localization
#define T(x) x


UninstallWindow::UninstallWindow()
	: BWindow(BRect(100, 100, 600, 300), T("Installed packages"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE
		| B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BView* view = new UninstallView(Bounds());
	AddChild(view);

	ResizeTo(Bounds().Width(), view->Bounds().Height());
}

