/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 *		Philippe Saint-Pierre, stpere@gmail.com
 */

#include "MainWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Box.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MessageRunner.h>
#include <Screen.h>
#include <TabView.h>
#include <TextView.h>

#include "FontView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main window"


static const uint32 kMsgSetDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgCheckFonts = 'chkf';


MainWindow::MainWindow()
	: BWindow(BRect(0, 0, 1, 1), B_TRANSLATE_SYSTEM_NAME("Fonts"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgSetDefaults), B_WILL_DRAW);
	fDefaultsButton->SetEnabled(false);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert), B_WILL_DRAW);
	fRevertButton->SetEnabled(false);

//	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	BBox* box = new BBox(B_FANCY_BORDER, NULL);

	fFontsView = new FontView();

//	tabView->AddTab(fFontsView);
	box->AddChild(fFontsView);

	fFontsView->UpdateFonts();

	const float kInset = be_control_look->DefaultItemSpacing();
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(kInset, kInset, kInset, kInset)
		.Add(box)
//		.Add(tabView)
		.AddStrut(kInset)
		.AddGroup(B_HORIZONTAL)
			.Add(fDefaultsButton)
			.AddStrut(kInset)
			.Add(fRevertButton)
			.AddGlue();

	if (fSettings.WindowCorner() == BPoint(-1, -1)) {
		// center window on screen
		CenterOnScreen();
	} else {
		MoveTo(fSettings.WindowCorner());

		// make sure window is on screen
		BScreen screen(this);
		if (!screen.Frame().InsetByCopy(10, 10).Intersects(Frame()))
			CenterOnScreen();
	}

	fRunner = new BMessageRunner(this, new BMessage(kMsgCheckFonts), 3000000);
		// every 3 seconds

	fDefaultsButton->SetEnabled(fFontsView->IsDefaultable());
}


MainWindow::~MainWindow()
{
	delete fRunner;
}


bool
MainWindow::QuitRequested()
{
	fSettings.SetWindowCorner(Frame().LeftTop());

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgSetSize:
		case kMsgSetFamily:
		case kMsgSetStyle:
			fFontsView->MessageReceived(message);
			break;

		case kMsgUpdate:
			fDefaultsButton->SetEnabled(fFontsView->IsDefaultable());
			fRevertButton->SetEnabled(fFontsView->IsRevertable());
			break;

		case kMsgSetDefaults:
			fFontsView->SetDefaults();
			fDefaultsButton->SetEnabled(false);
			fRevertButton->SetEnabled(fFontsView->IsRevertable());
			break;

		case kMsgRevert:
			fFontsView->Revert();
			fDefaultsButton->SetEnabled(fFontsView->IsDefaultable());
			fRevertButton->SetEnabled(false);
			break;

		case kMsgCheckFonts:
			if (update_font_families(true))
				fFontsView->UpdateFonts();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::_Center()
{
	BRect screenFrame = BScreen(this).Frame();
	BRect windowRect = Frame();

	MoveTo(
		(screenFrame.Width() - windowRect.Width())  / 2,
		(screenFrame.Height() - windowRect.Height()) / 2);
}
