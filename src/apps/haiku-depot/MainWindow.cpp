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
	fPackageInfoView = new PackageInfoView();
	fPackageActionsView = new PackageActionsView();
	
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

	_InitDummyModel();
	_AdoptModel();
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


void
MainWindow::_AdoptModel()
{
	PackageInfoList packages = fModel.CreatePackageList();
	
	fPackageListView->Clear();
	for (int32 i = 0; i < packages.CountItems(); i++) {
		fPackageListView->AddPackage(packages.ItemAtFast(i));
	}
}


void
MainWindow::_InitDummyModel()
{
	// TODO: The Model could be filled from another thread by 
	// sending messages which contain collected package information.
	// The Model could be cached on disk.
	
	DepotInfo depot(B_TRANSLATE("Default"));

	PackageInfo wonderbrush(
		"WonderBrush",
		"2.1.2",
		"A vector based graphics editor.",
		"2.1.2 - Initial Haiku release.");
	wonderbrush.AddUserRating(
		UserRating(UserInfo("humdinger"), 4.5f,
		"Awesome!", "en", "2.1.2")
	);
	wonderbrush.AddUserRating(
		UserRating(UserInfo("bonefish"), 5.0f,
		"The best!", "en", "2.1.2")
	);
	depot.AddPackage(wonderbrush);

	PackageInfo paladin(
		"Paladin",
		"1.2.0",
		"A C/C++ IDE based on Pe.",
		"");
	paladin.AddUserRating(
		UserRating(UserInfo("stippi"), 3.5f,
		"Could be more integrated from the sounds of it.",
		"en", "1.2.0")
	);
	paladin.AddUserRating(
		UserRating(UserInfo("mmadia"), 5.0f,
		"It rocks! Give a try",
		"en", "1.1.0")
	);
	paladin.AddUserRating(
		UserRating(UserInfo("bonefish"), 2.0f,
		"It just needs to use my jam-rewrite 'ham' and it will be great.",
		"en", "1.1.0")
	);
	depot.AddPackage(paladin);

	fModel.AddDepot(depot);
}
