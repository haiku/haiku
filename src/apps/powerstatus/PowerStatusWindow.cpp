/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "PowerStatusWindow.h"
#include "PowerStatusView.h"

#include <Application.h>
#include <Catalog.h>


PowerStatusWindow::PowerStatusWindow()
	:
	BWindow(BRect(100, 150, 281, 299), B_TRANSLATE_SYSTEM_NAME("PowerStatus"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BView* topView = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	topView->AddChild(new PowerStatusReplicant(Bounds(), B_FOLLOW_ALL));
}


PowerStatusWindow::~PowerStatusWindow()
{
}


bool
PowerStatusWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
