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
	: Inherited(BRect(10, 30, 630, 470), "Bitmap Drawing Tests", B_DOCUMENT_WINDOW, 0)
	, fFailedTests(0)
	, fNumberOfTests(0)
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
		m->AddItem(new BMenuItem("Run Color Space B_RGB32", new BMessage(kMsgRunTests1), 'S'));
	mb->AddItem(m);

	backdrop->AddChild(mb);

	BRect b = Bounds();
	b.top = mb->Bounds().bottom + 1;

	fHeader = new BStringView(b, "header", 
		"X", B_FOLLOW_LEFT | B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	float width, height;
	fHeader->GetPreferredSize(&width, &height);
	fHeader->ResizeTo(b.Width(), height);
	backdrop->AddChild(fHeader);
	b.top = fHeader->Frame().bottom + 1;

	b.right -= B_V_SCROLL_BAR_WIDTH;
	b.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fListView = new BListView(b, "Results", B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
	backdrop->AddChild(new BScrollView("scroll_results", fListView, B_FOLLOW_ALL_SIDES, 0, true, true));	

	UpdateHeader();
}

void
PictureTestWindow::UpdateHeader()
{
	BString text;
	text << "failures = " << fFailedTests << ",  tests =" << fNumberOfTests;
	fHeader->SetText(text.String());
}

void
PictureTestWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case kMsgRunTests:
			RunTests();
			break;
		case kMsgRunTests1:
			RunTests1();
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

	RunTests(colorSpaces, sizeof(colorSpaces) / sizeof(color_space));	
}

void
PictureTestWindow::RunTests1()
{
	color_space colorSpaces[] = {
		B_RGBA32
	};
	RunTests(colorSpaces, 1);
}

void
PictureTestWindow::RunTests(color_space *colorSpaces, int32 n)
{
	for (int testIndex = 0; testIndex < 2; testIndex ++) {
		BString text;
		switch (testIndex)
		{
			case 0:
				text = "Flatten Picture Test";
				break;
			case 1:
				text = "Archive Picture Test";
				break;
			default:
				text = "Unknown test method!";
		}
		fListView->AddItem(new BStringItem(text.String()));
		RunTests(testIndex, colorSpaces, n);
	}

	UpdateHeader();
}

void 
PictureTestWindow::RunTests(int32 testIndex, color_space *colorSpaces, int32 n)
{
	for (int32 csIndex = 0; csIndex < n; csIndex ++) {
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
		
		BRect frame(0, 0, 100, 30);
		fListView->AddItem(new HeaderListItem("Direct Drawing", "Picture Drawing",
						"Restored Picture", "", "Test Name", "Error Message", frame));

		RunTests(testIndex, colorSpace);
	}
}

void 
PictureTestWindow::RunTests(int32 testIndex, color_space colorSpace)
{
	BRect frame(0, 0, 100, 30);
	for (int i = 0; gTestCases[i].name != NULL; i ++) {
		TestCase *testCase = &gTestCases[i];
		PictureTest *test;
		switch (testIndex) {
			case 0:
				test = new FlattenPictureTest();
				break;
			case 1:
				test = new ArchivePictureTest();
				break;
			default:
				continue;
		}
		
		test->SetColorSpace(colorSpace);
		bool ok = test->Test(testCase->func, frame);
		
		TestResultItem *item = new TestResultItem(testCase->name, frame);
		item->SetOk(ok);
		item->SetErrorMessage(test->ErrorMessage());
		item->SetDirectBitmap(test->DirectBitmap(true));
		item->SetOriginalBitmap(test->BitmapFromPicture(true));
		item->SetArchivedBitmap(test->BitmapFromRestoredPicture(true));
		
		delete test;
		
		fListView->AddItem(item);

		fNumberOfTests ++;
		if (!ok)
			fFailedTests ++;
	}
}
