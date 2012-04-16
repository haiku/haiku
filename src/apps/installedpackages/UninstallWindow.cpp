/*
 * Copyright (c) 2007-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "UninstallWindow.h"

#include <Catalog.h>
#include <GroupLayout.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UninstallWindow"


UninstallWindow::UninstallWindow()
	:
	BWindow(BRect(100, 100, 600, 300),
		B_TRANSLATE_SYSTEM_NAME("InstalledPackages"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(new UninstallView());
}

