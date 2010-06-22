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
#include <TextView.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Main window"


OverlayWindow::OverlayWindow() 
	:
	BWindow(BRect(100, 100, 450, 350), "OverlayImage", 
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	OverlayView *replView = new OverlayView(Bounds());
	AddChild(replView);

	BView *bgView = new BView(Bounds(), "bgView", B_FOLLOW_ALL, B_WILL_DRAW);
	bgView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bgView);
	
	BString text;
	text << B_TRANSLATE("Enable \"Show replicants\" in Deskbar.");
	text << "\n";
	text << B_TRANSLATE("Drag & drop an image.");
	text << "\n";
	text << B_TRANSLATE("Drag the replicant to the Desktop.");
	replView->SetToolTip(text);
}


bool
OverlayWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
