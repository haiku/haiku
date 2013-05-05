/*
 * Copyright 1999-2010, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * OverlayImage is based on the code presented in this article:
 * http://www.haiku-os.org/documents/dev/replishow_a_replicable_image_viewer
 *
 * Authors:
 *			Seth Flexman
 *			Hartmuth Reh
 *			Humdinger		<humdingerb@gmail.com>
 */

#include "OverlayView.h"
#include "OverlayWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <String.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main window"


OverlayWindow::OverlayWindow() 
	:
	BWindow(BRect(50, 50, 500, 200), B_TRANSLATE_SYSTEM_NAME("OverlayImage"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	OverlayView *replView = new OverlayView(Bounds());
	AddChild(replView);

	BView *bgView = new BView(Bounds(), "bgView", B_FOLLOW_ALL, B_WILL_DRAW);
	bgView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bgView);
}


bool
OverlayWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
