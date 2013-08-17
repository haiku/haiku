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
#include "PackageInfoView.h"
#include "PackageListView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


MainWindow::MainWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	_InitDummyModel();

	BMenuBar* menuBar = new BMenuBar(B_TRANSLATE("Main Menu"));
	_BuildMenu(menuBar);
	
	fFilterView = new FilterView(fModel);
	fPackageListView = new PackageListView();
	fPackageInfoView = new PackageInfoView(&fPackageManager);
	
	fSplitView = new BSplitView(B_VERTICAL, 5.0f);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(menuBar)
		.Add(fFilterView)
		.AddSplit(fSplitView)
			.AddGroup(B_VERTICAL)
				.Add(fPackageListView)
				.SetInsets(
					B_USE_DEFAULT_SPACING, 0.0f,
					B_USE_DEFAULT_SPACING, 0.0f)
			.End()
			.Add(fPackageInfoView)
		.End()
	;

	fSplitView->SetCollapsible(0, false);
	fSplitView->SetCollapsible(1, false);

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

		case MSG_PACKAGE_SELECTED:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK
				&& index >= 0 && index < fVisiblePackages.CountItems()) {
				_AdoptPackage(fVisiblePackages.ItemAtFast(index));
			} else {
				_ClearPackage();
			}
			break;
		}
		
		case MSG_CATEGORY_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			fModel.SetCategory(name);
			_AdoptModel();
			break;
		}
		
		case MSG_DEPOT_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			fModel.SetDepot(name);
			_AdoptModel();
			break;
		}
		
		case MSG_SEARCH_TERMS_MODIFIED:
		{
			// TODO: Do this with a delay!
			BString searchTerms;
			if (message->FindString("search terms", &searchTerms) != B_OK)
				searchTerms = "";
			fModel.SetSearchTerms(searchTerms);
			_AdoptModel();
		}
		
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
	fVisiblePackages = fModel.CreatePackageList();
	
	fPackageListView->Clear();
	for (int32 i = 0; i < fVisiblePackages.CountItems(); i++) {
		fPackageListView->AddPackage(fVisiblePackages.ItemAtFast(i));
	}
}


void
MainWindow::_AdoptPackage(const PackageInfo& package)
{
	fPackageInfoView->SetPackage(package);
}


void
MainWindow::_ClearPackage()
{
	fPackageInfoView->Clear();
}


