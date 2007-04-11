/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "NetworkStatusWindow.h"
#include "NetworkStatusView.h"

#include <Application.h>


NetworkStatusWindow::NetworkStatusWindow()
	: BWindow(BRect(150, 150, 249, 249), "NetworkStatus", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BView* topView = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	SetSizeLimits(25, 265, 25, 265);

	topView->AddChild(new NetworkStatusView(Bounds().InsetByCopy(5, 5),
		B_FOLLOW_ALL));
}


NetworkStatusWindow::~NetworkStatusWindow()
{
}


bool
NetworkStatusWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
