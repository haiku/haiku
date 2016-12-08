/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include "SoftwareUpdaterWindow.h"

#include <stdio.h>

#include <AppDefs.h>
#include <Application.h>
#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdater"


const uint32 UPDATE_MESSAGE = 'iUPD';


SoftwareUpdaterWindow::SoftwareUpdaterWindow()
	:
	BWindow(BRect(0, 0, 500, 300), "Software Update",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
{
	BStringView* introText = new BStringView("intro",
		"Software updates are available.", B_WILL_DRAW);

	BButton* updateButton = new BButton("Apply Updates",
		new BMessage(UPDATE_MESSAGE));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_VERTICAL, B_USE_ITEM_SPACING)
			.SetInsets(B_USE_WINDOW_SPACING)
			.Add(introText)
			.Add(updateButton)
    ;
	CenterOnScreen();
	Show();
}


SoftwareUpdaterWindow::~SoftwareUpdaterWindow()
{
}


bool
SoftwareUpdaterWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
