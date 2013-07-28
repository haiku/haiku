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
	BMenuBar* menuBar = new BMenuBar(B_TRANSLATE("Main Menu"));
	_BuildMenu(menuBar);
	
	fFilterView = new FilterView();
	fPackageListView = new PackageListView();
	fPackageActionsView = new PackageActionsView();
	fPackageInfoView = new PackageInfoView();
	
	fSplitView = new BSplitView(B_VERTICAL, B_USE_SMALL_SPACING);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(menuBar)
		.Add(fFilterView)
		.AddGroup(B_VERTICAL)
			.AddSplit(fSplitView)
				.Add(fPackageListView)
				.Add(fPackageInfoView)
			.End()
			.Add(fPackageActionsView)
			.SetInsets(B_USE_DEFAULT_SPACING, 0.0f, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING)
		.End()
	;

	fSplitView->SetCollapsible(0, false);
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


void
MainWindow::_BuildMenu(BMenuBar* menuBar)
{
	BMenu* menu = new BMenu(B_TRANSLATE("Package"));
	menuBar->AddItem(menu);

}
