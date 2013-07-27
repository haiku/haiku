/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <TabView.h>

#include "FilterView.h"
#include "PackageActionsView.h"
#include "PackageInfoView.h"
#include "PackageListView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


MainWindow::MainWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	BMenuBar* mainMenu = new BMenuBar(B_TRANSLATE("Main Menu"));
	BMenu* menu = new BMenu(B_TRANSLATE("Package"));
	mainMenu->AddItem(menu);
	
	FilterView* filterView = new FilterView();
	
	PackageListView* packageListView = new PackageListView();

	PackageActionsView* packageActionsView = new PackageActionsView();

	PackageInfoView* packageInfoView = new PackageInfoView();
	
	BSplitView* splitView = new BSplitView(B_VERTICAL, B_USE_SMALL_SPACING);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(mainMenu)
		.Add(filterView)
		.AddGroup(B_VERTICAL)
			.AddSplit(splitView)
				.Add(packageListView)
				.Add(packageInfoView)
			.End()
			.Add(packageActionsView)
			.SetInsets(B_USE_DEFAULT_SPACING, 0.0f, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING)
		.End()
	;

	splitView->SetCollapsible(0, false);
}


MainWindow::~MainWindow()
{
}


bool
MainWindow::QuitRequested()
{
	BMessage message(MSG_MAIN_WINDOW_CLOSED);
	message.AddRect("window frame", Frame());
	be_app->PostMessage(&message);
	
	return true;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
