/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Locale.h"
#include "LocaleWindow.h"

#include <Application.h>
#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TabView.h>


const static uint32 kMsgSelectLanguage = 'slng';
const static uint32 kMsgDefaults = 'dflt';
const static uint32 kMsgRevert = 'revt';


LocaleWindow::LocaleWindow(BRect rect)
	: BWindow(rect, "Locale", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// Buttons at the bottom

	BButton *button = new BButton("Defaults", new BMessage(kMsgDefaults));

	fRevertButton = new BButton("Revert", new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	// Tabs
	BTabView *tabView = new BTabView("tabview");

	// Language tab

	BView *tab = new BView("Language", B_WILL_DRAW);
	tab->SetViewColor(tabView->ViewColor());
	tab->SetLayout(new BGroupLayout(B_VERTICAL,10));
	tabView->AddTab(tab);

	{
		BListView *listView = new BListView("preferred", B_SINGLE_SELECTION_LIST);
		listView->SetSelectionMessage(new BMessage(kMsgSelectLanguage));

		BScrollView *scrollView = new BScrollView("scroller", listView, 
			0, false, true, B_FANCY_BORDER);
		tab->AddChild(BGroupLayoutBuilder(B_HORIZONTAL,10)
			.Add(scrollView)
			.AddGlue()
		);
	}

	// Country tab

	tab = new BView("Country", B_WILL_DRAW);
	tab->SetViewColor(tabView->ViewColor());
	tabView->AddTab(tab);

	// Keyboard tab

	tab = new BView("Keyboard", B_WILL_DRAW);
	tab->SetViewColor(tabView->ViewColor());
	tabView->AddTab(tab);

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

	// Layout management

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL,10)
		.Add(tabView)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL,10)
			.Add(button)
			.Add(fRevertButton)
			.AddGlue()
		)
		.SetInsets(5,5,5,5)
	);
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