void
MainWindow::_InitDummyModel()
{
	// TODO: The Model could be filled from another thread by 
	// sending messages which contain collected package information.
	// The Model could be cached on disk.
	
	DepotInfo depot(B_TRANSLATE("Default"));

	// WonderBrush
	PackageInfo wonderbrush(
		BitmapRef(new SharedBitmap(601), true),
		"WonderBrush",
		"2.1.2",
		PublisherInfo(
			BitmapRef(),
			"YellowBites",
			"superstippi@gmx.de",
			"http://www.yellowbites.com"),
		"A vector based graphics editor.",
		"WonderBrush is YellowBites' software for doing graphics design "
		"on Haiku. It combines many great under-the-hood features with "
		"powerful tools and an efficient and intuitive interface.",
		"2.1.2 - Initial Haiku release.");
	wonderbrush.AddUserRating(
		UserRating(UserInfo("humdinger"), 4.5f,
		"Awesome!", "en", "2.1.2", 0, 0)
	);
	wonderbrush.AddUserRating(
		UserRating(UserInfo("bonefish"), 5.0f,
		"The best!", "en", "2.1.2", 3, 1)
	);
	wonderbrush.AddScreenshot(
		BitmapRef(new SharedBitmap(603), true));
	wonderbrush.AddCategory(fModel.CategoryGraphics());
	wonderbrush.AddCategory(fModel.CategoryProductivity());
	
	depot.AddPackage(wonderbrush);

	// Paladin
	PackageInfo paladin(
		BitmapRef(new SharedBitmap(602), true),
		"Paladin",
		"1.2.0",
		PublisherInfo(
			BitmapRef(),
			"DarkWyrm",
			"bpmagic@columbus.rr.com",
			"http://darkwyrm-haiku.blogspot.com"),
		"A C/C++ IDE based on Pe.",
		"If you like BeIDE, you'll like Paladin even better. "
		"The interface is streamlined, it has some features sorely "
		"missing from BeIDE, like running a project in the Terminal, "
		"and has a bundled text editor based upon Pe.",
		"");
	paladin.AddUserRating(
		UserRating(UserInfo("stippi"), 3.5f,
		"Could be more integrated from the sounds of it.",
		"en", "1.2.0", 0, 1)
	);
	paladin.AddUserRating(
		UserRating(UserInfo("mmadia"), 5.0f,
		"It rocks! Give a try",
		"en", "1.1.0", 1, 0)
	);
	paladin.AddUserRating(
		UserRating(UserInfo("bonefish"), 2.0f,
		"It just needs to use my jam-rewrite 'ham' and it will be great.",
		"en", "1.1.0", 3, 1)
	);
	paladin.AddScreenshot(
		BitmapRef(new SharedBitmap(605), true));
	paladin.AddCategory(fModel.CategoryDevelopment());

	depot.AddPackage(paladin);

	// Sequitur
	PackageInfo sequitur(
		BitmapRef(new SharedBitmap(604), true),
		"Sequitur",
		"2.1.0",
		PublisherInfo(
			BitmapRef(),
			"Angry Red Planet",
			"hackborn@angryredplanet.com",
			"http://www.angryredplanet.com"),
		"BeOS-native MIDI sequencer.",
		"Sequitur is a BeOS-native MIDI sequencer with a MIDI processing "
		"add-on architecture. It allows you to record, compose, store, "
		"and play back music from your computer. Sequitur is designed for "
		"people who like to tinker with their music. It facilitates rapid, "
		"dynamic, and radical processing of your performance.",
		"2.1.0 - 04 August 2002.\n\n"
		" * Important fix to file IO that prevents data corruption.\n\n"
		" * Dissolve filter could cause crash when operating on notes "
		"with extremely short durations. (thanks to David Shipman)\n\n"
		" * New tool bar ARP Curves (select Tool Bars->Show->ARP Curves "
		"in a track window) contains new tools for shaping events. Each "
		"tool is documented in the Tool Guide, but in general they are "
		"used to shape control change, pitch bend, tempo and velocity "
		"events (and notes if you're feeling creative), and the curve "
		"can be altered by pressing the 'z' and 'x' keys.\n\n"
		"All curve tools make use of a new tool seed for drawing "
		"bezier curves; see section 6.3.2. of the user's guide for an "
		"explanation of the Curve seed.\n\n"
		" * New menu items in the Song window: Edit->Expand Marked Range "
		"and Edit->Contract Marked Range. These items are only active if "
		"the loop markers are on. The Expand command shifts everything "
		"from the left marker to the end of the song over by the total "
		"loop range. The Contract command deletes the area within the "
		"loop markers, then shifts everything after the right marker "
		"left by the total loop range. If no tracks are selected, the "
		"entire song is shifted. Otherwise, only the selected tracks are "
		"affected.\n\n"
		" * System exclusive commands can now be added to devices. There "
		"is no user interface for doing so, although developer tools are "
		"provided here. Currently, the E-mu Proteus 2000 and E-mu EOS "
		"devices include sysex commands for controlling the FX.\n\n"
		" * A new page has been added to the Preferences window, Views. "
		"This page lets you set the height for the various views in "
		"Sequitur, such as pitch bend, control change, etc.\n\n"
		" * Multiple instances of the same MIDI device are given unique "
		"names. For example, two instances of SynC Modular will be "
		"named \"SynC Modular 1\" and \"SynC Modular 2\" so they can be "
		"accessed independently. (thanks to David Shipman)\n\n"
		" * In the track window, holding down the CTRL key now accesses "
		"an alternate set of active tools. (Assign these tools by "
		"holding down the CTRL key and clicking on the desired tool)\n\n"
		" * Right mouse button can now be mapped to any tool. (thanks "
		"to David Shipman)\n\n"
		" * The Vaccine.P and Vaccine.V filters have been deprecated "
		"(although they are still available here). They are replaced by "
		"the Vaccine filter. In addition to injecting a motion into the "
		"pitch and velocity of notes, this new filter can inject it "
		"into control change, pitch bend, tempo change and channel "
		"pressure events. The new tool Broken Down Line uses this "
		"filter.\n\n"
		" * Note to filter developers: The filter API has changed. You "
		"will need to recompile your filters.");
	sequitur.AddUserRating(
		UserRating(UserInfo("pete"), 4.5f,
		"I can weave a web of sound! And it connects to PatchBay. Check "
		"it out, I can wholeheartly recommend this app!! This rating "
		"comment is of course only so long, because the new TextView "
		"layout needs some testing. Oh, and did I mention it works with "
		"custom installed sound fonts? Reading through this comment I find "
		"that I did not until now. Hopefully there are enough lines now to "
		"please the programmer with the text layouting and scrolling of "
		"long ratings!", "en", "2.1.0", 4, 1)
	);
	sequitur.AddUserRating(
		UserRating(UserInfo("stippi"), 3.5f,
		"It seems very capable. Still runs fine on Haiku. The interface "
		"is composed of many small, hard to click items. But you can "
		"configure a tool for each mouse button, which is great for the "
		"work flow.", "en", "2.1.0", 2, 1)
	);
	sequitur.AddCategory(fModel.CategoryAudio());
	
	depot.AddPackage(sequitur);

	// Finish off
	fModel.AddDepot(depot);

	fModel.SetPackageState(wonderbrush, UNINSTALLED);
	fModel.SetPackageState(paladin, ACTIVATED);
}
