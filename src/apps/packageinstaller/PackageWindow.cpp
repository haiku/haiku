/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageWindow.h"

#include <Application.h>

#include <GroupLayout.h>

// Macro reserved for later localization
#define T(x) x


PackageWindow::PackageWindow(const entry_ref *ref)
	:	BWindow(BRect(100, 100, 600, 300), T("Package Installer"), B_TITLED_WINDOW,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	//SetLayout(new BGroupLayout(B_HORIZONTAL));

	fBackground = new PackageView(Bounds(), ref);
	AddChild(fBackground);

	ResizeTo(Bounds().Width(), fBackground->Bounds().Height());
}


PackageWindow::~PackageWindow()
{
	RemoveChild(fBackground);
	
	delete fBackground;
}


void
PackageWindow::Quit()
{
	be_app->PostMessage(P_WINDOW_QUIT);
	BWindow::Quit();
}

