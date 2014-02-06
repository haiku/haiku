/*
 * Copyright 2014 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <GradientLinear.h>
#include <Message.h>
#include <Picture.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
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

		BRect rect(view->Bounds());
		rect.InsetBy(rect.Width() / 3, rect.Height() / 3);
		BPoint center(
			rect.left + rect.Width() / 2,
			rect.top + rect.Height() / 2);

		for (int32 i = 0; i < 360; i += 40) {
			BAffineTransform transform;
			transform.RotateBy(center, i * M_PI / 180.0);
			view->SetTransform(transform);

			view->SetHighColor(51, 151, 255, 20);
			view->FillRect(rect);

			view->SetHighColor(51, 255, 151, 180);
			view->DrawString("Rect", center);
		}
	}
};


// #pragma mark - BitmapTest


class BitmapTest : public Test {
public:
	BitmapTest()
		:
		Test("Bitmap"),
		fBitmap(_LoadBitmap(555))
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		BRect rect(view->Bounds());

		if (fBitmap == NULL) {
			view->SetHighColor(255, 0, 0);
			view->FillRect(rect);
			view->SetHighColor(0, 0, 0);
			view->DrawString("Failed to load the bitmap.", BPoint(20, 20));
			return;
		}

		rect.left = (rect.Width() - fBitmap->Bounds().Width()) / 2;
		rect.top = (rect.Height() - fBitmap->Bounds().Height()) / 2;
		rect.right = rect.left + fBitmap->Bounds().Width();
		rect.bottom = rect.top + fBitmap->Bounds().Height();

		BPoint center(
			rect.left + rect.Width() / 2,
			rect.top + rect.Height() / 2);

		BPicture picture;
		view->BeginPicture(&picture);
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		BFont font;
		view->GetFont(&font);
		font.SetSize(70);
		view->SetFont(&font);
		view->SetHighColor(0, 0, 0, 80);
		view->FillRect(view->Bounds());
		view->SetHighColor(0, 0, 0, 255);
		view->DrawString("CLIPPING", BPoint(0, center.y + 35));
		view->EndPicture();

		view->ClipToPicture(&picture);

		BAffineTransform transform;
			transform.RotateBy(center, 30 * M_PI / 180.0);
			view->SetTransform(transform);
	
		view->DrawBitmap(fBitmap, fBitmap->Bounds(), rect);
	}

private:
	status_t
	_GetAppResources(BResources& resources) const
	{
		app_info info;
		status_t status = be_app->GetAppInfo(&info);
		if (status != B_OK)
			return status;
	
		return resources.SetTo(&info.ref);
	}


	BBitmap* _LoadBitmap(int resourceID) const
	{
		BResources resources;
		status_t status = _GetAppResources(resources);
		if (status != B_OK)
			return NULL;
	
		size_t dataSize;
		const void* data = resources.LoadResource(B_MESSAGE_TYPE, resourceID,
			&dataSize);
		if (data == NULL)
			return NULL;

		BMemoryIO stream(data, dataSize);

		// Try to read as an archived bitmap.
		BMessage archive;
		status = archive.Unflatten(&stream);
		if (status != B_OK)
			return NULL;

		BBitmap* bitmap = new BBitmap(&archive);

		status = bitmap->InitCheck();
		if (status != B_OK) {
			delete bitmap;
			bitmap = NULL;
		}

		return bitmap;
	}

private:
	BBitmap*	fBitmap;
};


// #pragma mark - Gradient


class GradientTest : public Test {
public:
	GradientTest()
		:
		Test("Gradient")
	{
	}
	
	virtual void Draw(BView* view, BRect updateRect)
	{
		BRect rect(view->Bounds());
		rect.InsetBy(rect.Width() / 3, rect.Height() / 3);
		BPoint center(
			rect.left + rect.Width() / 2,
			rect.top + rect.Height() / 2);

		BAffineTransform transform;
		transform.RotateBy(center, 30.0 * M_PI / 180.0);
		view->SetTransform(transform);

		rgb_color top = (rgb_color){ 255, 255, 0, 255 };
		rgb_color bottom = (rgb_color){ 0, 255, 255, 255 };

		BGradientLinear gradient;
		gradient.AddColor(top, 0.0f);
		gradient.AddColor(bottom, 255.0f);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());

		float radius = std::min(rect.Width() / 5, rect.Height() / 5);

		view->FillRoundRect(rect, radius, radius, gradient);
	}
};


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	TestWindow* window = new TestWindow();

	window->AddTest(new RectsTest());
	window->AddTest(new BitmapTest());
	window->AddTest(new GradientTest());

	window->SetToTest(2);
	window->Show();

	app.Run();
	return 0;
}
