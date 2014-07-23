/*
 * Copyright 2014 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "harness.h"


#include <Application.h>
#include <Bitmap.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollView.h>


Test::Test(const char* name)
	:
	fName(name)
{
}


Test::~Test()
{
}


// #pragma mark - TestView


class TestView : public BView {
public:
								TestView();
	virtual						~TestView();

	virtual	void				Draw(BRect updateRect);

			void				SetTest(Test* test);

private:
			Test*				fTest;
};


TestView::TestView()
	:
	BView(NULL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fTest(NULL)
{
}


TestView::~TestView()
{
}


void
TestView::Draw(BRect updateRect)
{
	if (fTest != NULL)
		fTest->Draw(this, updateRect);
}


void
TestView::SetTest(Test* test)
{
	fTest = test;
	Invalidate();
}


// #pragma mark - TestWindow


enum {
	MSG_SELECT_TEST	= 'stst'
};


TestWindow::TestWindow(const char* title)
	:
	BWindow(BRect(50.0, 50.0, 450.0, 250.0), title,
		B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	fTestView = new TestView();

	BScrollView* scrollView = new BScrollView("scroll", fTestView, 0, true,
		true);

	fTestSelectionField = new BMenuField("test selection",
		"Select test:", new BPopUpMenu("select"));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.AddGroup(B_HORIZONTAL)
			.Add(fTestSelectionField)
			.AddGlue()
			.SetInsets(B_USE_DEFAULT_SPACING)
		.End()
		.Add(scrollView)
	;
}


TestWindow::~TestWindow()
{
	for (int32 i = fTests.CountItems() - 1; i >= 0; i++)
		delete (Test*)fTests.ItemAt(i);
}


void
TestWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SELECT_TEST:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				SetToTest(index);
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


void
TestWindow::AddTest(Test* test)
{
	if (test == NULL || fTests.HasItem(test))
		return;

	if (!fTests.AddItem(test)) {
		delete test;
		return;
	}

	BMessage* message = new BMessage(MSG_SELECT_TEST);
	message->AddInt32("index", fTests.CountItems() - 1);

	BMenuItem* item = new BMenuItem(test->Name(), message);
	if (!fTestSelectionField->Menu()->AddItem(item)) {
		fTests.RemoveItem(fTests.CountItems() - 1);
		delete test;
		delete item;
		return;
	}

	if (fTests.CountItems() == 1)
		SetToTest(0);
}


void
TestWindow::SetToTest(int32 index)
{
	Test* test = (Test*)fTests.ItemAt(index);
	if (test == NULL)
		return;

	fTestSelectionField->Menu()->ItemAt(index)->SetMarked(true);

	fTestView->SetTest(test);
}


