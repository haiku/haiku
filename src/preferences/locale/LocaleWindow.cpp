/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Locale.h"
#include "LocaleWindow.h"

#include <Application.h>
#include <Screen.h>
#include <TabView.h>
#include <ScrollView.h>
#include <ListView.h>
#include <Button.h>


const static uint32 kMsgSelectLanguage = 'slng';
const static uint32 kMsgDefaults = 'dflt';
const static uint32 kMsgRevert = 'revt';


LocaleWindow::LocaleWindow(BRect rect)
	: BWindow(rect, "Locale", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	rect = Bounds();
	BView *view = new BView(rect, "view", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	BButton *button = new BButton(rect, "defaults", "Defaults",
		new BMessage(kMsgDefaults), B_FOLLOW_NONE);
	button->ResizeToPreferred();
	button->MoveTo(10, rect.bottom - 10 - button->Bounds().Height());
	view->AddChild(button);

	fRevertButton = new BButton(rect, "revert", "Revert",
		new BMessage(kMsgRevert), B_FOLLOW_NONE);
	fRevertButton->ResizeToPreferred();
	fRevertButton->MoveTo(20 + button->Bounds().Width(), button->Frame().top);
	fRevertButton->SetEnabled(false);
	view->AddChild(fRevertButton);

	rect.InsetBy(10, 10);
	rect.bottom -= 10 + button->Bounds().Height();
	BTabView *tabView = new BTabView(rect, "tabview");

	rect = tabView->ContainerView()->Bounds();
	rect.InsetBy(2, 2);
	BView *tab = new BView(rect, "Language", B_FOLLOW_NONE, B_WILL_DRAW);
	tab->SetViewColor(tabView->ViewColor());
	tabView->AddTab(tab);

	{
		BRect frame = rect;
		frame.InsetBy(12, 12);
		frame.right = 100 + B_V_SCROLL_BAR_WIDTH;
		frame.bottom = 150;

		BListView *listView = new BListView(frame, "preferred", B_SINGLE_SELECTION_LIST, 
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
		listView->SetSelectionMessage(new BMessage(kMsgSelectLanguage));

		BScrollView *scrollView = new BScrollView("scroller", listView, 
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 0, false, true, B_FANCY_BORDER);
		tab->AddChild(scrollView);
	}

	tab = new BView(rect, "Country", B_FOLLOW_NONE, B_WILL_DRAW);
	tab->SetViewColor(tabView->ViewColor());
	tabView->AddTab(tab);

	tab = new BView(rect, "Keyboard", B_FOLLOW_NONE, B_WILL_DRAW);
	tab->SetViewColor(tabView->ViewColor());
	tabView->AddTab(tab);

	view->AddChild(tabView);

	// check if the window is on screen

	rect = BScreen().Frame();
	rect.right -= 20;
	rect.bottom -= 20;

	BPoint position = Frame().LeftTop();
	if (!rect.Contains(position)) {
		// center window on screen as it doesn't fit on the saved position
		position.x = (rect.Width() - Bounds().Width()) / 2;
		position.y = (rect.Height() - Bounds().Height()) / 2;
	}
	MoveTo(position);
}


bool
LocaleWindow::QuitRequested()
{
	BMessage update(kMsgSettingsChanged);
	update.AddRect("window_frame", Frame());
	be_app_messenger.SendMessage(&update);

	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}


void
LocaleWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgDefaults:
			// reset default settings
			break;

		case kMsgRevert:
			// revert to last settings
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

