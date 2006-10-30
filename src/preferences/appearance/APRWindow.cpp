/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */

#include <Messenger.h>
#include "APRWindow.h"
#include "APRView.h"
#include "defs.h"

APRWindow::APRWindow(BRect frame)
 :	BWindow(frame, "Appearance", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
 			B_ALL_WORKSPACES )
{
	APRView *colorview = new APRView(Bounds(),"Colors",B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(colorview);
}

bool
APRWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
