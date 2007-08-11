/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */


#include <Application.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <StringView.h>
#include <ListItem.h>
#include <View.h>

#include <stdio.h>

#include "PictureTest.h"
#include "PictureTestCases.h"
#include "PictureTestWindow.h"
#include "TestResultItem.h"

PictureTestWindow::PictureTestWindow()
	: Inherited(BRect(100,100,500,300), "Picture Tests", B_DOCUMENT_WINDOW, 0)
{
	BuildGUI();
}

bool PictureTestWindow::QuitRequested()
{
	bool isOk = Inherited::QuitRequested();
	if (isOk) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	
	return isOk;
}


void PictureTestWindow::BuildGUI()
{
	BView* backdrop = new BView(Bounds(), "backdrop", B_FOLLOW_ALL, B_WILL_DRAW);
	backdrop->SetViewColor(::ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(backdrop);
	
	BMenuBar* mb = new BMenuBar(Bounds(), "menubar");
	BMenu* m = new BMenu("File");
		m->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
		m->SetTargetForItems(be_app_messenger);
	mb->AddItem(m);

	m = new BMenu("Tests");
		m->AddItem(new BMenuItem("Run", new BMessage(kMsgRunTests), 'R'));
	// not implemented
	//	m->AddSeparatorItem();
	//	m->AddItem(new BMenuItem("Write images", new BMessage(kMsgWriteImages), 'W'));
	mb->AddItem(m);

	backdrop->AddChild(mb);

	BRect b = Bounds();
	b.top = mb->Bounds().bottom + 1;
	
	BStringView *header = new BStringView(b, "header", "Picture, Unflattened Picture, Test Name, Error Message", B_FOLLOW_LEFT | B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	header->ResizeToPreferred();
	backdrop->AddChild(header);
	b.top = header->Frame().bottom + 1;
	
	
	b.right -= B_V_SCROLL_BAR_WIDTH;
	b.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fListView = new BListView(b, "Results", B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
	backdrop->AddChild(new BScrollView("scroll_results", fListView, B_FOLLOW_ALL_SIDES, 0, true, true));	
}

void
PictureTestWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case kMsgRunTests:
			RunTests();
			break;
	}
	Inherited::MessageReceived(msg);
}


void
PictureTestWindow::RunTests()
{
	color_space colorSpaces[] = {
		B_RGBA32,
		B_RGB32,
		B_RGB24,
		B_RGB16,
		B_RGB15
	};
	BRect frame(0, 0, 100, 30);
	for (uint32 csIndex = 0; csIndex < sizeof(colorSpaces)/sizeof(color_space); csIndex ++) {
		color_space colorSpace = colorSpaces[csIndex];
		const char *csText;
		switch (colorSpace) {
			case B_RGBA32:
				csText = "B_RGB32";
				break;
			case B_RGB32:
				csText = "B_RGB32";
				break;
			case B_RGB24:
				csText = "B_RGB24";
				break;
			case B_RGB16:
				csText = "B_RGB16";
				break;
			case B_RGB15:
				csText = "B_RGB15";
				break;
			default:
				csText = "Unknown";
		}
		
		BString text;
		text = "Color space: ";
		text += csText;
		fListView->AddItem(new BStringItem(text.String()));
		
		for (int i = 0; gTestCases[i].name != NULL; i ++) {
			TestCase *testCase = &gTestCases[i];
			PictureTest test;
			test.SetColorSpace(colorSpace);
			bool ok = test.Test(testCase->func, frame);
			
			TestResultItem *item = new TestResultItem(testCase->name, frame);
			item->SetOk(ok);
			item->SetErrorMessage(test.ErrorMessage());
			item->SetOriginalBitmap(test.GetOriginalBitmap(true));
			item->SetArchivedBitmap(test.GetArchivedBitmap(true));
			
			fListView->AddItem(item);
		}
	}
}