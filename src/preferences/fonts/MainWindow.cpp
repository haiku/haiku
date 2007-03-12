/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "FontView.h"
#include "MainWindow.h"

#include <Application.h>
#include <Button.h>
#include <MessageRunner.h>
#include <Screen.h>
#include <TabView.h>


static const uint32 kMsgSetDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgCheckFonts = 'chkf';


MainWindow::MainWindow()
	: BWindow(BRect(100, 100, 445, 340), "Fonts", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BRect rect = Bounds();
	BView* view = new BView(rect, "background", B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect.left = 10;
	rect.top = rect.bottom - 10;
	BButton *button = new BButton(rect, "defaults", "Defaults",
		new BMessage(kMsgSetDefaults), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	button->ResizeToPreferred();
	float buttonHeight = button->Bounds().Height();
	button->MoveBy(0, -buttonHeight);
	view->AddChild(button);

	rect = button->Frame();
	rect.OffsetBy(button->Bounds().Width() + 10, 0);

	fRevertButton = new BButton(rect, "revert", "Revert",
		new BMessage(kMsgRevert), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetEnabled(false);
	view->AddChild(fRevertButton);

	rect = Bounds();
	rect.top += 5;
	rect.bottom -= 20 + buttonHeight;
	BTabView *tabView = new BTabView(rect, "tabview", B_WIDTH_FROM_LABEL); 

	rect = tabView->ContainerView()->Bounds().InsetByCopy(5, 8);
	fFontsView = new FontView(rect);

	tabView->AddTab(fFontsView);

	fFontsView->UpdateFonts();
	fFontsView->RelayoutIfNeeded();
	float width, height;
	fFontsView->GetPreferredSize(&width, &height);

	// make sure the tab is large enough for the fonts view
	float widthDiff = width + 10
		- tabView->ContainerView()->Bounds().Width();
	if (widthDiff > 0) {
		tabView->ResizeBy(widthDiff, 0);
		tabView->ContainerView()->ResizeBy(widthDiff, 0);
	}

	float heightDiff = height + 16
		- tabView->ContainerView()->Bounds().Height();
	if (heightDiff > 0) {
		tabView->ResizeBy(0, heightDiff);
		tabView->ContainerView()->ResizeBy(0, heightDiff);
	}

	ResizeTo(tabView->Bounds().Width(), tabView->Frame().bottom + 20 + buttonHeight);
	view->AddChild(tabView);
	fFontsView->ResizeToPreferred();

	if (fSettings.WindowCorner() == BPoint(-1, -1)) {
		// center window on screen
		_Center();
	} else {
		MoveTo(fSettings.WindowCorner());

		// make sure window is on screen
		BScreen screen(this);
		if (!screen.Frame().InsetByCopy(10, 10).Intersects(Frame()))
			_Center();
	}

	fRunner = new BMessageRunner(this, new BMessage(kMsgCheckFonts), 3000000);
		// every 3 seconds
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
		case kMsgUpdate:
			fRevertButton->SetEnabled(fFontsView->IsRevertable());
			break;

		case kMsgSetDefaults:
			fFontsView->SetDefaults();
			fRevertButton->SetEnabled(fFontsView->IsRevertable());
			break;

		case kMsgRevert:
			fFontsView->Revert();
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
	BScreen screen;
	BRect screenFrame = screen.Frame();

	MoveTo(screenFrame.left + (screenFrame.Width() - Bounds().Width()) / 2,
		screenFrame.top + (screenFrame.Height() - Bounds().Height()) / 2);
}
