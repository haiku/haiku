/*
 * Copyright (c) 2007-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageWindow"


PackageWindow::PackageWindow(const entry_ref *ref)
	:
	BWindow(BRect(100, 100, 600, 300),
		B_TRANSLATE_SYSTEM_NAME("PackageInstaller"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
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

