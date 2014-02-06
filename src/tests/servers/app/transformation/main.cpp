/*
 * Copyright 2014 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <Picture.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>


static const char* kAppSignature = "application/x.vnd-Haiku.Transformation";


class Test {
public:
								Test(const char* name);
	virtual						~Test();

			const char*			Name() const
									{ return fName.String(); }

	virtual	void				Draw(BView* view, BRect updateRect) = 0;

private:
			BString				fName;
};


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


class TestWindow : public BWindow {
public:
								TestWindow();
	virtual						~TestWindow();

	virtual	void				MessageReceived(BMessage* message);

			void				AddTest(Test* test);
			void				SetToTest(int32 index);

private:
			TestView*			fTestView;

			BMenuField*			fTestSelectionField;

			BList				fTests;
};


TestWindow::TestWindow()
	:
	BWindow(BRect(50.0, 50.0, 450.0, 250.0), "Transformations Test",
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


// #pragma mark - Test1


class RectsTest : public Test {
public:
	RectsTest()
		:
		Test("Rects")
	{
	}
	
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->DrawString("Rects", BPoint(20, 30));

		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->SetHighColor(51, 151, 255, 40);

		BRect rect(view->Bounds());
		rect.InsetBy(rect.Width() / 3, rect.Height() / 3);
		BPoint center(
			rect.left + rect.Width() / 2,
			rect.top + rect.Height() / 2);

		for (int32 i = 0; i < 360; i += 20) {
			BAffineTransform transform;
			transform.RotateBy(center, i * M_PI / 180.0);
			view->SetTransform(transform);
			view->FillRect(rect);
		}
	}
};


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	TestWindow* window = new TestWindow();

	window->AddTest(new RectsTest());

	window->SetToTest(0);
	window->Show();

	app.Run();
	return 0;
}
