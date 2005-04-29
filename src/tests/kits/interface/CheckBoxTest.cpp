/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Window.h>
#include <Box.h>
#include <StringView.h>

// include Be's and our version (see Jamfile)
#include <CheckBox.h>
#define BCheckBox HCheckBox
#undef _CHECK_BOX_H
#include <CheckBox.h>
#undef BCheckBox

#include <stdio.h>


class DividedBackgroundView : public BView {
	public:
		DividedBackgroundView(BRect rect);

		virtual void Draw(BRect updateRect);
};


DividedBackgroundView::DividedBackgroundView(BRect rect)
	: BView(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(80, 120, 80);
}


void 
DividedBackgroundView::Draw(BRect updateRect)
{
	PushState();
	
	BRect bounds = Bounds();
	BRect rect = bounds;
	rect.right = rect.left + bounds.Width() / 2;

	SetLowColor(120, 0, 0);
	FillRect(rect, B_SOLID_LOW);

	rect.left = rect.right + 1;
	rect.right = bounds.right;

	SetLowColor(0, 0, 120);
	FillRect(rect, B_SOLID_LOW);
}


//	#pragma mark -


class Window : public BWindow {
	public:
		Window();

		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 520, 430), "CheckBox-Test",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect(20, 10, 200, 30);
	BStringView *stringView = new BStringView(rect, NULL, "Be's BCheckBox");
	stringView->SetFont(be_bold_font);
	AddChild(stringView);

	rect.OffsetBy(0, 40);
	BView *checkBox = new BCheckBox(rect, NULL, "Test 1", NULL);
	AddChild(checkBox);

	rect.OffsetBy(0, 60);
	BView *view = new BView(rect.InsetByCopy(-15, -15), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	view->SetViewColor(240, 180, 20);
	AddChild(view);

	checkBox = new BCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 2", NULL);
	view->AddChild(checkBox);
	checkBox->SetViewColor(220, 170, 20);

	rect.OffsetBy(0, 60);
	BBox *box = new BBox(rect.InsetByCopy(-15, -15), NULL);
	AddChild(box);

	checkBox = new BCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 3", NULL);
	checkBox->SetViewColor(220, 170, 20);
		// is ignored...
	box->AddChild(checkBox);

	rect.OffsetBy(0, 60);
	view = new DividedBackgroundView(rect.InsetByCopy(-15, -15));
	AddChild(view);

	checkBox = new BCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 4", NULL);
	view->AddChild(checkBox);

	rect.OffsetBy(0, 60);
	view = new DividedBackgroundView(rect.InsetByCopy(-15, -15));
	AddChild(view);

	checkBox = new BCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 5", NULL);
	checkBox->ResizeToPreferred();
	view->AddChild(checkBox);

	// Haiku's BCheckBox

	rect.Set(240, 10, 400, 30);
	stringView = new BStringView(rect, NULL, "Haiku's BCheckBox");
	stringView->SetFont(be_bold_font);
	AddChild(stringView);

	rect.OffsetBy(0, 40);
	checkBox = new HCheckBox(rect, NULL, "Test 1", NULL);
	AddChild(checkBox);

	rect.OffsetBy(0, 60);
	view = new BView(rect.InsetByCopy(-15, -15), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	view->SetViewColor(240, 180, 20);
	AddChild(view);

	checkBox = new HCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 2", NULL);
	view->AddChild(checkBox);
	checkBox->SetViewColor(220, 170, 20);

	rect.OffsetBy(0, 60);
	box = new BBox(rect.InsetByCopy(-15, -15), NULL);
	AddChild(box);

	checkBox = new HCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 3", NULL);
	checkBox->SetViewColor(220, 170, 20);
		// is ignored...
	box->AddChild(checkBox);

	rect.OffsetBy(0, 60);
	view = new DividedBackgroundView(rect.InsetByCopy(-15, -15));
	AddChild(view);

	checkBox = new HCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 4", NULL);
	view->AddChild(checkBox);

	rect.OffsetBy(0, 60);
	view = new DividedBackgroundView(rect.InsetByCopy(-15, -15));
	AddChild(view);

	checkBox = new HCheckBox(rect.OffsetToCopy(15, 15), NULL, "Test 5", NULL);
	checkBox->ResizeToPreferred();
	view->AddChild(checkBox);
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun(void);
};


Application::Application()
	: BApplication("application/x-vnd.obos-test")
{
}


void
Application::ReadyToRun(void)
{
	BWindow *window = new Window();
	window->Show();
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	Application app;

	app.Run();
	return 0;
}

