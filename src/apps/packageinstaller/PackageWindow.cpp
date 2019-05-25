/*
 * Copyright (c) 2007-2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "PackageWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>

#include "PackageView.h"
#include "main.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageWindow"


PackageWindow::PackageWindow(const entry_ref* ref)
	:
	BWindow(BRect(100, 100, 600, 300),
		B_TRANSLATE_SYSTEM_NAME("LegacyPackageInstaller"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	PackageView* view = new PackageView(ref);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(view)
	;

	CenterOnScreen();
}


PackageWindow::~PackageWindow()
{
}


void
PackageWindow::Quit()
{
	be_app->PostMessage(P_WINDOW_QUIT);
	BWindow::Quit();
}

