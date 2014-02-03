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


static const char* kAppSignature = "application/x.vnd-Haiku.ClipToPicture";


class TestRenderer {
public:
	virtual	void				Draw(BView* view, BRect updateRect) = 0;
};


class Test {
public:
								Test(const char* name,
									TestRenderer* clippingTest,
									TestRenderer* validateTest);
								~Test();

			const char*			Name() const
									{ return fName.String(); }

			TestRenderer*		ClippingTest() const
									{ return fClippingTest; }
			TestRenderer*		ValidateTest() const
									{ return fValidateTest; }

private:
			BString				fName;
			TestRenderer*		fClippingTest;
			TestRenderer*		fValidateTest;
};


Test::Test(const char* name, TestRenderer* clippingTest,
		TestRenderer* validateTest)
	:
	fName(name),
	fClippingTest(clippingTest),
	fValidateTest(validateTest)
{
}


Test::~Test()
{
	delete fClippingTest;
	delete fValidateTest;
}


// #pragma mark - TestView


class TestView : public BView {
public:
								TestView();
	virtual						~TestView();

	virtual	void				Draw(BRect updateRect);

			void				SetTestRenderer(TestRenderer* renderer);

private:
			TestRenderer*		fTestRenderer;
};


TestView::TestView()
	:
	BView(NULL, B_WILL_DRAW),
	fTestRenderer(NULL)
{
}


TestView::~TestView()
{
}


void
TestView::Draw(BRect updateRect)
{
	if (fTestRenderer != NULL)
		fTestRenderer->Draw(this, updateRect);
}


void
TestView::SetTestRenderer(TestRenderer* renderer)
{
	fTestRenderer = renderer;
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
			TestView*			fClippingTestView;
			TestView*			fValidateTestView;

			BMenuField*			fTestSelectionField;

			BList				fTests;
};


TestWindow::TestWindow()
	:
	BWindow(BRect(50.0, 50.0, 450.0, 250.0), "ClipToPicture Test",
		B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	fClippingTestView = new TestView();
	fValidateTestView = new TestView();

	BGroupView* group = new BGroupView(B_HORIZONTAL);
	BLayoutBuilder::Group<>(group, B_HORIZONTAL, 0.0f)
		.Add(fClippingTestView)
		.Add(fValidateTestView)
	;

	BScrollView* scrollView = new BScrollView("scroll", group, 0, true, true);

	BStringView* leftLabel = new BStringView("left label", 
		"ClipToPicture:");
	leftLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BStringView* rightLabel = new BStringView("right label", 
		"Validation:");
	rightLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fTestSelectionField = new BMenuField("test selection",
		"Select test:", new BPopUpMenu("select"));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.AddGroup(B_HORIZONTAL)
			.Add(fTestSelectionField)
			.AddGlue()
			.SetInsets(B_USE_DEFAULT_SPACING)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(leftLabel)
			.Add(rightLabel)
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

	fClippingTestView->SetTestRenderer(test->ClippingTest());
	fValidateTestView->SetTestRenderer(test->ValidateTest());
}


// #pragma mark - Test1


class Test1Clipping : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		BPicture picture;
		view->BeginPicture(&picture);
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->DrawString("Clipping to text.", BPoint(20, 30));
		view->EndPicture();

		view->ClipToPicture(&picture);

		view->FillRect(view->Bounds());
	}
};


class Test1Validate : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->DrawString("Clipping to text.", BPoint(20, 30));
	}
};


// #pragma mark - Test2


class Test2Clipping : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->PushState();
		
		BPicture picture;
		view->BeginPicture(&picture);
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->SetScale(2.0);
		view->DrawString("Scaled clipping", BPoint(10, 15));
		view->DrawString("to text.", BPoint(10, 25));
		view->EndPicture();

		view->PopState();

		view->ClipToPicture(&picture);

		view->FillRect(view->Bounds());
	}
};


class Test2Validate : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->SetScale(2.0);
		view->DrawString("Scaled clipping", BPoint(10, 15));
		view->DrawString("to text.", BPoint(10, 25));
	}
};


// #pragma mark - Test2


class Test3Clipping : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		BPicture picture;
		view->BeginPicture(&picture);
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->DrawString("Scaled picture", BPoint(10, 15));
		view->EndPicture();

		view->SetScale(2.0);
		view->ClipToPicture(&picture);

		view->SetScale(1.0);
		view->FillRect(view->Bounds());
	}
};


class Test3Validate : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->SetScale(2.0);
		view->DrawString("Scaled picture", BPoint(10, 15));
	}
};


// #pragma mark - Test2


class Test4Clipping : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

		BPicture rect;
		view->BeginPicture(&rect);
		view->FillRect(BRect(20, 20, 50, 50));
		view->EndPicture();

		view->ClipToPicture(&rect);

		view->PushState();
		BPicture circle;
		view->BeginPicture(&circle);
		view->FillEllipse(BRect(20, 20, 50, 50));
		view->EndPicture();
		
		view->ClipToInversePicture(&circle);

		view->FillRect(view->Bounds());

		view->PopState();
	}
};


class Test4Validate : public TestRenderer {
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->FillRect(BRect(20, 20, 50, 50));
		view->FillEllipse(BRect(20, 20, 50, 50), B_SOLID_LOW);
	}
};


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	TestWindow* window = new TestWindow();

	window->AddTest(new Test("Simple clipping",
		new Test1Clipping(), new Test1Validate()));

	window->AddTest(new Test("Scaled clipping 1",
		new Test2Clipping(), new Test2Validate()));

	window->AddTest(new Test("Scaled clipping 2",
		new Test3Clipping(), new Test3Validate()));

	window->AddTest(new Test("Nested states",
		new Test4Clipping(), new Test4Validate()));

	window->SetToTest(3);
	window->Show();

	app.Run();
	return 0;
}
