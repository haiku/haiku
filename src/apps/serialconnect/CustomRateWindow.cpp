/*
 * Copyright 2016, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include "CustomRateWindow.h"

#include "SerialApp.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <Spinner.h>


#define B_TRANSLATION_CONTEXT "Custom baudrate window"


static const uint32 kOkButtonMsg = 'ok';

CustomRateWindow::CustomRateWindow(int baudrate)
	: BWindow(BRect(100, 100, 200, 150), B_TRANSLATE("Custom baudrate"),
		B_FLOATING_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_CLOSE_ON_ESCAPE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	BGroupLayout* layout = new BGroupLayout(B_HORIZONTAL);
	SetLayout(layout);

	BGroupView* root = new BGroupView(B_VERTICAL);
	AddChild(root);

	BGroupLayoutBuilder(root)
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(fSpinner = new BSpinner("spin", B_TRANSLATE("Baudrate:"), NULL))
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(new BButton("ok", B_TRANSLATE("OK"), new BMessage(kOkButtonMsg)))
			.Add(new BButton("cancel", B_TRANSLATE("Cancel"),
				new BMessage(B_QUIT_REQUESTED)))
		.End()
	.End();

	fSpinner->SetMinValue(50);
	fSpinner->SetMaxValue(3000000);
	fSpinner->SetValue(baudrate);

	CenterOnScreen();
}


void
CustomRateWindow::MessageReceived(BMessage* message)
{
	if (message->what == kOkButtonMsg)
	{
		BMessage* settings = new BMessage(kMsgSettings);
		settings->AddInt32("baudrate", fSpinner->Value());
		be_app->PostMessage(settings);
		Quit();
		return;
	}

	BWindow::MessageReceived(message);
}
